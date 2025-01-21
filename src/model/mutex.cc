#include "mutex.h"

void Mutex::lock() {
    model->action(new ModelAction(ATOMIC_LOCK, this));
}

bool Mutex::try_lock() {
    return model->action(new ModelAction(ATOMIC_TRYLOCK, this));
}

void Mutex::unlock() {
    model->action(new ModelAction(ATOMIC_UNLOCK, this));
}

void Mutex::set_owner(Thread* thr) {
    if (owner != nullptr && owner->get_state() != THREAD_CRASHED) {
        auto& mutex_set = owner->get_owned_mutexes();
        auto iter = mutex_set.find(this);
        assert(iter != mutex_set.end());
        mutex_set.erase(iter);
    }
    if (thr != nullptr) {
        auto& mutex_set = thr->get_owned_mutexes();
        auto pair = mutex_set.insert(this);
        assert(pair.second);
    }
    owner = thr;
    recursive_lock_count = 0;
}