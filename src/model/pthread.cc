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

int pthread_detach(pthread_t t) {
	//Doesn't do anything
	//Return success
	return 0;
}

/* Take care of both pthread_yield and c++ thread yield */
int sched_yield() {
	model->action(new ModelAction(THREAD_YIELD));
	return 0;
}

void pthread_exit(void *value_ptr) {
	model->action(new ModelAction(THREADONLY_FINISH, value_ptr)); // does not return
    assert(0);
}

pthread_t pthread_self() {
    return thread_id;
}