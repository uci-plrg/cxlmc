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

	//placeholder store buffer policy, to be changed later
	bool to_flush = rand()%2;
	if (to_flush) 
		curr_thread->get_thread_memory()->pop_from_store_buffer();

	return val;
}

process_id_t Model::get_process_id(ModelAction *action) { 
	return scheduler->get_thread(action->get_thread_id())->get_process_id(); 
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

bool Model::has_postcrash_unflushed_write(void *addr) {
	storelist &stores = get_storelist(addr);
	modelclock_t cl_begin = get_cacheline(addr).getBegin();
	for (auto itr = stores.rbegin(); itr != stores.rend(); itr++) {
		ModelAction *store = *itr;
		if (store->get_seq_num() <= cl_begin)
			break;
		if (crashed_processes.find(get_process_id(store)) == crashed_processes.end())
			return true;
	}

	return false;
}

void Model::evict_clflush(ModelAction* action) {
    assert(action->get_type() == CACHE_CLFLUSH);
	modelclock_t seq_num = get_next_sequence_num();
	action->set_seq_num(seq_num);
	void *addr = action->get_location();
	//only update the flush range when there are writes that can be flushed
	if (has_postcrash_unflushed_write(addr))
		get_cacheline(addr).setBegin(seq_num);
}

void Model::build_may_read_from(ModelAction *read, shared::vector<ModelAction *> &rfset) {
	//TODO: handle load/store of varying sizes by checking overlap
	
	ModelAction *lastWrite = scheduler->get_thread(read->get_thread_id())->get_thread_memory()->get_last_write(read);
	if(lastWrite) {
		rfset.push_back(lastWrite);
		return;
	}

	storelist &stores = get_storelist(read->get_location());
	CacheLine &cl = get_cacheline(read->get_location());

	for (auto itr = stores.rbegin(); itr != stores.rend(); itr++) {
		//running processes
		ModelAction *store = *itr;
		if (crashed_processes.find(get_process_id(store)) == crashed_processes.end()) {
			rfset.push_back(store);
			return;
		} else { //crashed processes
			if (store->get_seq_num() <= cl.getBegin()) {
				rfset.push_back(store);
				return;
			} else if (cl.getEnd() == 0 || store->get_seq_num() <= cl.getEnd())
				rfset.push_back(store);
		}
	}
}

void Model::do_read(ModelAction * read, ModelAction *write) {
	assert(read->get_type() == NONATOMIC_LOAD);
	if (!write) {
		read->set_value(VALUE_NONE);
		return;
	}

	modelclock_t write_pid = get_process_id(write);
	modelclock_t read_pid = get_process_id(read);
	CacheLine &cl = get_cacheline(read->get_location());

	if (crashed_processes.find(write_pid) != crashed_processes.end()) {
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
	}

	read->set_value(write->get_value());
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
			printf("%p: (%d, %d), ", pair.first, pair.second.getBegin(), pair.second.getEnd()); 
        printf("\n\n");

        printf("placeholder data: \n");
        for (auto &s: placeholder_data)
            printf("%s, ", s.c_str());
        printf("\n");
}

void Model::finish_execution() {
    // TODO check if other threads of this process is still running

    bool isLast = !scheduler->finalize();

    int num = execution_num.load();
    
    printf("process %d done\n", process_id);
                    
    if (isLast) {
		print_execution_summary();
        rollback_again = num+1 <= MAX_EXECUTION && nodestack->has_another_execution();
        if (rollback_again) {
            printf("-------------------------- execution %d--------------------------\n", num+1);
			reset_execution_data();
            nodestack->reset_execution();
        }

        scheduler->reset();
        execution_num.store(num+1);
    }
    exit(EXIT_SUCCESS);
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
        if (thread->get_process_id() == process_id) {
            printf("thread %d crashed\n", i);
            thread->set_state(THREAD_CRASHED);
        }
    }
	crashed_processes[process_id] = get_next_sequence_num();
    finish_execution();
}

void Model::insert_crash() {
    if (!should_crash()) {
        return;
    }
    execute_crash();
}
