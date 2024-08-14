#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include <atomic>

#include "allocators.h"
#include "thread_data.h"
#include "config.h"

//process local data
extern int process_id;
extern int thread_id;

class Scheduler {
    const int process_count;
    std::atomic_int thread_count;
    std::atomic_int active_thread;
    thread_data thread_status[MAX_THREADS];
public:
    Scheduler(int pc);

    void set_process_id(int pid) { process_id = pid; thread_id = pid; }
    
    int get_process_id() { return process_id; }

    int get_thread_id() { return thread_id; }

    void new_thread(void* (*func)(void*), void* arg);

    void wait();
    
    void yield();

    void finalize();
    
};
#endif
