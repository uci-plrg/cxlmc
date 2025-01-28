#ifndef _MUTEX_H
#define _MUTEX_H

#include "model.h"

class Mutex {
    int mutex_type;
    Thread* owner;
    int recursive_lock_count;
public:
    Mutex(int type) : mutex_type(type), owner(nullptr), recursive_lock_count(0) {}

    bool lock();

    bool try_lock();

    void unlock();

    void set_owner(Thread* thr);

    void owner_crashed() { owner = nullptr; recursive_lock_count = 0; }
    
    Thread* get_owner() { return owner; }

    int get_mutex_type() { return mutex_type; }
    
    void increment_lock_count() { recursive_lock_count++; }
    int get_recursive_lock_count() { return recursive_lock_count; }
    void decrement_lock_count() { recursive_lock_count--; }

    SHAREDALLOC
};

#endif
