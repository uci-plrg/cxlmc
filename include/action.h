#ifndef _ACTION_H
#define _ACTION_H

#include "allocators.h"
#include "types.h"
#include <atomic>
using std::memory_order;
using std::memory_order_relaxed;
using std::memory_order_consume;
using std::memory_order_acquire;
using std::memory_order_release;
using std::memory_order_acq_rel;
using std::memory_order_seq_cst;

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

	ATOMIC_STORE,   // < Anatomic store
	ATOMIC_LOAD,	// < Anatomic load
	ATOMIC_LOCK,	// < A lock action
	ATOMIC_TRYLOCK,	// < A trylock action
	ATOMIC_UNLOCK,	// < An unlock action
	ATOMIC_NOTIFY_ONE,	// < A notify_one action
	ATOMIC_NOTIFY_ALL,	// < A notify all action
	ATOMIC_WAIT,	// < A wait action
	ATOMIC_TIMEDWAIT,	// < A timed wait action

	CACHE_MFENCE,     // < A memory fence
	CACHE_SFENCE,	  // < A store fence
	CACHE_CLFLUSH,	  // < A cacheline flush
	CACHE_CLFLUSHOPT, // < An optimized cacheline flush
	PLACEHOLDER	      // < Placeholder
	
} action_type_t;

const char *action_type2str(action_type_t type);

class ModelAction {
	thread_id_t tid;
	action_type_t type;

	void* location;
	uint64_t value;
	modelclock_t seq_num;
	modelclock_t last_clflush;
	uint size;
	memory_order order;
	const char *position;

public:
	ModelAction(action_type_t t) : tid(thread_id), type(t), seq_num(0), size(0), order(memory_order_seq_cst), position(NULL) {}
	ModelAction(action_type_t t, void* loc, uint64_t val=0, memory_order ord=memory_order_seq_cst, uint sz=1, const char *pos=NULL) : tid(thread_id), type(t), location(loc), value(val), seq_num(0), size(sz), order(ord), position(pos) {}
	ModelAction(ModelAction &action) : tid(action.tid), type(action.type), location(action.location), value(action.value), seq_num(action.seq_num), size(action.size), order(action.order), position(action.position) {}

	void set_seq_num(modelclock_t seq_n) { seq_num = seq_n; }
	modelclock_t get_seq_num () {return seq_num; }
	thread_id_t get_thread_id() { return tid; }
	action_type_t get_type() { return type; }
	void* get_location() { return location; }
	uint64_t get_value() { return value; }
	void set_value(uint64_t val) { value = val; }
	uint get_size() { return size; }
	modelclock_t get_last_clflush() { return last_clflush; }
	void set_last_clflush(modelclock_t lc) { last_clflush = lc; }
	bool is_seq_cst() { return order == memory_order_seq_cst; }

	Thread* get_thread();
	Mutex* get_mutex();
	ConditionVariable* get_cond();

    MODELALLOC
};

#endif
