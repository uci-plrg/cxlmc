#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <dlfcn.h>
#include "threads.h"
#include "user.h"

int pthread_create(pthread_t* tid, const pthread_attr_t* attr, pthread_start_t func, void* arg) {
    struct pthread_params params{func, arg};
    model->action(new ModelAction(PTHREAD_CREATE, tid, (uint64_t)&params));
    return 0;
}

int pthread_join(pthread_t tid, void** ret_val) {
    Thread* thread = model->get_scheduler()->get_thread(tid);
    model->action(new ModelAction(PTHREAD_JOIN, thread, tid));
    if (ret_val) {
        *ret_val = thread->ret_val;
    }
	return 0;
}