#ifndef _TYPES_H
#define _TYPES_H

typedef int thread_id_t;
typedef int process_id_t;
//should start at 1, 0 means the modelclock is undefined
typedef unsigned int modelclock_t;

class ConditionVariable;
class Mutex;
class Thread;

#endif
