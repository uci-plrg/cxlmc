#ifndef _THREAD_MEMORY_H
#define _THREAD_MEMORY_H

#include "snapshot_ADT.h"
#include "action.h"

class ThreadMemory {
    shared::list<ModelAction *> storeBuffer;
    shared::list<ModelAction *> flushBuffer;
    ModelAction* last_sfence;
public:
    ThreadMemory() : last_sfence(nullptr) {}
    ~ThreadMemory() {
        for (auto s: storeBuffer)
            delete s;
        for (auto f: flushBuffer)
            delete f;
    }

    void add_to_store_buffer(ModelAction *action);
	ModelAction *get_last_write(ModelAction* act);
	bool pop_from_store_buffer();
    void empty_store_buffer();
    void empty_flush_buffer();
};

#endif
