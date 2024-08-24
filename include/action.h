#ifndef _ACTION_H
#define _ACTION_H

extern int thread_id;

typedef enum action_type {
	THREAD_START,	// < First action in each thread
	PTHREAD_CREATE,	// < A pthread creation action
	PTHREAD_JOIN,	// < A pthread join action
	STORE,			// < A store to memory action
	LOAD,			// < A load from memory action
	PLACEHOLDER	    // < Placeholder
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
};

#endif
