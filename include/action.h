#ifndef _ACTION_H
#define _ACTION_H

#include "allocators.h"
#include "types.h"

extern thread_id_t thread_id;

typedef enum action_type {
	THREAD_START,	  // < First action in each thread
	THREAD_YIELD,	  // < A thread yield action
	THREAD_FINISH,	  // < A thread completion action
	THREADONLY_FINISH,// < A thread completion action (pthread_exit)

	PTHREAD_CREATE,	  // < A pthread creation action
	PTHREAD_JOIN,	  // < A pthread join action
	NONATOMIC_STORE,  // < A nonatomic store
	NONATOMIC_LOAD,	  // < A nonatomic load
	CACHE_MFENCE,     // < A memory fence
	CACHE_SFENCE,	  // < A store fence
	CACHE_CLFLUSH,	  // < A cacheline flush
	CACHE_CLFLUSHOPT, // < An optimized cacheline flush
	PLACEHOLDER	      // < Placeholder
} action_type_t;

class ModelAction {
	thread_id_t tid;
	action_type_t type;

	void* location;
	uint64_t value;
	modelclock_t seq_num;
public:
	ModelAction(action_type_t t) : tid(thread_id), type(t) {}
	ModelAction(action_type_t t, void* loc, uint64_t val=0) : tid(thread_id), type(t), location(loc), value(val) {}
	ModelAction(ModelAction &action) : tid(action.tid), type(action.type), location(action.location), value(action.value) {}

	void set_seq_num(modelclock_t seq_n) { seq_num = seq_n; }
	modelclock_t get_seq_num () {return seq_num; }
	thread_id_t get_thread_id() { return tid; }
	action_type_t get_type() { return type; }
	void* get_location() { return location; }
	uint64_t get_value() { return value; }
	void set_value(uint64_t val) { value = val; }

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
