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
mspace shared::shared_space;

void Model::action(std::string s) {
    action(new ModelAction(PLACEHOLDER, &s));
}

void Model::action(ModelAction* action) {
    scheduler->yield();
    execute(action);
    delete action; // probably will not do this later once we need to store it in the thread data
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
            user_data.clear();
        }

        scheduler->reset();
        execution_num.store(num+1);
    } else {
        while (execution_num.load() != num+1) {
            sleep(0);
        }
    }

}
