#include <unistd.h>

#include "scheduler.h"

void Scheduler::wait() {
    while (active.load() != process_id) {
        sleep(0);
    }
}

void Scheduler::yield() {
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
    wait();
    printf("%d done\n", process_id);
    process_status[process_id].store(1);
    yield();
}
