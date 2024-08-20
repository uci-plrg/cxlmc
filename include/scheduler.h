#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include <atomic>

#include "allocators.h"
#include "threads.h"
#include "config.h"

//process local data
extern int process_id;
extern int thread_id;

class Scheduler {
    const int process_count;
    std::atomic_int thread_count;
    std::atomic_int active_thread;
    thread_data_t thread_data[MAX_THREADS];
public:
    Scheduler(int pc);
    void process_init(int pid) { process_id = pid; thread_id = pid; }
    
    int get_process_id() { return process_id; }

    int get_thread_id() { return thread_id; }

    int new_thread(void* (*func)(void*), void* arg);

    thread_data_t* get_thread(int tid) { return &thread_data[tid]; }

    void wait();
    
    void yield();
    
    //returns true if there are other running threads, else false
    bool yield_no_wait();

    //returns true if there are other running threads, else false
    bool finalize();

    void reset();
    
};
#endif
