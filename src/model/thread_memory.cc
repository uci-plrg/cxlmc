#include <assert.h>

#include "model.h"
#include "thread_memory.h"

void ThreadMemory::addToStoreBuffer(ModelAction *action) {
	assert(action->get_type() == STORE || action->get_type() == CLFLUSH);
    printf("add to store buffer\n");
    storeBuffer.push_back(action);
}

bool ThreadMemory::popFromStoreBuffer() {
    printf("pop from store buffer\n");
    if (storeBuffer.size() == 0)
        return false;
    ModelAction *action = storeBuffer.front();
    storeBuffer.pop_front();

    switch (action->get_type()) {
	case STORE: {
		model->evict_store(action);
		break;
	}
	case CLFLUSH: {
		model->evict_clflush(action);
		break;
	}
	default:
        assert(false && "UNREACHABLE");
	}

    return false;
}

void ThreadMemory::emptyStoreBuffer() {
   while (!storeBuffer.empty())
	   popFromStoreBuffer();
}
