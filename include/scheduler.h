#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include <atomic>
#include <assert.h>

#include "allocators.h"
#include "threads.h"
#include "config.h"
#include "shared_ADT.h"

//process local data
extern process_id_t process_id;
extern thread_id_t thread_id;

class Scheduler {
    const int process_count;
    std::atomic<thread_id_t> active_thread;
    shared::vector<Thread*> threads;

    //returns true if there are other running threads, else false
    bool last_yield();

public:
    Scheduler(int pc);
    void process_init(process_id_t pid) { process_id = pid; thread_id = pid; }
    
    process_id_t get_process_id() { return process_id; }

    thread_id_t get_thread_id() { return thread_id; }

    thread_id_t new_thread(pthread_start_t func, void* arg);

    Thread* get_thread(thread_id_t tid) { return threads[tid]; }

    int get_thread_count() { return threads.size(); }

    Thread* current_thread() { return threads[thread_id]; }

    void wake_threads_waiting_on(Thread* thread);

    void wait();
     
    void yield();

    //returns true if there are other running threads, else false
    bool finalize();

    void reset();

    void assert_active() { assert(active_thread.load() == thread_id); }
    
};
#endif
