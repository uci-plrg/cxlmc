#include <unistd.h>

#include "scheduler.h"

int process_id;
int thread_id;

Scheduler::Scheduler(int pc):
    process_count(pc),
    thread_count(pc),
    active_thread(0) {
        for (int i = 0; i < pc; i++) {
            Thread* thread = new Thread(i);
            thread->setup_tls();
            threads.push_back(thread);
        }
    }

int Scheduler::new_thread(void* (*func)(void*), void* arg) {
    int tid = thread_count.load();
    printf("init thread %d\n", tid);
    Thread* new_thread = new Thread(tid, process_id);
    new_thread->setup_context(func, arg);
    threads.push_back(new_thread);
    thread_count.fetch_add(1);
    return tid;
}

void Scheduler::wait() {
    while (1) {
        int active = active_thread.load();
        if (threads[active]->get_process_id() == process_id) {
            if (active == thread_id) {
                break;
            }
            Thread* prev = current_thread();
            thread_id = active;
            prev->swap(get_thread(active));
        }
        sleep(0);
    }
}

void Scheduler::yield() {
    int active = active_thread.load();
    int tc = thread_count.load();
    for (int i = 1; i < tc; i++) {
        int tid = (active + i) % tc;
        if (threads[tid]->get_state() == THREAD_RUNNING) {
            active_thread.store(tid);
            wait();
        }
    }
}

bool Scheduler::last_yield() {
    int active = active_thread.load();
    int tc = thread_count.load();
    for (int i = 1; i < tc; i++) {
        int tid = (active + i) % tc;
        if (threads[tid]->get_state() == THREAD_RUNNING) {
            active_thread.store(tid);
            return true;
        }
    }

    return false;
}

bool Scheduler::finalize() {
    printf("thread %d done\n", thread_id);
    threads[thread_id]->set_state(THREAD_COMPLETED);
    return last_yield();
}

void Scheduler::reset() {
    for (Thread* thread: threads) {
        delete thread;
    }
    threads.clear();
    for (int i = 0; i < process_count; i++) {
        Thread* thread = new Thread(i);
        thread->setup_tls();
        threads.push_back(thread);
    }

    thread_count.store(process_count);
    active_thread.store(0);
}
