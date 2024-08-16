#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <dlfcn.h>
#include "threads.h"
#include "user.h"

int pthread_create(pthread_t *__restrict __newthread, const pthread_attr_t *__restrict __attr, void *(*__start_routine)(void *), void *__restrict __arg) {
    *__newthread = model->get_scheduler()->new_thread(__start_routine, __arg);
    return 0;
}

int pthread_join(pthread_t __th, void ** __thread_return) {
    Scheduler* scheduler = model->get_scheduler();
    thread_data_t* thread = scheduler->get_thread(__th);
    scheduler->wait();
    printf("%d called pthread_join %d\n", thread_id, __th);
    while (thread->state.load() == THREAD_RUNNING) {
        printf("%d still joining %d\n", thread_id, __th);
        scheduler->yield();
        scheduler->wait();
    }
	return 0;
}