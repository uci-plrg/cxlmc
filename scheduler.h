#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include <atomic>

#include "allocators.h"

//process local data
extern int process_id;

class Scheduler {
    int process_count;
    std::atomic_int active;
    std::atomic_int *process_status;
public:
    Scheduler(int pc): 
        process_count(pc),
        process_status((std::atomic_int*)mspace_calloc(shared::shared_space, pc, sizeof(std::atomic_int))) {}
 
    int get_process_id() { return process_id; }

    void wait();
    
    void yield();

    void done();
    
};
#endif
