#ifndef _THREAD_INFO_H
#define _THREAD_INFO_H

#include <ucontext.h>
#include <atomic>

typedef enum thread_state {
	THREAD_RUNNING,
	THREAD_COMPLETED
} thread_state;

typedef struct thread_data {
    std::atomic_int thread_id;
    std::atomic_int process_id;
    std::atomic<thread_state> state;
    ucontext_t context;
} thread_data_t;

#endif