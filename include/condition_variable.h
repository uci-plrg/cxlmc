#ifndef _CONDITION_VARIABLE_H
#define _CONDITION_VARIABLE_H

#include "mutex.h"

class ConditionVariable {
public:
    void notify_one();
    void notify_all();
    void wait(Mutex* lock);

	SHAREDALLOC
};

#endif
