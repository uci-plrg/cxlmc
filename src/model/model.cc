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

Model *model;
mspace shared::shared_space;

void Model::action(std::string s) {
    scheduler->wait();
    std::cout << "process " << scheduler->get_process_id() << ", " << "thread " << scheduler->get_thread_id() << ", " << s << std::endl;
    user_data.push_back(shared::string(s.c_str()));
    scheduler->yield();
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
