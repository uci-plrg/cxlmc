#include "condition_variable.h"

void ConditionVariable::notify_one() {
    model->action(new ModelAction(ATOMIC_NOTIFY_ONE, this));
}

void ConditionVariable::notify_all() {
    model->action(new ModelAction(ATOMIC_NOTIFY_ALL, this));
}

void ConditionVariable::wait(Mutex* lock) {
    model->action(new ModelAction(ATOMIC_WAIT, this, (uint64_t)lock));
    lock->lock();
}