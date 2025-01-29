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
mspace shared_space = NULL;
mspace snapshot_space = NULL;
mspace cxl_space = NULL;

uint64_t Model::action(ModelAction* action, bool yield) {
    scheduler->assert_active();
	if (is_crashed(process_id)) {
		scheduler->process_crash();
		exit(EXIT_SUCCESS);
	}

    Thread* curr_thread = scheduler->current_thread();
    curr_thread->set_pending(action);
	if (yield && !action->is_second_part_of_rmw())
	    scheduler->yield();
	if (action->is_read() || action->is_write())
		ensureInitialValue(action);
	inside_model = true;
    execute(action);
    curr_thread->set_pending(nullptr);

	uint64_t val = action->get_value();
    delete action; 

	process_store_buffer();
	inside_model = false;
	return val;
}

bool Model::is_crashed(process_id_t pid) {
	return crashes.find(pid) != crashes.end();
}

void Model::process_store_buffer() {
	//placeholder store buffer policy, to be changed later
	uint thread_count = scheduler->get_thread_count();
	uint thread_to_pop = rand()%thread_count;
	for (uint i = 0; i < thread_count; i++) {
		if (!is_crashed(scheduler->get_thread(thread_to_pop)->get_process_id()))
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
	action->set_seq_num(get_next_sequence_num());
    get_storelist(action->get_location()).push_back(action);
}

void Model::evict_clflush(ModelAction* action) {
    assert(action->get_type() == CACHE_CLFLUSH || action->get_type() == CACHE_CLFLUSHOPT);
								
	modelclock_t seq_num = get_next_sequence_num();
	action->set_seq_num(seq_num);
	uintptr_t addr = getCacheID(action->get_location());
	cacheline cl = obj_to_cl.get_cacheline(addr);
	if (!empty_flush(get_storelist(action->get_location()), cl.getBegin()))
		insert_crash();
	obj_to_cl.set_cacheline(addr, cacheline{seq_num, cl.getEnd()});
	delete action;
}

uint64_t Model::build_read_from(ModelAction *read) {	
	uintptr_t addr = getCacheID(read->get_location());
	uint numslotsleft = read->get_size();
	//optimize: store delta to data in rfEntry
	auto rf = new shared::vector<ModelAction *>(numslotsleft);

	if(scheduler->get_thread(read->get_thread_id())->get_thread_memory()->get_latest_writes(read, *rf, numslotsleft)) {
		uint64_t ret = get_read_value(read->get_location(), *rf);
		delete rf;
		return ret;
	}

	storeList &stores = get_storelist(read->get_location());
	process_id_t rpid = get_process_id(read);
	unsigned p_count = scheduler->get_process_count();
		
	for (auto itr = stores.rbegin(); itr != stores.rend(); itr++) {
		ModelAction *store = *itr;
		process_id_t wpid = get_process_id(store);
		uint slotsleft_copy = numslotsleft;
		auto citr = crashes.find(wpid);

		//running processes
		if (citr == crashes.end()) {
			if (auto old = get_overlaps_save_old(store, read, *rf, numslotsleft)) {
				cacheline cl = obj_to_cl.get_cacheline(addr);
				//consider crashing the writing process before the read
				if (store->get_type() != ATOMIC_INIT && 
						wpid != rpid &&	
						!scheduler->get_thread(store->get_thread_id())->is_completed() && 
						crashes.size() < MAX_CRASHES_PER_EXECUTION &&
						crashes.size() + 1 < p_count &&
						!empty_flush(stores, cl.getBegin()) &&
						decision_point(2, read->get_position()) == 0) 
				{
					crashes.emplace(wpid, next_sequence_num);
					obj_to_cl.insert_crash(next_sequence_num);
					delete rf;
					rf = old;
					numslotsleft = slotsleft_copy;
				} else {
					delete old;
					cacheline &new_cl = obj_to_cl.set_cacheline(addr, cacheline{next_sequence_num, cl.getEnd()});
					set_new_cl_end(stores, itr, new_cl);
				}
			}
		} 
		citr = crashes.find(wpid);
		if(citr != crashes.end()) { //crashed processes
			modelclock_t crash_clock = citr->second;
			cacheline &cl = obj_to_cl.get_cacheline(addr, crash_clock);
			if (store->get_seq_num() <= cl.getBegin()) { //must have persisted
				if (get_overlaps(store, read, *rf, numslotsleft)) {
					set_new_cl_end(stores, itr, cl);
				}
			} else if (cl.getEnd() == 0 || store->get_seq_num() < cl.getEnd()) { //may have persisted
				if (auto old = get_overlaps_save_old(store, read, *rf, numslotsleft)) {
					//not persisted
					if (store->get_type() != ATOMIC_INIT && decision_point(2, read->get_position()) == 0) {
						delete rf;
						rf = old;
						numslotsleft = slotsleft_copy;
					//persisted
					} else {
						delete old;
						cacheline &new_cl = obj_to_cl.set_cacheline(addr, cacheline{store->get_seq_num(), cl.getEnd()});
						set_new_cl_end(stores, itr, new_cl);
					}
				}
			}
		}
		
		if (numslotsleft == 0) {
			uint64_t ret = get_read_value(read->get_location(), *rf);
			delete rf;
			return ret;
		}
	}

	assert(false);
	return 0;
}

//inline?
void Model::set_new_cl_end(const storeList &stores, storeList::reverse_iterator itr, cacheline &cl) {
	ModelAction *write = *itr;
	auto fitr = itr.base();
	uintptr_t wbot = (uintptr_t) write->get_location();
	uintptr_t wtop = wbot + write->get_size();
	//find next overlapping write
	for (; fitr != stores.end(); fitr++) {
		uintptr_t wbot2 = (uintptr_t) (*fitr)->get_location();
		uintptr_t wtop2 = wbot2 + (*fitr)->get_size();
		if (wtop > wbot2 && wbot < wtop2) {
			modelclock_t next_write_seq = (*fitr)->get_seq_num();
			if (cl.getEnd() == 0 || cl.getEnd() > next_write_seq)
				cl.setEnd(next_write_seq);
			break;
		}
	}
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
        
        printf("\n");
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
		inside_model = true;
		if (VERBOSE > 0)
			print_execution_summary();
		reset_execution_data();
		scheduler->reset();
		printf("Shared Space Memory Usage:\n");
		mspace_malloc_stats(shared_space);
		inside_model = false;

        rollback_again = rollback_again && num+1 <= MAX_EXECUTION && nodestack->has_another_execution();
        if (rollback_again) {
            nodestack->reset_execution();
			if (execution_num_save == num + 1)
				nodestack->save_state(num + 1, ns_save);
		}
        execution_num.store(num+1);
		fwake((uint32_t*)&execution_num);
    }
}

bool Model::wait_for_next_execution(int num) {
    if (num > MAX_EXECUTION || !rollback_again) {
        return false;
    }

	int loaded;
    while ((loaded = execution_num.load()) < num) {
		fwait((uint32_t*)&execution_num, loaded);
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
		for (auto &itr: mutex_map)
			delete itr.second;
		for (auto &itr: cond_map)
			delete itr.second;
		mutex_map.clear();
		cond_map.clear();
}

int Model::decision_point(int numchoices, const char *pos) {
	if (DEBUG_LEVEL > 0 && pos && nodestack->next_is_curr_backtrack())
		printf("backtrack to %s of process %d\n", pos, process_id);
	return nodestack->explore_next(numchoices)->get_choice(); 
}

bool Model::should_crash() {
    if (crashes.size() < MAX_CRASHES_PER_EXECUTION && (process_id_t) crashes.size() + 1 < scheduler->get_process_count() && decision_point(2) == 0) {
        return true;
    }
    return false;
}

void Model::insert_crash() {
    if (!should_crash()) {
        return;
    }
	
	crashes[process_id] = next_sequence_num;
	obj_to_cl.insert_crash(next_sequence_num);
	scheduler->process_crash();
	exit(EXIT_SUCCESS);
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
