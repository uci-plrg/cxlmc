#include <unistd.h>

#include "scheduler.h"

process_id_t process_id;
thread_id_t thread_id;

Scheduler::Scheduler(int pc):
    process_count(pc),
    active_thread(0) {
        for (int i = 0; i < pc; i++) {
            threads.push_back(new Thread(i));
        }
    }

thread_id_t Scheduler::new_thread(pthread_start_t func, void* arg) {
    thread_id_t tid = get_thread_count();
    printf("init thread %d\n", tid);
    threads.push_back(new Thread(tid, process_id, current_thread(), pthread_params{func, arg}));
    return tid;
}

void Scheduler::wait() {
    while (1) {
        thread_id_t active = active_thread.load();
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
    assert_active();

    thread_id_t active = active_thread.load();
    int tc = get_thread_count();
    for (int i = 1; i < tc; i++) {
        thread_id_t tid = (active + i) % tc;
        if (threads[tid]->get_state() == THREAD_RUNNING) {
            active_thread.store(tid);
            wait();
            return;
        }
    }
}

bool Scheduler::last_yield() {
    assert_active();
    
    thread_id_t active = active_thread.load();
    int tc = get_thread_count();
    for (int i = 1; i < tc; i++) {
        thread_id_t tid = (active + i) % tc;
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
        threads.push_back(new Thread(i));
    }

    active_thread.store(0);
}

void Scheduler::wake_threads_waiting_on(Thread* thread) {
    for (Thread* waiter: threads) {
        if (waiter->waiting_on() == thread) {
            waiter->set_state(THREAD_RUNNING);
        }
    }
}
