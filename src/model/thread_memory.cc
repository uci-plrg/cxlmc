#include <assert.h>

#include "model.h"
#include "thread_memory.h"

void ThreadMemory::addToStoreBuffer(ModelAction *action) {
    storeBuffer.push_back(action);
}

bool ThreadMemory::popFromStoreBuffer() {
    if (storeBuffer.size() == 0)
        return false;
    ModelAction *action = storeBuffer.front();
    storeBuffer.pop_front();

    if (action->get_type() == STORE)
        model->add_to_store_list(action);
    else
        assert(false && "UNREACHABLE");

    return false;
}
