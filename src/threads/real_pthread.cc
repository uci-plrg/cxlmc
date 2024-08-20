#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include "threads.h"

// static int (*real_epoll_wait_p)(int epfd, struct epoll_event *events, int maxevents, int timeout) = NULL;

// int real_epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout) {
// 	return real_epoll_wait_p(epfd, events, maxevents, timeout);
// }

static int (*pthread_mutex_init_p)(pthread_mutex_t *__mutex, const pthread_mutexattr_t *__mutexattr) = NULL;

int real_pthread_mutex_init(pthread_mutex_t *__mutex, const pthread_mutexattr_t *__mutexattr) {
	return pthread_mutex_init_p(__mutex, __mutexattr);
}

static int (*pthread_mutex_lock_p) (pthread_mutex_t *__mutex) = NULL;

int real_pthread_mutex_lock (pthread_mutex_t *__mutex) {
	return pthread_mutex_lock_p(__mutex);
}

static int (*pthread_mutex_unlock_p) (pthread_mutex_t *__mutex) = NULL;

int real_pthread_mutex_unlock (pthread_mutex_t *__mutex) {
	return pthread_mutex_unlock_p(__mutex);
}

static int (*pthread_create_p) (pthread_t *__restrict, const pthread_attr_t *__restrict, void *(*)(void *), void * __restrict) = NULL;

int real_pthread_create (pthread_t *__restrict __newthread, const pthread_attr_t *__restrict __attr, void *(*__start_routine)(void *), void *__restrict __arg) {
	return pthread_create_p(__newthread, __attr, __start_routine, __arg);
}

static int (*pthread_join_p) (pthread_t __th, void ** __thread_return) = NULL;

int real_pthread_join (pthread_t __th, void ** __thread_return) {
	return pthread_join_p(__th, __thread_return);
}

static void (*pthread_exit_p)(void *) __attribute__((noreturn))= NULL;

void real_pthread_exit (void * value_ptr) {
	pthread_exit_p(value_ptr);
}

void real_init_all() {
	char * error;
	// if (!real_epoll_wait_p) {
	// 	real_epoll_wait_p = (int (*)(int epfd, struct epoll_event *events, int maxevents, int timeout))dlsym(RTLD_NEXT, "epoll_wait");
	// 	if ((error = dlerror()) != NULL) {
	// 		fputs(error, stderr);
	// 		exit(EXIT_FAILURE);
	// 	}
	// }

	if (!pthread_mutex_init_p) {
		pthread_mutex_init_p = (int (*)(pthread_mutex_t *__mutex, const pthread_mutexattr_t *__mutexattr))dlsym(RTLD_NEXT, "pthread_mutex_init");
		if ((error = dlerror()) != NULL) {
			fputs(error, stderr);
			exit(EXIT_FAILURE);
		}
	}
	if (!pthread_mutex_lock_p) {
		pthread_mutex_lock_p = (int (*)(pthread_mutex_t *__mutex))dlsym(RTLD_NEXT, "pthread_mutex_lock");
		if ((error = dlerror()) != NULL) {
			fputs(error, stderr);
			exit(EXIT_FAILURE);
		}
	}
	if (!pthread_mutex_unlock_p) {
		pthread_mutex_unlock_p = (int (*)(pthread_mutex_t *__mutex))dlsym(RTLD_NEXT, "pthread_mutex_unlock");
		if ((error = dlerror()) != NULL) {
			fputs(error, stderr);
			exit(EXIT_FAILURE);
		}
	}
	if (!pthread_create_p) {
		pthread_create_p = (int (*)(pthread_t *__restrict, const pthread_attr_t *__restrict, void *(*)(void *), void *__restrict))dlsym(RTLD_NEXT, "pthread_create");
		if ((error = dlerror()) != NULL) {
			fputs(error, stderr);
			exit(EXIT_FAILURE);
		}
	}
	if (!pthread_join_p) {
		pthread_join_p = (int (*)(pthread_t __th, void ** __thread_return))dlsym(RTLD_NEXT, "pthread_join");
		if ((error = dlerror()) != NULL) {
			fputs(error, stderr);
			exit(EXIT_FAILURE);
		}
	}

	if (!pthread_exit_p) {
		*((void (**)(void *)) &pthread_exit_p) = (void (*)(void *))dlsym(RTLD_NEXT, "pthread_exit");
		if ((error = dlerror()) != NULL) {
			fputs(error, stderr);
			exit(EXIT_FAILURE);
		}
	}
}