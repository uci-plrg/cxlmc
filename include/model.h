#ifndef _MODEL_H
#define _MODEL_H

#include <iostream>
#include <string>

#include "scheduler.h"
#include "shared_data.h"

class Model {
    Scheduler *scheduler;
    std::atomic_int execution_num;
    bool rollback_again;
    shared::vector<shared::string> user_data;

public:
    Model(Scheduler *s): scheduler(s), execution_num(1), rollback_again(true) {}    

    void action(std::string s);
    
    void finishExecution();

    Scheduler *get_scheduler() { return scheduler; }

    bool should_rollback_again() { return rollback_again; }

    void print_user_data() {
        std::cout << "user data: ";
        for (auto s: user_data)
            std::cout << s << " ";
        std::cout << std::endl;
    }
};

#endif
