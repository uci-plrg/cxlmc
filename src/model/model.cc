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

void Model::action(ModelAction* action) {
    scheduler->assert_active();

    Thread* curr_thread = scheduler->current_thread();
    curr_thread->set_pending(action);
    scheduler->yield();
    execute(action);
    curr_thread->set_pending(nullptr);
    delete action; 

    curr_thread->get_thread_memory()->popFromStoreBuffer();
}

void Model::add_to_store_list(ModelAction* action) {
    assert(action->get_type() == STORE);
    store_list.push_back(action);
}

void Model::print_execution_summary() {
        printf("store list: \n");
        for (auto s: store_list)
            printf("loc: %p, val: %ld, ", s->get_location(), s->get_value());
        printf("\n");
        printf("placeholder data: \n");
        for (auto s: placeholder_data)
            printf("%s, ", s.c_str());
        printf("\n");
}

void Model::finishExecution() {
    bool isLast = !scheduler->finalize();

    int num = execution_num.load();
    
    printf("process %d done\n", process_id);
                    
    if (isLast) {
		print_execution_summary();
        if (num+1> MAX_EXECUTION)
            rollback_again = false;
        else {
            printf("-------------------------- execution %d--------------------------\n", num+1);
            store_list.clear();
            placeholder_data.clear();
        }

        scheduler->reset();
        execution_num.store(num+1);
    } else {
        while (execution_num.load() != num+1) {
            sleep(0);
        }
    }

}
