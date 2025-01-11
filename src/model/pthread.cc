#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <dlfcn.h>
#include <sched.h>
#include "threads.h"
#include "model.h"
#include "condition_variable.h"

inline ExtPtr make_ext_ptr(void *ptr) {
	return ExtPtr{ptr, model->mem_is_cxl(ptr) ? -1: process_id};
}

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
	ExtPtr ep = make_ext_ptr(p_mutex);
	if (attr != NULL)
		pthread_mutexattr_gettype(attr, &mutex_type);

	Mutex* m = new Mutex(mutex_type);

	model->get_mutex_map()->emplace(ep, m);

	return 0;
}

int pthread_mutex_lock(pthread_mutex_t *p_mutex) {
	/* to protect the case where PTHREAD_MUTEX_INITIALIZER is used
	   instead of pthread_mutex_init, or where *p_mutex is not stored
	   in the execution->mutex_map for some reason. */
	ExtPtr ep = make_ext_ptr(p_mutex);
    auto mutex_map = model->get_mutex_map();
	if (mutex_map->find(ep) == mutex_map->end()) {
		pthread_mutex_init(p_mutex, NULL);
	}

	Mutex* m = mutex_map->at(ep);

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
	ExtPtr ep = make_ext_ptr(p_mutex);
    auto mutex_map = model->get_mutex_map();
	if (mutex_map->find(ep) == mutex_map->end()) {
		pthread_mutex_init(p_mutex, NULL);
	}

	Mutex* m = mutex_map->at(ep);
	return m->try_lock() ? 0 : EBUSY;
}

int pthread_mutex_unlock(pthread_mutex_t *p_mutex) {
	ExtPtr ep = make_ext_ptr(p_mutex);
	Mutex* m = model->get_mutex_map()->at(ep);

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

int pthread_mutex_destroy(pthread_mutex_t *p_mutex) {
	auto mutex_map = model->get_mutex_map();
	ExtPtr ep = make_ext_ptr(p_mutex);
	auto iter = mutex_map->find(ep);
	if (iter == mutex_map->end()) {
		Mutex* mutex = iter->second;
		if (mutex->get_owner()) {
			// TODO also fail if referenced by cond var
			return EBUSY;
		}
		mutex_map->erase(iter);
		delete mutex;
		return 0;
	}

	return EINVAL;
}

int pthread_cond_init(pthread_cond_t *p_cond, const pthread_condattr_t *attr) {
	ConditionVariable *v = new ConditionVariable();
	ExtPtr ep = make_ext_ptr(p_cond);

	model->get_cond_map()->emplace(ep, v);

	return 0;
}

int pthread_cond_wait(pthread_cond_t *p_cond, pthread_mutex_t *p_mutex) {
    auto mutex_map = model->get_mutex_map();
    auto cond_map = model->get_cond_map();
	ExtPtr ep_mutex = make_ext_ptr(p_mutex);
	ExtPtr ep_cond = make_ext_ptr(p_cond);
	if (mutex_map->find(ep_mutex) == mutex_map->end()) {
		pthread_mutex_init(p_mutex, NULL);
	}
	if (cond_map->find(ep_cond) == cond_map->end()) {
		pthread_cond_init(p_cond, NULL);
	}

	Mutex* m = mutex_map->at(ep_mutex);
	ConditionVariable* v = cond_map->at(ep_cond);

	v->wait(m);
	return 0;
}

int pthread_cond_timedwait(pthread_cond_t *p_cond,
													 pthread_mutex_t *p_mutex, const struct timespec *abstime) {
	auto mutex_map = model->get_mutex_map();
    auto cond_map = model->get_cond_map();
	ExtPtr ep_mutex = make_ext_ptr(p_mutex);
	ExtPtr ep_cond = make_ext_ptr(p_cond);
	if (mutex_map->find(ep_mutex) == mutex_map->end()) {
		pthread_mutex_init(p_mutex, NULL);
	}
	if (cond_map->find(ep_cond) == cond_map->end()) {
		pthread_cond_init(p_cond, NULL);
	}

	Mutex* m = mutex_map->at(ep_mutex);
	ConditionVariable* v = cond_map->at(ep_cond);

	model->action(new ModelAction(ATOMIC_TIMEDWAIT, v, (uint64_t)m));
    m->lock();
	return 0;
}

int pthread_cond_signal(pthread_cond_t *p_cond) {
	// notify only one blocked thread
    auto cond_map = model->get_cond_map();
	ExtPtr ep_cond = make_ext_ptr(p_cond);
	if (cond_map->find(ep_cond) == cond_map->end()) {
		pthread_cond_init(p_cond, NULL);
	}

	ConditionVariable* v = cond_map->at(ep_cond);

	v->notify_one();
	return 0;
}

int pthread_cond_broadcast(pthread_cond_t *p_cond) {
	// notify all blocked threads
    auto cond_map = model->get_cond_map();
	ExtPtr ep_cond = make_ext_ptr(p_cond);
	if (cond_map->find(ep_cond) == cond_map->end()) {
		pthread_cond_init(p_cond, NULL);
	}

	ConditionVariable* v = cond_map->at(ep_cond);

	v->notify_all();
	return 0;
}

int pthread_cond_destroy(pthread_cond_t *p_cond) {
	auto cond_map = model->get_cond_map();
	ExtPtr ep_cond = make_ext_ptr(p_cond);
	auto iter = cond_map->find(ep_cond);
	if (iter == cond_map->end()) {
		ConditionVariable* v = iter->second;
		cond_map->erase(iter);
		delete v;
		return 0;
	}
	return EINVAL;
}
