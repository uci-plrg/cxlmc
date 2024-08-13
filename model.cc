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

mspace shared::shared_space;
Scheduler *scheduler;

shared::vector<shared::string> *user_data;

void model_init(int process_id, Scheduler *s, mspace ms, shared::vector<shared::string> *us) {
    shared::shared_space = ms;
    scheduler = s;
    scheduler->set_process_id(process_id);
    user_data = us;
}
    
void action(std::string s) {
    scheduler->wait_till_turn();
    std::cout << "process " << scheduler->get_process_id() << ", " << s << std::endl;
    user_data->push_back(shared::string(s.c_str()));
    scheduler->give_next_turn();
}

void model_done() {
    scheduler->done(); 
}
