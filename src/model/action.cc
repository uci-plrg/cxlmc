#include "action.h"
#include "assert.h"
const char *action_type2str(action_type_t type) {
	switch (type) {
		case THREAD_START: 
			return "THREAD_START";
		case THREAD_YIELD:
			return "THREAD_YIELD";
		case THREAD_FINISH:
			return "THREAD_FINISH";
		case THREADONLY_FINISH:
			return "THREADONLY_FINISH";
		case PTHREAD_CREATE:
			return "PTHREAD_CREATE";
		case PTHREAD_JOIN:
			return "PTHREAD_JOIN";
		case NONATOMIC_STORE:
			return "NONACTOMIC_STORE";
		case NONATOMIC_LOAD:
			return "NONACTOMIC_LOAD";
		case ATOMIC_LOCK:
			return "ATOMIC_LOCK";
		case ATOMIC_TRYLOCK:
			return "ATOMIC_TRYLOCK";
		case ATOMIC_UNLOCK:
			return "ATOMIC_UNLOCK";
		case ATOMIC_NOTIFY_ONE:
			return "ATOMIC_NOTIFY_ONE";
		case ATOMIC_NOTIFY_ALL:
			return "ATOMIC_NOTIFY_ALL";
		case ATOMIC_WAIT:
			return "ATOMIC_WAIT";
		case ATOMIC_TIMEDWAIT:
			return "ATOMCI_TIMEDWAIT";
		case CACHE_MFENCE:
			return "CACHE_MFENCE";
		case CACHE_SFENCE:
			return "CACHE_SFENCE";
		case CACHE_CLFLUSH:
			return "CACHE_CLFLUSH";
		case CACHE_CLFLUSHOPT:
			return "CACHE_CLFLUSHOPT";
		case PLACEHOLDER:
			return "PLACEHOLDER";
		default:
			assert(false && "unreachable");
			return "";
	}
}

Thread* ModelAction::get_thread() {
    switch(type) {
    case PTHREAD_JOIN:
        return (Thread*)location;
    default:
        assert(0);
    }
}

Mutex* ModelAction::get_mutex() {
    switch(type) {
    case ATOMIC_TRYLOCK:
    case ATOMIC_LOCK:
    case ATOMIC_UNLOCK:
        return (Mutex*)location;
    case ATOMIC_WAIT:
        return (Mutex*)value;
    default:
        assert(0);
    }
}

ConditionVariable* ModelAction::get_cond() {
    switch(type) {
    case ATOMIC_NOTIFY_ONE:
    case ATOMIC_NOTIFY_ALL:
    case ATOMIC_WAIT:
    case ATOMIC_TIMEDWAIT:
        return (ConditionVariable*)location;
    default:
        assert(0);
    }
}
