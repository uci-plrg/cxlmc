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

//process local data
mspace shared::shared_space;
Model *model;
int process_id;
    
void Model::action(std::string s) {
    scheduler->wait();
    std::cout << "process " << scheduler->get_process_id() << ", " << s << std::endl;
    user_data.push_back(shared::string(s.c_str()));
    scheduler->yield();
}

void user_action(std::string s) {
    model->action(s);
}

void user_init(int pid, Model *m, mspace ms) {
    model = m;    
    shared::shared_space = ms;
    process_id = pid;
}

void user_done() {
    model->get_scheduler()->done(); 
}
