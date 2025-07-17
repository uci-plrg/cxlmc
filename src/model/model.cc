#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include <atomic>
#include <cstring>
#include <string>

#include "model.h"
#include "scheduler.h"
#include "executor.h"
#include "futex.h"

Model *model = nullptr;
bool inside_model = false;
// whether to do backtrack search
bool backtrack = true;
mspace shared_space = NULL;
mspace snapshot_space = NULL;
mspace cxl_space = NULL;

uint64_t Model::action(ModelAction* action, bool yield) {
    scheduler->assert_active();
	if (is_crashed(process_id)) {
		scheduler->process_crash();
		exit(EXIT_SUCCESS);
	}
	inside_model = true;

    Thread* curr_thread = scheduler->current_thread();
    curr_thread->set_pending(action);
	if (yield && !action->is_second_part_of_rmw())
		scheduler->yield();
	if (action->is_read() || action->is_write())
		ensureInitialValue(action);
	if (action->get_type() == CACHE_CLFLUSHOPT)
		action->set_earliest_effect(next_sequence_num);
    execute(action);
    curr_thread->set_pending(nullptr);

	uint64_t val = action->get_value();
    delete action; 

	process_store_buffer();
	inside_model = false;
	return val;
}

void Model::process_store_buffer() {
	//placeholder store buffer policy, to be changed later
	uint thread_count = scheduler->get_thread_count();
	uint thread_to_pop = rand()%thread_count;
	for (uint i = 0; i < thread_count; i++) {
		process_id_t pid = scheduler->get_thread(thread_to_pop)->get_process_id();
		if (is_live(pid))
			break;
		thread_to_pop = (thread_to_pop+1) % thread_count;
	}
	
	ThreadMemory *mem = scheduler->get_thread(thread_to_pop)->get_thread_memory();

	if (mem->get_store_buffer_size() == 0)
		return;

	uint num_to_pop = random()%EVICT_MAX;
	if (num_to_pop >= mem->get_store_buffer_size())
		num_to_pop = mem->get_store_buffer_size()-1;
	for (uint j = 0; j < num_to_pop; j++) {
		mem->pop_from_store_buffer();
	}
}

process_id_t Model::get_process_id(ModelAction *action) { 
	return scheduler->get_thread(action->get_thread_id())->get_process_id(); 
}

Model::storeList &Model::get_storelist(void *addr)  { 
	void *aligned = alignAddress(addr);
	return obj_to_wr[aligned];
}

void Model::evict_store(ModelAction* action) {
    assert(action->is_write());

    storeList &stores = get_storelist(action->get_location());
	action->set_seq_num(get_next_sequence_num());
    stores.push_back(action);
}

void Model::evict_clflush(ModelAction* action) {
    assert(action->get_type() == CACHE_CLFLUSH || action->get_type() == CACHE_CLFLUSHOPT);
								
	modelclock_t seq_num;
	if (action->get_type() == CACHE_CLFLUSH) {
		seq_num = get_next_sequence_num();
		action->set_seq_num(seq_num);
	} else
		seq_num = action->get_earliest_effect();
	uintptr_t addr = getCacheID(action->get_location());
	cacheline cl = obj_to_cl.get_cacheline(addr);
	auto stores = get_storelist(action->get_location());

	for (auto itr = stores.rbegin(); itr != stores.rend(); itr++) {
		if ((*itr)->get_seq_num() <= cl.getBegin() ||
			crashes.size() >= MAX_CRASHES_PER_EXECUTION || 
			crashes.size() >= (unsigned) scheduler->get_process_count()-1)

			break;
		auto wpid = get_process_id(*itr);
		if (is_live(wpid))
			insert_crash(wpid);
	}

	if (seq_num > cl.getBegin())
		obj_to_cl.set_cacheline(addr, cacheline{seq_num, cl.getEnd()});
}

void Model::check_memory_poisoning(ModelAction *read) {
	uintptr_t cache_addr = getCacheID(read->get_location());
    modelclock_t poison_begin = 0;
	modelclock_t poison_crash;
    for (int i=0; i < CACHELINE_SIZE/8; i++) {
		void *addr = ((char *)cache_addr) + i;
		storeList &stores = get_storelist(addr);
        if (stores.size() == 0)
            continue;
        auto head = *stores.begin();
		auto citr = crashes.find(get_process_id(head));
		if (citr == crashes.end())
            continue;
		cacheline &cl = obj_to_cl.get_cacheline(cache_addr, citr->second);
        modelclock_t seq_num = head->get_seq_num();
        if (cl.getEnd() != 0 && read->get_seq_num() > cl.getEnd()) {
			fprintf(stderr, "poison value on read at %s\n", read->get_position());
			exit(EXIT_FAILURE);
		}
        if (seq_num > cl.getBegin() && seq_num > poison_begin) {
            poison_begin = seq_num;
            poison_crash = citr->second; 
        }
    }

    if (poison_begin != 0) {
		cacheline &cl = obj_to_cl.get_cacheline(cache_addr, poison_crash);
        if (decision_point(2) == 0) {
			obj_to_cl.set_cacheline(cache_addr, cacheline{cl.getBegin(), poison_begin});
			fprintf(stderr, "poison value on read at %s\n", read->get_position());
			exit(EXIT_FAILURE);
        } else {
            obj_to_cl.set_cacheline(cache_addr, cacheline{poison_begin, cl.getEnd()});
        }
    }
}

uint64_t Model::build_read_from(ModelAction *read) {	
	uintptr_t cache_addr = getCacheID(read->get_location());
	auto pos = read->get_position();
	uint numslotsleft = read->get_size();
	//optimize: store delta to data in rfEntry
	auto rf = new shared::vector<ModelAction *>(numslotsleft);

	if(scheduler->get_thread(read->get_thread_id())->get_thread_memory()->local_bypassing(read, *rf, numslotsleft)) {
		uint64_t ret = get_read_value(read->get_location(), *rf);
		delete rf;
		return ret;
	}

    storeList &stores = get_storelist(read->get_location());
    assert(stores.size() != 0);

#ifdef MEM_POISON
    check_memory_poisoning(read);
#endif

	int branch = 0;

	process_id_t rpid = get_process_id(read);
	unsigned p_count = scheduler->get_process_count();
	bool at_backtrack = false;

	for (auto itr = stores.rbegin(); itr != stores.rend(); itr++) {
		ModelAction *store = *itr;
		process_id_t wpid = get_process_id(store);
		uint slotsleft_copy = numslotsleft;
		auto citr = crashes.find(wpid);

		//running processes
		if (citr == crashes.end()) {
			if (auto old = get_overlaps_save_old(store, read, *rf, numslotsleft)) {
				//consider crashing the writing process before the read
				if (backtrack && store->get_type() != ATOMIC_INIT && wpid != rpid) {
					cacheline cl = obj_to_cl.get_cacheline(cache_addr);
					unsigned crash_count = crashes.size();
					if (crash_count < MAX_CRASHES_PER_EXECUTION &&
						crash_count + 1< p_count &&
						!is_completed(wpid) && 
						store->get_seq_num() > cl.getBegin() &&
                        ++crash_points &&
						decision_point(2, &at_backtrack) == 0)
					{
						crashes.emplace(wpid, next_sequence_num);
						obj_to_cl.insert_crash(next_sequence_num);
						delete rf;
						rf = old;
						numslotsleft = slotsleft_copy;
					} else {
						branch++;
						delete old;
                        if (store->get_seq_num() > cl.getBegin())
							obj_to_cl.set_cacheline(cache_addr, cacheline{store->get_seq_num(), cl.getEnd()});
					}
				} else
					delete old;
			}
		} 
		citr = crashes.find(wpid);
		if(citr != crashes.end()) { //crashed processes
			modelclock_t crash_clock = citr->second;
			cacheline &cl = obj_to_cl.get_cacheline(cache_addr, crash_clock);
			if (store->get_seq_num() <= cl.getBegin()) { //must have persisted
				get_overlaps(store, read, *rf, numslotsleft);
			} else if (cl.getEnd() == 0 || store->get_seq_num() < cl.getEnd()) { //may have persisted
#ifdef EQUIV_STORE_SKIP
                // With two equivalent may-persist stores
                // the second can be skipped
                // this avoids blow-up in # of executions in some cases
                auto next_itr = itr;
                next_itr++;
                if(next_itr != stores.rend() && store->equivalent(*next_itr))
                    continue;
#endif
				if (auto old = get_overlaps_save_old(store, read, *rf, numslotsleft)) {
					//not persisted
					if (store->get_type() != ATOMIC_INIT && backtrack && decision_point(2, &at_backtrack) == 0) {
						delete rf;
						rf = old;
						numslotsleft = slotsleft_copy;
						cl.setEnd(store->get_seq_num());
					//persisted
					} else {
						branch++;
						delete old;
						obj_to_cl.set_cacheline(cache_addr, cacheline{store->get_seq_num(), cl.getEnd()});
					}
				}
			}
		}
		
		if (numslotsleft == 0) {
			uint64_t ret = get_read_value(read->get_location(), *rf);
#if DEBUG_LEVEL > 0
			if (pos && at_backtrack) {
				printf("backtrack to node %d at %s of process %d, branch %d of rfset, read %lx\n", nodestack->get_head_idx(), pos, process_id, branch, ret);
				printf("read-from: ");
				for (uint i = 0; i < rf->size(); i++)
					if (i==0 || (*rf)[i] != (*rf)[i-1])
						printf("(+%d: val=%lx, seq=%u), ", i, (*rf)[i]->get_value(), (*rf)[i]->get_seq_num());
				printf("\n");
			}
#endif 
			delete rf;
			return ret;
		}
	}

	assert(false);
	return VALUE_NONE;
}

void Model::print_execution_summary() {
		printf("\nExecution Summary\n");
        printf("stores: \n");
        for (auto &itr: obj_to_wr) {
			printf("cacheline %p [", itr.first);
			for (auto s: itr.second) {
				int offset = (char *) s->get_location() - (char *) itr.first;
				printf("(+%d: val=%lx, seq=%u), ", offset, s->get_value(), s->get_seq_num());
			}
			printf("]\n");
		}
        printf("\n");

        printf("constraints: \n");
		obj_to_cl.dump();	
        printf("\n");

		printf("crashed processes: \n");
		for (auto &pair: crashes)
			printf("p%d at %d, ", pair.first, pair.second);
		printf("\n\n");
}

void Model::terminate_early() {
    rollback_again = false;
    finish_execution();
    _Exit(EXIT_FAILURE);
}

void Model::finish_execution() {
    bool isLast = !scheduler->finalize();
	if (VERBOSE > 0)
		printf("process %d done\n", process_id);
    int num = execution_num.load();

    if (isLast) {
		if (VERBOSE > 1)
			print_execution_summary();
        if (VERBOSE > 0)
            printf("cumulative crash count: %d\n", crash_points);
        printf("\n");
		reset_execution_data();
		scheduler->reset();
        printf("crash points: %d\n", crash_points);
        crash_points = 0;
		//printf("Shared Space Memory Usage:\n");
		//mspace_malloc_stats(shared_space);

        rollback_again = rollback_again && num+1 <= MAX_EXECUTION && nodestack->has_another_execution();
        if (rollback_again) {
            nodestack->reset_execution();
			if (execution_num_save == num + 1)
				nodestack->save_state(num + 1, ns_save);
		}

        execution_num.store(num+1);
#ifdef SYNC_WAIT
		fwake((uint32_t*)&execution_num);
#endif
    }
}

bool Model::wait_for_next_execution(int num) {
    if (num > MAX_EXECUTION || !rollback_again) {
        return false;
    }

	int loaded;
    while ((loaded = execution_num.load()) < num) {
#ifdef SYNC_WAIT
		fwait((uint32_t*)&execution_num, loaded);
#endif
    }

    return rollback_again;
}

void Model::reset_execution_data() {
		next_sequence_num = 0;
		for (auto& itr: obj_to_wr)
			for (ModelAction* s: itr.second)
				delete s;
        obj_to_wr.clear();
		obj_to_cl.clear();
		crashes.clear();
		completed_procs.clear();
		for (auto &itr: mutex_map)
			delete itr.second;
		for (auto &itr: cond_map)
			delete itr.second;
		mutex_map.clear();
		cond_map.clear();
}

int Model::decision_point(int numchoices, bool *at_backtrack) {
	if (at_backtrack && nodestack->next_is_curr_backtrack())
		*at_backtrack = true;
	return nodestack->explore_next(numchoices)->get_choice(); 
}

bool Model::should_crash() {
    unsigned crash_count = crashes.size();
    return backtrack &&
        crash_count < MAX_CRASHES_PER_EXECUTION &&
        crash_count + 1 < (unsigned) scheduler->get_process_count() &&
        ++crash_points &&
        decision_point(2) == 0;
}

void Model::insert_crash(process_id_t pid) {
    if (!should_crash()) {
        return;
    }
	
	if (pid == process_id) {
		crashes[process_id] = next_sequence_num;
		obj_to_cl.insert_crash(next_sequence_num);
		scheduler->process_crash();
		exit(EXIT_SUCCESS);
	}
	
	crashes.emplace(pid, next_sequence_num);
	obj_to_cl.insert_crash(next_sequence_num);
}

void Model::ensureInitialValue(ModelAction *action) {
    void* addr = alignAddress(action->get_location());
    Model::storeList& list = get_storelist(addr);
    if (list.size() == 0) {
		list.push_back(new ModelAction(ATOMIC_INIT, addr, *(uint64_t*)addr, memory_order_relaxed, 8, "ensureInitialValue"));
	}
}

bool Model::mem_is_cxl(const void *addr) {
	return ((cxl_mapping != NULL) &&
					(((uintptr_t)addr) >= ((uintptr_t)cxl_mapping)) &&
					(((uintptr_t)addr) < (((uintptr_t)cxl_mapping) + CXL_MEM_SIZE)));

}

void Model::load_nodestack(char *filepath) {
	int exec_num = nodestack->set_state(filepath);
	execution_num.store(exec_num);
}
