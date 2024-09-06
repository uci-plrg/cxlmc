#include <assert.h>

#include "model.h"
#include "thread_memory.h"

void ThreadMemory::add_to_store_buffer(ModelAction *action) {
	assert(action->get_type() == NONATOMIC_STORE || action->get_type() == CACHE_CLFLUSH);
    //printf("add to store buffer\n");
    storeBuffer.push_back(action);
}

uint64_t ThreadMemory::get_last_write(ModelAction* act) {
     for (auto iter = storeBuffer.rbegin(); iter != storeBuffer.rend(); iter++) {
         ModelAction* write = *iter;
         if (write->get_type() == NONATOMIC_STORE && write->get_location() == act->get_location()) {
             return write->get_value();
         }
     }

     return *(uint64_t*) act->get_location();
 }

bool ThreadMemory::pop_from_store_buffer() {
    //printf("pop from store buffer\n");
    if (storeBuffer.size() == 0)
        return false;
    ModelAction *action = storeBuffer.front();
    storeBuffer.pop_front();

    switch (action->get_type()) {
	case NONATOMIC_STORE: {
		model->evict_store(action);
		break;
	}
	case CACHE_CLFLUSH: {
		model->evict_clflush(action);
		break;
	}
	default:
        assert(false && "UNREACHABLE");
	}

    return false;
}

void ThreadMemory::empty_store_buffer() {
   while (!storeBuffer.empty())
	   pop_from_store_buffer();
}
