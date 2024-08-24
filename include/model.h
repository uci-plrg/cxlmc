#ifndef _MODEL_H
#define _MODEL_H

#include <iostream>
#include <string>

#include "scheduler.h"
#include "shared_data.h"
#include "action.h"

class Model {
    Scheduler *scheduler;
    std::atomic_int execution_num;
    bool rollback_again;
    shared::vector<shared::string> placeholder_data;
    shared::list<ModelAction *> store_list;

public:
    Model(Scheduler *s): scheduler(s), execution_num(1), rollback_again(true) {}    

    void action(std::string s);
    void action(ModelAction* action);
    
    void add_to_store_list(ModelAction* action);
    
    void finishExecution();

    Scheduler *get_scheduler() { return scheduler; }

    bool should_rollback_again() { return rollback_again; }

    void print_placeholder_data() {
        std::cout << "placeholder data: ";
        for (auto s: placeholder_data)
            std::cout << s << " ";
        std::cout << std::endl;
    }

    shared::vector<shared::string> &get_placeholder_data() { return placeholder_data; };
};

extern Model *model;

#endif
