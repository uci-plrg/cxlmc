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
	return crashed_processes.find(pid) != crashed_processes.end();
}

void Model::process_store_buffer() {
	//placeholder store buffer policy, to be changed later
	uint num_to_pop = rand()%EVICT_MAX;
	uint thread_count = scheduler->get_thread_count();
	uint thread_to_pop = rand()%thread_count;
	uint i = 0;
	printf("thread_count %d, thread_to_pop %d\n", thread_count, thread_to_pop);
	while(is_crashed(scheduler->get_thread(thread_to_pop)->get_process_id())) {
		thread_to_pop = (thread_to_pop+1) % thread_count;
		i++;
		if (i == thread_count)
			return; 
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

void Model::set_cacheline(CacheLine &cl) {
	obj_to_cl[cl.getId()] = cl;
}

CacheLine &Model::get_cacheline(void *addr)  { 
	uintptr_t id = getCacheID(addr);
	auto itr = obj_to_cl.try_emplace(id, id).first;
	return itr->second; 
}

Model::storelist &Model::get_storelist(void *addr)  { 
	void *aligned = alignAddress(addr);
	auto itr = obj_to_wr.try_emplace(aligned).first;
	return itr->second; 
}

void Model::evict_store(ModelAction* action) {
    assert(action->get_type() == NONATOMIC_STORE);
	action->set_seq_num(get_next_sequence_num());
    get_storelist(action->get_location()).push_back(action);
}

bool Model::has_unflushed_write(void *addr, process_id_t pid) {
	storelist &stores = get_storelist(addr);
	modelclock_t cl_begin = get_cacheline(addr).getBegin();
	for (auto itr = stores.rbegin(); itr != stores.rend(); itr++) {
		ModelAction *store = *itr;
		if (store->get_seq_num() <= cl_begin)
			break;
		if (get_process_id(store) == pid)
			return true;
	}

	return false;
}

void Model::evict_clflush(ModelAction* action) {
    assert(action->get_type() == CACHE_CLFLUSH || action->get_type() == CACHE_CLFLUSHOPT);
    insert_crash();
	modelclock_t seq_num = get_next_sequence_num();
	action->set_seq_num(seq_num);
	void *addr = action->get_location();
	get_cacheline(addr).setBegin(seq_num);
}

void Model::build_may_read_from(ModelAction *read, shared::vector<rfEntry> &rfset) {	
	CacheLine &cl = get_cacheline(read->get_location());
	
	uint numslotsleft = read->get_size();
	shared::vector<ModelAction *> overlaps(numslotsleft);
	if(scheduler->get_thread(read->get_thread_id())->get_thread_memory()->get_last_write(read, overlaps, numslotsleft)) {
		rfset.push_back(rfEntry{overlaps, cl, numslotsleft, false});
		return;
	}

	storelist &stores = get_storelist(read->get_location());
	shared::vector<rfEntry> seedWrites;
	seedWrites.push_back(rfEntry{overlaps, cl, numslotsleft, false});

	for (auto itr = stores.rbegin(); itr != stores.rend(); itr++) {
		ModelAction *store = *itr;
		if (seedWrites.empty())
			break;

		for (auto &w: seedWrites) {
			uint &seedSlotsleft = w.numslotsleft;
			auto &seedOverlaps = w.overlaps;
			CacheLine &seedCL = w.cl;
			bool &seedShouldCrash = w.shouldCrash;
		
			//running processes
			if (!is_crashed(get_process_id(store))) {
				if (get_overlaps(seedOverlaps, store, read, seedSlotsleft))
					do_read(read, store, seedCL, seedShouldCrash);
			} else { //crashed processes
				if (store->get_seq_num() <= seedCL.getBegin()) { //must have persisted
					if (get_overlaps(seedOverlaps, store, read, seedSlotsleft))
						do_read(read, store, seedCL, seedShouldCrash);
				} else if (seedCL.getEnd() == 0 || store->get_seq_num() <= seedCL.getEnd()) { //may have persisted
					rfEntry copy(w);
					if (get_overlaps(copy.overlaps, store, read, copy.numslotsleft)) {
						do_read(read, store, copy.cl, copy.shouldCrash);
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

void Model::do_read(ModelAction *read, ModelAction *write, CacheLine &cl, bool &shouldCrash) {
	assert(read->get_type() == NONATOMIC_LOAD);
	
	modelclock_t write_pid = get_process_id(write);
	modelclock_t read_pid = get_process_id(read);

	if (is_crashed(write_pid)) {
		modelclock_t write_seq = write->get_seq_num();
		if (cl.getBegin() < write_seq)
			cl.setBegin(write_seq);
		storelist &stores = get_storelist(read->get_location());
		
		storelist::const_iterator itr; 
		for (itr = stores.begin(); itr != stores.end(); itr++) 
			if (*itr == write)
				break;
		assert(itr != stores.end());
		itr++;
		if (itr != stores.end()) {
			modelclock_t next_write_seq = (*itr)->get_seq_num();
			if (cl.getEnd() == 0 || cl.getEnd() >= next_write_seq)
				cl.setEnd(next_write_seq);
		}
	}
	else if (write_pid != read_pid) { 
		modelclock_t seq_num = get_next_sequence_num();
		cl.setBegin(seq_num);
		shouldCrash = true;
	}
}

void Model::print_execution_summary() {
        printf("\nstores: \n");
        for (auto &itr: obj_to_wr) {
			printf("aligned loc %p [", itr.first);
			for (auto s: itr.second) {
				int offset = (char *) s->get_location() - (char *) itr.first;
				printf("+%d: val=%ld, seq=%u, ", offset, s->get_value(), s->get_seq_num());
			}
			printf("]\n");
		}
        printf("\n");

        printf("cachelines: \n");
		for (auto &pair: obj_to_cl)
			printf("%p: (%d, %d), ", (void *)pair.first, pair.second.getBegin(), pair.second.getEnd()); 
        printf("\n\n");
		
		for (auto &cpair: crashed_processes) {
			printf("cachelines for crashed process %u: \n", cpair.first);
			for (auto &pair: cpair.second)
				printf("%p: (%d, %d), ", (void *)pair.first, pair.second.getBegin(), pair.second.getEnd()); 
        	printf("\n\n");
		}
        
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

    int num = execution_num.load();

    bool isLast = !scheduler->finalize();
    printf("process %d done\n", process_id);	

    if (isLast) {
		print_execution_summary();
        rollback_again = rollback_again && num+1 <= MAX_EXECUTION && nodestack->has_another_execution();
        if (rollback_again) {
            printf("-------------------------- execution %d--------------------------\n", num+1);
			reset_execution_data();
            nodestack->reset_execution();
        }

        crash_count = 0;
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
		crashed_processes.clear();
}

void Model::execute_crash() {
    for (int i = 0; i < scheduler->get_thread_count(); i++) {
        Thread* thread = scheduler->get_thread(i);
        if (thread->get_process_id() == process_id && !thread->is_completed()) {
            printf("thread %d crashed\n", i);
            thread->cleanup();
            thread->set_state(THREAD_CRASHED);
        }
    }
	cachelinemap &map = crashed_processes[process_id];
	for (auto &pair: obj_to_cl)
		if (has_unflushed_write((void*)pair.first, process_id))
			map[pair.first] = pair.second;

    exit(EXIT_SUCCESS);
}

bool Model::should_crash() {
    if (crash_count < MAX_CRASHES_PER_EXECUTION && decision_point(2) == 0) {
        crash_count++;
        return true;
    }
    return false;
}

void Model::insert_crash() {
    if (!should_crash()) {
        return;
    }
    execute_crash();
}
