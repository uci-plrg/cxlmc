#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <string>

#include "model.h"
#include "scheduler.h"
#include "executor.h"

Model *model;
mspace shared_space;
mspace snapshot_space;

void Model::action(ModelAction* action) {
    Thread* curr_thread = scheduler->current_thread();
    curr_thread->set_pending(action);
    scheduler->yield();
    execute(action);
    curr_thread->set_pending(nullptr);
    delete action; 
}

void Model::add_to_store_list(ModelAction* action) {
    assert(action->get_type() == STORE);
    store_list.push_back(action);
}

void Model::finishExecution() {
    bool isLast = !scheduler->finalize();

    int num = execution_num.load();
    
    std::cout << "process " << process_id << " done" << std::endl;
                    
    if (isLast) {
        if (num+1> MAX_EXECUTION)
            rollback_again = false;
        else {
            std::cout << "-------------------------- execution " << num+1 << "--------------------------" << std::endl;
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
