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

    void addToStoreBuffer(ModelAction *action);

    bool popFromStoreBuffer();
    void emptyStoreBuffer();
};

#endif
