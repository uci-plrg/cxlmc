#ifndef _MODEL_H
#define _MODEL_H

#include <iostream>
#include <string>

#include "scheduler.h"
#include "shared_data.h"

class Model {
    Scheduler *scheduler;
    shared::vector<shared::string> user_data;

public:
    Model(Scheduler *s): scheduler(s) {}    

    void action(std::string s);

    Scheduler *get_scheduler() { return scheduler; }

    void print_user_data() {
        std::cout << "user data: ";
        for (auto s: user_data)
            std::cout << s << " ";
        std::cout << std::endl;
    }
};

void user_action(std::string s);

extern "C" {
    void user_init(int pid, Model *m, mspace ms);    
    
    void user_done();
}
#endif
