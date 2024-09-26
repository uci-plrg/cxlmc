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

Model *model;
mspace shared_space;
mspace snapshot_space;

uint64_t Model::action(ModelAction* action) {
    scheduler->assert_active();
	if (is_crashed(process_id)) {
		scheduler->process_crash();
		exit(EXIT_SUCCESS);
	}

    Thread* curr_thread = scheduler->current_thread();
    curr_thread->set_pending(action);
    scheduler->yield();
    execute(action);
    curr_thread->set_pending(nullptr);

	uint64_t val = action->get_value();
    delete action; 

	process_store_buffer();
	return val;
}

bool Model::is_crashed(process_id_t pid) {
	return crashes.find(pid) != crashes.end();
}

void Model::process_store_buffer() {
	//placeholder store buffer policy, to be changed later
	uint num_to_pop = rand()%EVICT_MAX;
	uint thread_count = scheduler->get_thread_count();
	uint thread_to_pop = rand()%thread_count;
	for (uint i = 0; i < thread_count; i++) {
		if (!is_crashed(scheduler->get_thread(thread_to_pop)->get_process_id()))
			break;
		thread_to_pop = (thread_to_pop+1) % thread_count;
	}
	
	ThreadMemory *mem = scheduler->get_thread(thread_to_pop)->get_thread_memory();
	for (uint j = 0; j < num_to_pop; j++) {
		if (!mem->pop_from_store_buffer())
			break;
	}
}

process_id_t Model::get_process_id(ModelAction *action) { 
	return scheduler->get_thread(action->get_thread_id())->get_process_id(); 
}

void Model::do_read(rfEntry &e) {
	for (auto &pair: e.crashes)
		crashes.insert(pair);

	obj_to_cl[e.cl.getId()] = e.cl;
}

CacheLine &Model::get_cacheline(void *addr)  { 
	uintptr_t id = getCacheID(addr);
	return obj_to_cl.try_emplace(id, id).first->second;
}

Model::storeList &Model::get_storelist(void *addr)  { 
	void *aligned = alignAddress(addr);
	return obj_to_wr[aligned];
}

void Model::evict_store(ModelAction* action) {
    assert(action->get_type() == NONATOMIC_STORE);
	action->set_seq_num(get_next_sequence_num());
    get_storelist(action->get_location()).push_back(action);
}

void Model::evict_clflush(ModelAction* action) {
    assert(action->get_type() == CACHE_CLFLUSH || action->get_type() == CACHE_CLFLUSHOPT);
    insert_crash();
	modelclock_t seq_num = get_next_sequence_num();
	action->set_seq_num(seq_num);
	void *addr = action->get_location();
	CacheLine &cl = get_cacheline(addr);
	storeList &stores = get_storelist(addr);
	Range *r = &cl.get_current();
	if (modelclock_t clock = find_crashed_write(crashes, stores, stores.rbegin(), r->getBegin()) != 0)
		r = &cl.insert_range(clock, *r);
	r->setBegin(seq_num);
}

void Model::build_may_read_from(ModelAction *read, shared::vector<rfEntry> &rfset) {	
	CacheLine &cl = get_cacheline(read->get_location());
	uint numslotsleft = read->get_size();
	//optimize: maybe only store delta to cl
	rfEntry entry{cl, crashes, numslotsleft};

	if(scheduler->get_thread(read->get_thread_id())->get_thread_memory()->get_last_write(read, entry)) {
		rfset.push_back(entry);
		return;
	}

	storeList &stores = get_storelist(read->get_location());
	shared::vector<rfEntry> seedWrites;
	seedWrites.push_back(entry);
	process_id_t rpid = get_process_id(read);
		
	for (auto itr = stores.rbegin(); itr != stores.rend(); itr++) {
		ModelAction *store = *itr;
		process_id_t wpid = get_process_id(store);
		if (seedWrites.empty())
			break;

		for (uint i=0; i<seedWrites.size(); i++) {
			auto &w = seedWrites[i];
			auto citr = w.crashes.find(wpid);

			//running processes
			if (citr == w.crashes.end()) {
				rfEntry copy{w};
				if (w.get_overlaps(store, read)) {
					if (wpid != rpid) {
						w.cl.get_current().setBegin(next_sequence_num);
				
						//consider crashing the writing process
						if (!scheduler->get_thread(store->get_thread_id())->is_completed() && copy.crashes.size() < MAX_CRASHES_PER_EXECUTION) {
							copy.crashes.emplace(wpid, next_sequence_num);
							seedWrites.push_back(copy);
						}
					}
				}
			} else { //crashed processes
				modelclock_t crash_clock = citr->second;
				Range &r = w.cl.get_before(crash_clock);

				if (store->get_seq_num() <= r.getBegin()) { //must have persisted
					if (w.get_overlaps(store, read))
						read_crashed_update_cacheline(stores, itr, r, w);
				} else if (r.getEnd() == 0 || store->get_seq_num() <= r.getEnd()) { //may have persisted
					rfEntry copy(w);
					if (copy.get_overlaps(store, read)) {
						read_crashed_update_cacheline(stores, itr, copy.cl.get_before(crash_clock), copy);
						if (copy.numslotsleft == 0)
							rfset.push_back(copy);
						else
							seedWrites.push_back(copy);
					}
				}
			}
		}

		//move full seedWrites to rfset
		for (uint i = 0; i < seedWrites.size(); i++) {
			auto &w = seedWrites[i];
			if (w.numslotsleft == 0) {
				rfset.push_back(w);
				seedWrites[i] = seedWrites.back();
				seedWrites.pop_back();
			}
		}
	}
	for (auto w: seedWrites)
		rfset.push_back(w);
}


//inline?
modelclock_t Model::find_crashed_write(const shared::hashmap<process_id_t, modelclock_t> &curr_crashes, const storeList &stores, storeList::reverse_iterator itr, modelclock_t lb) {
	for (; itr != stores.rend(); itr++)  {
		if ((*itr)->get_seq_num() <= lb)
			return 0;
			
		auto citr = curr_crashes.find(get_process_id(*itr));
		if (citr != curr_crashes.end())
			return citr->second;
	}
	return 0;
}

//inline?
void Model::read_crashed_update_cacheline(const storeList &stores, storeList::reverse_iterator itr, Range &r, rfEntry &e) {
	assert(itr != stores.rend());
	ModelAction *write = *itr;

	modelclock_t write_seq = write->get_seq_num();
	Range *curr = &r;
	if (curr->getBegin() < write_seq) {
		if (modelclock_t clock = find_crashed_write(e.crashes, stores, itr, r.getBegin()) != 0)
			curr = &e.cl.insert_range(clock, r);
		curr->setBegin(write_seq);	
	}

	auto fitr = itr.base();
	uintptr_t wbot = (uintptr_t) write->get_location();
	uintptr_t wtop = wbot + write->get_size();
	//find next overlapping write
	for (; fitr != stores.end(); fitr++) {
		uintptr_t wbot2 = (uintptr_t) (*fitr)->get_location();
		uintptr_t wtop2 = wbot2 + (*fitr)->get_size();
		if (wtop > wbot2 && wbot < wtop2)
			break;
	}

	if (fitr != stores.end()) {
		modelclock_t next_write_seq = (*fitr)->get_seq_num();
		if (curr->getEnd() == 0 || curr->getEnd() >= next_write_seq)
			curr->setEnd(next_write_seq);
	}
}

void Model::print_execution_summary() {
		printf("\nExecution Summary\n");
        printf("stores: \n");
        for (auto &itr: obj_to_wr) {
			printf("aligned loc %p [", itr.first);
			for (auto s: itr.second) {
				int offset = (char *) s->get_location() - (char *) itr.first;
				printf("(+%d: val=%ld, seq=%u), ", offset, s->get_value(), s->get_seq_num());
			}
			printf("]\n");
		}
        printf("\n");

        printf("cachelines: \n");
		for (auto &pair: obj_to_cl) {
			printf("%p: {", (void *)pair.first);
			for (auto &cpair: pair.second.get_range_map())
				printf("(%d, %d), ", cpair.second.getBegin(), cpair.second.getEnd()); 
			printf("}\n\n");
		}	

		printf("crashed processes: \n");
		for (auto &pair: crashes)
			printf("p%d at %d, ", pair.first, pair.second);
		printf("\n\n");
        
		printf("placeholder data: \n");
        for (auto &s: placeholder_data)
            printf("%s, ", s.c_str());
        printf("\n");
}

void Model::terminate_early() {
    rollback_again = false;
    finish_execution();
    _Exit(EXIT_FAILURE);
}

void Model::finish_execution() {
    bool isLast = !scheduler->finalize();
    printf("process %d done\n", process_id);	
    int num = execution_num.load();

    if (isLast) {
		print_execution_summary();
        rollback_again = rollback_again && num+1 <= MAX_EXECUTION && nodestack->has_another_execution();
        if (rollback_again) {
            printf("-------------------------- execution %d--------------------------\n", num+1);
			reset_execution_data();
            nodestack->reset_execution();
        }

        scheduler->reset();
        execution_num.store(num+1);
    }
}

bool Model::wait_for_next_execution(int num) {
    if (num > MAX_EXECUTION || !rollback_again) {
        return false;
    }

    while (execution_num.load() < num) {
        sleep(0);
    }

    return rollback_again;
}

void Model::reset_execution_data() {
		memset(cxl_mapping, 0, CXL_MEM_SIZE);
		next_sequence_num = 0;
		for (auto itr: obj_to_wr)
			for (auto s: itr.second)
				delete s;
        obj_to_wr.clear();
        placeholder_data.clear();
		obj_to_cl.clear();
		crashes.clear();
}

bool Model::should_crash() {
    if (crashes.size() < MAX_CRASHES_PER_EXECUTION && decision_point(2) == 0) {
        return true;
    }
    return false;
}

void Model::insert_crash() {
    if (!should_crash()) {
        return;
    }
	
	crashes[process_id] = next_sequence_num;
	scheduler->process_crash();
	exit(EXIT_SUCCESS);
}
