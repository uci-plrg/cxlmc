#ifndef _THREAD_INFO_H
#define _THREAD_INFO_H

#include <ucontext.h>
#include <atomic>
#include <pthread.h>

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

// int real_epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
int real_pthread_mutex_init(pthread_mutex_t *__mutex, const pthread_mutexattr_t *__mutexattr);
int real_pthread_mutex_lock (pthread_mutex_t *__mutex);
int real_pthread_mutex_unlock (pthread_mutex_t *__mutex);
int real_pthread_create (pthread_t *__restrict __newthread, const pthread_attr_t *__restrict __attr, void *(*__start_routine)(void *), void *__restrict __arg);
int real_pthread_join (pthread_t __th, void ** __thread_return);
void real_pthread_exit (void * value_ptr) __attribute__((noreturn));
void real_init_all();

#endif