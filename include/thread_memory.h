#ifndef _THREAD_MEMORY_H
#define _THREAD_MEMORY_H

#include "snapshot_ADT.h"
#include "action.h"

class ThreadMemory {
    snapshot::list<ModelAction *> storeBuffer;
    snapshot::list<ModelAction *> flushBuffer;

public:
    ~ThreadMemory() {
        for (auto s: storeBuffer)
            delete s;
        for (auto f: flushBuffer)
            delete f;
    }

    void add_to_store_buffer(ModelAction *action);
	uint8_t get_last_write(ModelAction* act);
    bool pop_from_store_buffer();
    void empty_store_buffer();
};

#endif
