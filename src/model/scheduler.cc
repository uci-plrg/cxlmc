#include <unistd.h>

#include "scheduler.h"

int process_id;
int thread_id;

Scheduler::Scheduler(int pc):
    process_count(pc),
    thread_count(pc),
    active_thread(0) {
        for (int i = 0; i < pc; i++) {
            thread_data[i].process_id = thread_data[i].thread_id = i;
            thread_data[i].state.store(THREAD_RUNNING);
        }
    }

void run_thread(Scheduler* scheduler, void* (*func)(void*), void* arg) {
    func(arg);
    scheduler->finalize();
    scheduler->wait(); // never return here
}

int Scheduler::new_thread(void* (*func)(void*), void* arg) {
    wait();
    int tid = thread_count.load();
    printf("init thread %d\n", tid);
    thread_data[tid].process_id.store(process_id);
    thread_data[tid].thread_id.store(tid);
    thread_data[tid].state.store(THREAD_RUNNING);

    if (getcontext(&thread_data[tid].context) != 0) {
        perror("getcontext");
        return -1;
    }

    thread_data[tid].context.uc_link = nullptr;
    thread_data[tid].context.uc_stack.ss_sp = malloc(STACK_SIZE); // temp allocator
    thread_data[tid].context.uc_stack.ss_size = STACK_SIZE;
    thread_data[tid].context.uc_stack.ss_flags = 0;
    makecontext(&thread_data[tid].context, (void(*)()) run_thread, 3, this, func, arg);
    thread_count.fetch_add(1);
    yield();
    return tid;
}

void Scheduler::wait() {
    while (1) {
        int active = active_thread.load();
        if (thread_data[active].process_id == process_id) {
            if (active == thread_id) {
                break;
            }

            int prev_id = thread_id;
            thread_id = active;
            if (swapcontext(&thread_data[prev_id].context, &thread_data[thread_id].context) != 0) {
                perror("swapcontext");
            }
        }
        sleep(0);
    }
}

void Scheduler::yield() {
    int active = active_thread.load();
    int tc = thread_count.load();
    for (int i = 1; i < tc; i++) {
        int tid = (active + i) % tc;
        if (thread_data[tid].state.load() == THREAD_RUNNING) {
            active_thread.store(tid);
            return;
        }
    }
}

void Scheduler::finalize() {
    wait();
    printf("%d done\n", thread_id);
    thread_data[thread_id].state.store(THREAD_COMPLETED);
    yield();
}
