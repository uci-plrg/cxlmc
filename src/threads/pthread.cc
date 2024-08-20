#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <dlfcn.h>
#include "threads.h"
#include "user.h"

int pthread_create(pthread_t *__restrict __newthread, const pthread_attr_t *__restrict __attr, void *(*__start_routine)(void *), void *__restrict __arg) {
    int new_tid;
    struct pthread_params params{__start_routine, __arg};
    model->action(new ModelAction(PTHREAD_CREATE, &params, &new_tid));
    *__newthread = new_tid;
    // need error checking
    return 0;
}

int pthread_join(pthread_t __th, void ** __thread_return) {
    // doesn't take return value yet
    model->action(new ModelAction(PTHREAD_JOIN, &__th));
	return 0;
}