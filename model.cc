#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <string>

#include "model.h"
#include "shared_data.h"

mspace shared::shared_space;
int process_id;
shared_data_t *sd;

void wait_till_turn() {
    while (sd->active.load() != process_id) {
        sleep(0);
    }
}

void give_next_turn() {
    for (int i = process_id + 1; i < sd->process_count; i++) {
        if (!sd->process_status[i].load()) {
            sd->active.store(i);
            return;
        }
    }
    for (int i = 0; i < process_id; i++) {
        if (!sd->process_status[i].load()) {
            sd->active.store(i);
            return;
        }
    }
}

void fork_init(int id, shared_data_t* d, mspace ms) {
    process_id = id;
    sd = d;
    shared::shared_space = ms;
}

void done() {
    wait_till_turn();
    printf("%d done\n", process_id);
    sd->process_status[process_id].store(1);
    give_next_turn();
}
    
void action(std::string s) {
    wait_till_turn();
    std::cout << "process " << process_id << ", " << s << std::endl;
    sd->user_strings.push_back(shared::string(s.c_str()));
    give_next_turn();
}
