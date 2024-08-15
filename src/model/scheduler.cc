#include <unistd.h>

#include "scheduler.h"

int process_id;
int thread_id;

Scheduler::Scheduler(int pc):
    process_count(pc),
    thread_count(pc),
    active_thread(0) {
        for (int i = 0; i < pc; i++) {
            thread_status[i].process_id = thread_status[i].thread_id = i;
            thread_status[i].state.store(THREAD_RUNNING);
        }
    }

void run_thread(Scheduler* scheduler, void* (*func)(void*), void* arg) {
    func(arg);
    scheduler->finalize();
    scheduler->wait(); // never return here
}

void Scheduler::new_thread(void* (*func)(void*), void* arg) {
    wait();
    int tc = thread_count.load();
    int tid = tc;
    for (int i = 0; i < tc; i++) {
        if (thread_status[i].state == THREAD_COMPLETED) {
            tid = i;
            break;
        }
    }
    printf("init thread %d\n", tid);
    thread_status[tid].process_id.store(process_id);
    thread_status[tid].thread_id.store(tc);
    thread_status[tid].state.store(THREAD_RUNNING);

    if (getcontext(&thread_status[tid].context) != 0) {
        perror("getcontext");
        return;
    }

    thread_status[tid].context.uc_link = nullptr;
    thread_status[tid].context.uc_stack.ss_sp = malloc(STACK_SIZE); // temp allocator
    thread_status[tid].context.uc_stack.ss_size = STACK_SIZE;
    thread_status[tid].context.uc_stack.ss_flags = 0;
    makecontext(&thread_status[tid].context, (void(*)()) run_thread, 3, this, func, arg);
    if (tid == tc) {
        thread_count.fetch_add(1);
    }
    yield();
    return;
}

void Scheduler::wait() {
    while (1) {
        int active = active_thread.load();
        if (thread_status[active].process_id == process_id) {
            if (active == thread_id) {
                break;
            }

            int prev_id = thread_id;
            thread_id = active;
            if (swapcontext(&thread_status[prev_id].context, &thread_status[thread_id].context) != 0) {
                perror("swapcontext");
            }
        }
        sleep(0);
    }
}

bool Scheduler::yield() {
    int active = active_thread.load();
    int tc = thread_count.load();
    for (int i = 1; i < tc; i++) {
        int tid = (active + i) % tc;
        if (thread_status[tid].state.load() == THREAD_RUNNING) {
            active_thread.store(tid);
            return true;
        }
    }
    
    return false;
}

bool Scheduler::finalize() {
    wait();
    printf("thread %d done\n", thread_id);
    thread_status[thread_id].state.store(THREAD_COMPLETED);

    //if(thread_status[thread_id].context.uc_stack.ss_sp) {
    //    free(thread_status[thread_id].context.uc_stack.ss_sp);
    //} 
    return yield();
}

void Scheduler::reset() {
    thread_count = process_count;
    active_thread = 0;
    for (int i = 0; i < process_count; i++) {
        thread_status[i].process_id = thread_status[i].thread_id = i;
        thread_status[i].state.store(THREAD_RUNNING);
    }
}
