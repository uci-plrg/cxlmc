#ifndef _THREAD_INFO_H
#define _THREAD_INFO_H

#include <ucontext.h>
#include <pthread.h>

#include "action.h"
#include "shared_ADT.h"
#include "thread_memory.h"
#include "config.h"

typedef void *(*pthread_start_t)(void *);

struct pthread_params {
	pthread_start_t func;
	void *arg;
};

typedef enum thread_state {
	THREAD_RUNNING,
	THREAD_BLOCKED,
	THREAD_COMPLETED,
	THREAD_CRASHED,
} thread_state;

class Thread {
    thread_id_t thread_id;
    process_id_t process_id;
    thread_state state;
	Thread* parent;
	bool is_main;
	ModelAction* pending;
    ThreadMemory thread_memory;

    // process local
    void* stack;
    ucontext_t context;
public:
	pthread_params params;
	void* ret_val;
	void* tls;
	pthread_mutex_t mutex_tls;
	pthread_mutex_t mutex_finalize;
	pthread_t pthread_id;

    Thread(thread_id_t tid, process_id_t pid, Thread* par, pthread_params p);
    Thread(process_id_t pid); // create main thread

    thread_id_t get_thread_id() { return thread_id; }
    process_id_t get_process_id() { return process_id; }
    thread_state get_state() { return state; }
	void set_state(thread_state ts) { state = ts; }

	ucontext_t* get_context() { return &context; }
	ThreadMemory* get_thread_memory() { return &thread_memory; }
	void cleanup();
	void finalize();

    int setup_context();
	void swap(Thread* thread);

	ModelAction* get_pending() { return pending; }
	void set_pending(ModelAction* action) { pending = action; }

	void* waiting_on();

	bool is_completed() { return state == THREAD_COMPLETED || state == THREAD_CRASHED; }

    MODELALLOC
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
