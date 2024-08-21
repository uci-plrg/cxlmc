#ifndef _ACTION_H
#define _ACTION_H

#include "allocators.h"

extern int thread_id;

typedef enum action_type {
	THREAD_START,	// < First action in each thread
	PTHREAD_CREATE,	// < A pthread creation action
	PTHREAD_JOIN,	// < A pthread join action
	PLACEHOLDER	// < Placeholder
} action_type_t;

class ModelAction {
	int tid;
	action_type_t type;
	void* args;
	void* result;
public:
	ModelAction(action_type_t t) : tid(thread_id), type(t) {}
	ModelAction(action_type_t t, void* ar) : tid(thread_id), type(t), args(ar) {}
	ModelAction(action_type_t t, void* ar, void* res) : tid(thread_id), type(t), args(ar), result(res) {}

	action_type_t get_type() { return type; }
	void* get_args() { return args; }
	void* get_result() { return result; }

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

#endif