#ifndef _THREAD_INFO_H
#define _THREAD_INFO_H

#include <ucontext.h>
#include <atomic>
#include <pthread.h>

#include "action.h"
#include "shared_data.h"
#include "config.h"

typedef enum thread_state {
	THREAD_RUNNING,
	THREAD_COMPLETED
} thread_state;

class Thread {
    std::atomic_int thread_id;
    std::atomic_int process_id;
    std::atomic<thread_state> state;

    // process local
    void* stack;
    ucontext_t context;
public:
	void* ret_val;
	void* tls;
	pthread_mutex_t mutex_tls;
	pthread_mutex_t mutex_finalize;
	pthread_t pthread_id;

    Thread(int tid, int pid) : thread_id(tid), process_id(pid), state(THREAD_RUNNING), stack(nullptr), tls(nullptr) {}
    Thread(int pid) : Thread(pid, pid) {}

    int get_thread_id() { return thread_id.load(); }
    int get_process_id() { return process_id.load(); }
    thread_state get_state() { return state.load(); }
	void set_state(thread_state ts) { state.store(ts); }

	ucontext_t* get_context() { return &context; }
	void free_stack() { mspace_free(snapshot_space, stack); }
	void setup_tls();

    int setup_context(void* (*func)(void*), void* arg);
	void swap(Thread* thread);

    void * operator new(size_t size) {
		return mspace_malloc(shared_space, size);
	}
	void operator delete(void *p, size_t size) {
		mspace_free(shared_space, p);
	}
	void * operator new[](size_t size) {
		return mspace_malloc(shared_space, size);
	}
	void operator delete[](void *p, size_t size) {
		mspace_free(shared_space, p);
	}
};

typedef void *(*pthread_start_t)(void *);

struct pthread_params {
	pthread_start_t func;
	void *arg;
};

// int real_epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
int real_pthread_mutex_init(pthread_mutex_t *__mutex, const pthread_mutexattr_t *__mutexattr);
int real_pthread_mutex_lock (pthread_mutex_t *__mutex);
int real_pthread_mutex_unlock (pthread_mutex_t *__mutex);
int real_pthread_create (pthread_t *__restrict __newthread, const pthread_attr_t *__restrict __attr, void *(*__start_routine)(void *), void *__restrict __arg);
int real_pthread_join (pthread_t __th, void ** __thread_return);
void real_pthread_exit (void * value_ptr) __attribute__((noreturn));
void real_init_all();

#endif