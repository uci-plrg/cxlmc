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

	//placeholder store buffer policy, to be changed later
	srand(42 + thread_id);
	bool to_flush = rand()%2;
	if (to_flush) 
		curr_thread->get_thread_memory()->pop_from_store_buffer();

    curr_thread->set_pending(action);
    scheduler->yield();
    execute(action);
    curr_thread->set_pending(nullptr);

	uint64_t val = action->get_value();
    delete action; 
	return val;
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

void Model::evict_clflush(ModelAction* action) {
    assert(action->get_type() == CACHE_CLFLUSH);
    insert_crash();
	modelclock_t seq_num = get_next_sequence_num();
	action->set_seq_num(seq_num);
	get_cacheline(action->get_location()).setBegin(seq_num);
}

void Model::build_may_read_from(ModelAction *read, shared::vector<ModelAction *> &rfset) {
	//TODO: handle load/store of varying sizes by checking overlap
	
	ModelAction *lastWrite = scheduler->get_thread(read->get_thread_id())->get_thread_memory()->get_last_write(read);
	if(lastWrite) {
		rfset.push_back(lastWrite);
		return;
	}

	storelist &stores = get_storelist(read->get_location());
	if (!stores.empty()) {
		rfset.push_back(stores.back());
		return;
	}

	//TODO: handle read from crashed processes
}

void Model::do_read(ModelAction * action, process_id_t write_pid, uint64_t value) {
	assert(action->get_type() == NONATOMIC_LOAD);
	Thread *reader_thread = scheduler->get_thread(action->get_thread_id());
	if (write_pid != reader_thread->get_process_id()) { 
		modelclock_t seq_num = get_next_sequence_num();
		get_cacheline(action->get_location()).setBegin(seq_num);
		//TODO: constraint for crashed processes
	}

	action->set_value(value);
}

void Model::print_execution_summary() {
        printf("stores: \n");
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

void Model::terminate_early() {
    rollback_again = false;
    finish_execution();
    _Exit(EXIT_FAILURE);
}

void Model::finish_execution() {
    for (int i = 0; i < scheduler->get_thread_count(); i++) {
        Thread* thread = scheduler->get_thread(i);
        if (thread->get_process_id() == process_id && !thread->is_completed()) {
            printf("thread %d terminated\n", i);
            if (is_fork)
                thread->cleanup();
            thread->set_state(THREAD_COMPLETED);
        }
    }

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
	crashed_processes[process_id] = get_next_sequence_num();
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
