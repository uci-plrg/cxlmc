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