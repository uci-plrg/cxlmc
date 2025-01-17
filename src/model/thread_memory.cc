#include <assert.h>

#include "thread_memory.h"
#include "model.h"

void ThreadMemory::add_to_store_buffer(ModelAction *action) {
	assert(action->get_type() == ATOMIC_INIT
        || action->get_type() == ATOMIC_STORE
        || action->get_type() == NONATOMIC_STORE
        || action->get_type() == ATOMIC_RMW
        || action->get_type() == CACHE_SFENCE
        || action->get_type() == CACHE_CLFLUSH
        || action->get_type() == CACHE_CLFLUSHOPT);
	if (VERBOSE > 1)
		printf("add to store buffer: type=%s, val=%lx\n", action_type2str(action->get_type()), action->get_value());
    storeBuffer.push_back(action);
}

bool ThreadMemory::get_lastest_writes(ModelAction* read, rfEntry &entry, uint &numslotsleft) {
     for (auto iter = storeBuffer.rbegin(); iter != storeBuffer.rend(); iter++) {
         ModelAction* write = *iter;
         if (write->get_type() == NONATOMIC_STORE) {
			 entry.get_overlaps(write, read, numslotsleft);
			 if (numslotsleft == 0)
				return true;
         }
     }

     return false;
 }

bool ThreadMemory::pop_from_store_buffer() {
    if (storeBuffer.size() == 0)
        return false;
    ModelAction *action = storeBuffer.front();
	if (VERBOSE > 1)
		printf("pop store buffer: type=%s, val=%lx\n", action_type2str(action->get_type()), action->get_value());
    storeBuffer.pop_front();

    switch (action->get_type()) {
	case ATOMIC_INIT:
	case ATOMIC_STORE: 
    case NONATOMIC_STORE:
    case ATOMIC_RMW: {
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
