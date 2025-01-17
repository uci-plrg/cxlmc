#ifndef _THREAD_MEMORY_H
#define _THREAD_MEMORY_H

#include "shared_ADT.h"
#include "action.h"
#include "rfentry.h"

class ThreadMemory {
    shared::list<ModelAction *> storeBuffer;
    shared::list<ModelAction *> flushBuffer;
    ModelAction* last_sfence;

public:
    ThreadMemory() : last_sfence(nullptr) {}
    ~ThreadMemory() {
        for (ModelAction *s: storeBuffer) {
            delete s;
		}
        for (ModelAction *f: flushBuffer)
            delete f;
    }

    void add_to_store_buffer(ModelAction *action);
	bool get_lastest_writes(ModelAction* read, rfEntry &entry, uint &numslotsleft);
	bool pop_from_store_buffer();
    void empty_store_buffer();
    void empty_flush_buffer();
	size_t get_store_buffer_size() {return storeBuffer.size(); }
	size_t get_flush_buffer_size() {return flushBuffer.size(); }

};

#endif
