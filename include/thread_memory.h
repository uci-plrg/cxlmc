#ifndef _THREAD_MEMORY_H
#define _THREAD_MEMORY_H

#include "shared_data.h"
#include "action.h"

class ThreadMemory {
    shared::list<ModelAction *> storeBuffer;
    shared::list<ModelAction *> flushBuffer;

public:
    void addToStoreBuffer(ModelAction *action);

    bool popFromStoreBuffer();
};

#endif
