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

extern bool is_fork;

class Scheduler {
    const unsigned process_count;
    std::atomic<thread_id_t> active_thread;
    shared::vector<Thread*> threads;

public:
    Scheduler(int pc);
	~Scheduler() {
		for (Thread *t: threads)
			delete t;
	}
    void process_init(process_id_t pid) { process_id = pid; thread_id = pid; }
    
    process_id_t get_process_id() { return process_id; }

    thread_id_t get_thread_id() { return thread_id; }

    thread_id_t new_thread(pthread_start_t func, void* arg);

    Thread* get_thread(thread_id_t tid) { return threads[tid]; }
    
    unsigned get_thread_count() { return threads.size(); }
    
	unsigned get_process_count() { return process_count; }

    Thread* current_thread() { return threads[thread_id]; }

    void wake_all_threads_waiting_on(void* v);
    void wake_thread_waiting_on(void* v);

    void wait();
     
    void yield();

    //returns true if there are other running threads, else false
    bool last_yield();

    //returns true if there are other running threads, else false
    bool finalize();

	void process_shutdown();

	void process_crash();

    void reset();

    void assert_active() { assert(active_thread.load() == thread_id || !is_fork); }
    
};
#endif
