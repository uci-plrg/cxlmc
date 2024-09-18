#ifndef _THREAD_MEMORY_H
#define _THREAD_MEMORY_H

#include "shared_ADT.h"
#include "action.h"

inline bool get_overlaps(shared::vector<ModelAction *> &overlaps, ModelAction *write, ModelAction *read, uint &numslotsleft) {
	uintptr_t wbot = (uintptr_t) write->get_location();
	uint wsize = write->get_size();
	uintptr_t wtop = wbot + wsize;
	uintptr_t rbot = (uintptr_t) read->get_location();
	uint rsize = read->get_size();
	uintptr_t rtop = rbot + rsize;
	//skip on if there is no overlap
	if ((wbot >= rtop) || (rbot >= wtop))
		return false;

	uintptr_t offset = wbot - rbot;
	//the ith byte of read will be the (i - offset)th byte of write
	for(uint i = offset ;i < offset + wsize && i < rsize; i++) {
		if (overlaps[i] == NULL) {
			overlaps[i] = write;
			numslotsleft--;
			if (numslotsleft == 0)
				return true;
		}
	}
	return false;
}

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
	bool get_last_write(ModelAction* read, shared::vector<ModelAction *> &overlaps, uint &numslotsleft);
	ModelAction *get_last_write(ModelAction* act);
	bool pop_from_store_buffer();
    void empty_store_buffer();
    void empty_flush_buffer();
};

#endif
