#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <string>

#include "shared_mem.h"

int thread_id = 0;

void wait_till_turn() {
    while (sd->active.load() != thread_id) {
        sleep(0);
    }
}

void give_next_turn() {
    for (int i = thread_id + 1; i < sd->thread_count; i++) {
        if (!sd->thread_status[i].load()) {
            sd->active.store(i);
        }
    }
    for (int i = 0; i < thread_id; i++) {
        if (!sd->thread_status[i].load()) {
            sd->active.store(i);
        }
    }
}

void thing(std::string s) {
    wait_till_turn();
    std::cout << "thread " << thread_id << ", " << s << std::endl;
    sd->user_strings.push_back(shared::string(s.c_str()));
    give_next_turn();
}

extern "C" {
    void fork_init(int id, shared_data_t* d, mspace ms) {
        thread_id = id;
        sd = d;
        shared_space = ms;
    }

    void done() {
        wait_till_turn();
        printf("%d done\n", thread_id);
        sd->thread_status[thread_id].store(1);
        give_next_turn();
    }
}
