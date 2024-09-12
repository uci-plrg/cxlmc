#include <assert.h>

#include "model.h"
#include "thread_memory.h"

void ThreadMemory::add_to_store_buffer(ModelAction *action) {
	assert(action->get_type() == NONATOMIC_STORE
        || action->get_type() == CACHE_SFENCE
        || action->get_type() == CACHE_CLFLUSH
        || action->get_type() == CACHE_CLFLUSHOPT);
    //printf("add to store buffer\n");
    storeBuffer.push_back(action);
}

ModelAction *ThreadMemory::get_last_write(ModelAction* act) {
     for (auto iter = storeBuffer.rbegin(); iter != storeBuffer.rend(); iter++) {
         ModelAction* write = *iter;
         if (write->get_type() == NONATOMIC_STORE && write->get_location() == act->get_location()) {
			 return write;
         }
     }

     return NULL;
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
	case CACHE_CLFLUSHOPT: {
        if (last_sfence != nullptr) {
            action->set_last_clflush(last_sfence->get_seq_num());
        }
		flushBuffer.push_back(action);
		break;
	}
    case CACHE_SFENCE: {
        empty_flush_buffer();
        last_sfence = action;
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

void ThreadMemory::empty_flush_buffer() {
    while (!flushBuffer.empty()) {
        ModelAction* action = flushBuffer.front();
        flushBuffer.pop_front();
        assert(action->get_type() == CACHE_CLFLUSHOPT);
        model->evict_clflush(action);
    }
}