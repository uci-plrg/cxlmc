#include <unistd.h>

#include "scheduler.h"

int process_id;

void Scheduler::wait_till_turn() {
    while (active.load() != process_id) {
        sleep(0);
    }
}

void Scheduler::give_next_turn() {
    for (int i = process_id + 1; i < process_count; i++) {
        if (!process_status[i].load()) {
            active.store(i);
            return;
        }
    }
    for (int i = 0; i < process_id; i++) {
        if (!process_status[i].load()) {
            active.store(i);
            return;
        }
    }
}

void Scheduler::done() {
    wait_till_turn();
    printf("%d done\n", process_id);
    process_status[process_id].store(1);
    give_next_turn();
}
