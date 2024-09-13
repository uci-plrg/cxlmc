#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <dlfcn.h>
#include "threads.h"
#include "model.h"

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

int pthread_mutex_init(pthread_mutex_t *p_mutex, const pthread_mutexattr_t * attr) {
	int mutex_type = PTHREAD_MUTEX_DEFAULT;
	if (attr != NULL)
		pthread_mutexattr_gettype(attr, &mutex_type);

	Mutex* m = new Mutex(mutex_type);

	model->get_mutex_map()->emplace(p_mutex, m);

	return 0;
}

int pthread_mutex_lock(pthread_mutex_t *p_mutex) {
	/* to protect the case where PTHREAD_MUTEX_INITIALIZER is used
	   instead of pthread_mutex_init, or where *p_mutex is not stored
	   in the execution->mutex_map for some reason. */
    auto mutex_map = model->get_mutex_map();
	if (mutex_map->find(p_mutex) == mutex_map->end()) {
		pthread_mutex_init(p_mutex, NULL);
	}

	Mutex* m = mutex_map->at(p_mutex);

	if (m != NULL) {
		m->lock();
	} else {
		return 1;
	}

	return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *p_mutex) {
	/* to protect the case where PTHREAD_MUTEX_INITIALIZER is used
	   instead of pthread_mutex_init, or where *p_mutex is not stored
	   in the execution->mutex_map for some reason. */
    auto mutex_map = model->get_mutex_map();
	if (mutex_map->find(p_mutex) == mutex_map->end()) {
		pthread_mutex_init(p_mutex, NULL);
	}

	Mutex* m = mutex_map->at(p_mutex);
	return m->try_lock() ? 0 : EBUSY;
}

int pthread_mutex_unlock(pthread_mutex_t *p_mutex) {
	Mutex* m = model->get_mutex_map()->at(p_mutex);

	if (m != NULL) {
		m->unlock();
	} else {
		printf("try to unlock an untracked pthread_mutex\n");
		return 1;
	}

	return 0;
}

int pthread_mutex_timedlock (pthread_mutex_t *__restrict p_mutex,
														 const struct timespec *__restrict abstime) {
    return pthread_mutex_lock(p_mutex);
}