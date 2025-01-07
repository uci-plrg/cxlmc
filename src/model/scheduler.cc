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
        real_sched_yield();
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

    if (threads[active]->get_state() != THREAD_RUNNING) {
        printf("DEADLOCK\n");
        abort();
    }
}

bool Scheduler::last_yield() {
    assert_active();
    bool threads_blocked = false;
    
    thread_id_t active = active_thread.load();
    int tc = get_thread_count();
    for (int i = 1; i < tc; i++) {
        thread_id_t tid = (active + i) % tc;
        if (threads[tid]->get_state() == THREAD_RUNNING) {
            active_thread.store(tid);
            return true;
        }

        threads_blocked = threads_blocked || threads[tid]->get_state() == THREAD_BLOCKED;
    }

    if (threads_blocked) {
        printf("DEADLOCK\n");
        abort();
    }

    return false;
}

bool Scheduler::finalize() {
    if (!threads[thread_id]->is_completed()) {
        printf("thread %d done\n", thread_id);
        threads[thread_id]->set_state(THREAD_COMPLETED);
    }
    return last_yield();
}

void Scheduler::process_shutdown() {
	//TODO: make into an action?
	yield();
	for (thread_id_t i = 0; i < get_thread_count(); i++) {
        Thread* thread = get_thread(i);
        if (thread->get_process_id() == process_id) {
			if (!thread->is_completed()) {
				printf("thread %d terminated\n", i);
				if (is_fork)
					thread->cleanup();
				thread->set_state(THREAD_COMPLETED);
			}
			thread->get_thread_memory()->empty_store_buffer();
			thread->get_thread_memory()->empty_flush_buffer();
        }
    }
}

void Scheduler::process_crash() {
	for (thread_id_t i = 0; i < get_thread_count(); i++) {
	    Thread* thread = get_thread(i);
	    if (thread->get_process_id() == process_id && !thread->is_completed()) {
	        printf("thread %d crashed\n", i);
	        thread->cleanup();
	        thread->set_state(THREAD_CRASHED);
	    }
	}
}

void Scheduler::reset() {
    for (Thread* thread: threads) {
        delete thread;
    }
    threads.clear();
    for (process_id_t i = 0; i < process_count; i++) {
        threads.push_back(new Thread(i));
    }

    active_thread.store(0);
}

void Scheduler::wake_all_threads_waiting_on(void* v) {
    for (Thread* waiter: threads) {
        if (waiter->waiting_on() == v) {
            printf("waking thread %d\n", waiter->get_thread_id());
            waiter->set_state(THREAD_RUNNING);
        }
    }
}

void Scheduler::wake_thread_waiting_on(void* v) { // should be random?
    int tc = get_thread_count();
    for (int i = 1; i < tc; i++) {
        thread_id_t tid = (thread_id + i) % tc;
        if (threads[tid]->waiting_on() == v) {
            printf("waking thread %d\n", tid);
            threads[tid]->set_state(THREAD_RUNNING);
            return;
        }
    }
}
