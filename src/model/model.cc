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
	auto itr = obj_to_cacheline.try_emplace(id, id).first;
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
	modelclock_t seq_num = get_next_sequence_num();
	action->set_seq_num(seq_num);
	get_cacheline(action->get_location()).setBegin(seq_num);
}

void Model::print_execution_summary() {
        printf("stores: \n");
        for (auto &itr: obj_to_wr) {
			printf("aligned loc %p [", itr.first);
			for (auto s: itr.second)
				printf("loc: %p, val= %ld, seq=%u,", s->get_location(), s->get_value(), s->get_seq_num());
			printf("]\n");
		}
        printf("\n");

        printf("cachelines: \n");
		for (auto &pair: obj_to_cacheline)
			printf("%p: (%d, %d), ", pair.first, pair.second.getBegin(), pair.second.getEnd()); 
        printf("\n");

        printf("placeholder data: \n");
        for (auto &s: placeholder_data)
            printf("%s, ", s.c_str());
        printf("\n");
}

void Model::finish_execution() {
    bool isLast = !scheduler->finalize();

    int num = execution_num.load();
    
    printf("process %d done\n", process_id);
                    
    if (isLast) {
		print_execution_summary();
        if (num+1> MAX_EXECUTION)
            rollback = false;
        else {
            printf("-------------------------- execution %d--------------------------\n", num+1);
			reset_execution_data();
        }

        scheduler->reset();
        execution_num.store(num+1);
    } else {
        while (execution_num.load() != num+1) {
            sleep(0);
        }
    }

}

void Model::reset_execution_data() {
		memset(cxl_mapping, 0, CXL_MEM_SIZE);
		next_sequence_num = 0;
        obj_to_wr.clear();
        placeholder_data.clear();
		obj_to_cacheline.clear();
}
