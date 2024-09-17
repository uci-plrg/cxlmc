#include "action.h"
#include "assert.h"

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