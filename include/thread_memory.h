#ifndef _THREAD_MEMORY_H
#define _THREAD_MEMORY_H

#include "shared_ADT.h"
#include "action.h"

inline bool get_overlaps(ModelAction *write, ModelAction *read, shared::vector<ModelAction *> &rf, uint &numslotsleft) {
	uintptr_t wbot = (uintptr_t) write->get_location();
	uint wsize = write->get_size();
	uintptr_t wtop = wbot + wsize;
	uintptr_t rbot = (uintptr_t) read->get_location();
	uint rsize = read->get_size();
	uintptr_t rtop = rbot + rsize;
	bool ret = false;
	//skip on if there is no overlap
	if ((wbot >= rtop) || (rbot >= wtop))
		return ret;

	//offset at the beginning of read
	uintptr_t offset = (wbot > rbot) ? (wbot - rbot) : 0;
	//the ith byte of read will be the (i - offset)th byte of write
	for(uint i = offset ; i < offset + wsize && i < rsize; i++) {
		if (rf[i] == NULL) {
			ret = true;
			rf[i] = write;
			numslotsleft--;
			if (numslotsleft == 0)
				return ret;
		}
	}
	return ret;
}

inline shared::vector<ModelAction *> * get_overlaps_save_old(ModelAction *write, ModelAction *read, shared::vector<ModelAction *> &rf, uint &numslotsleft) {
	uintptr_t wbot = (uintptr_t) write->get_location();
	uint wsize = write->get_size();
	uintptr_t wtop = wbot + wsize;
	uintptr_t rbot = (uintptr_t) read->get_location();
	uint rsize = read->get_size();
	uintptr_t rtop = rbot + rsize;
	shared::vector<ModelAction *>* ret = NULL;
	//skip on if there is no overlap
	if ((wbot >= rtop) || (rbot >= wtop))
		return ret;

	//offset at beginning of read
	uintptr_t offset = (wbot > rbot) ? (wbot - rbot) : 0;
	//the ith byte of read will be the (i - woffset)th byte of write
	for(uint i = offset ; i < offset + wsize && i < rsize; i++) {
		if (rf[i] == NULL) {
			if (ret == NULL)
				ret = new shared::vector<ModelAction *>(rf);
			rf[i] = write;
			numslotsleft--;
			if (numslotsleft == 0)
				return ret;
		}
	}
	return ret;
}

inline uint64_t get_read_value(void *read_loc, shared::vector<ModelAction *> &rf) {
	uint64_t value = 0;
	for (int i= (int)rf.size()-1; i >= 0; i--) {
		value = value << 8;
		auto write = rf[i];
		if (!write)
			continue;
		int offset = i + (char *)read_loc - (char *)write->get_location();
		uint64_t writevalue = write->get_value() >> (8 * offset);
		value |= writevalue & 0xff;
	}
	return value;
}

class ThreadMemory {
    shared::list<ModelAction *> storeBuffer;
    shared::list<ModelAction *> flushBuffer;
    modelclock_t last_sfence = 0;
    shared::hashmap<uintptr_t, modelclock_t> obj_to_last_wr_or_clf;

public:
    ThreadMemory() {}
    ~ThreadMemory() {
        for (ModelAction *s: storeBuffer) {
            delete s;
		}
        for (ModelAction *f: flushBuffer)
            delete f;
    }

    void add_to_store_buffer(ModelAction *action);
	bool local_bypassing(ModelAction* read, shared::vector<ModelAction *> &rf, uint &numslotsleft);
	bool pop_from_store_buffer();
    void empty_store_buffer();
    void empty_flush_buffer();
	size_t get_store_buffer_size() {return storeBuffer.size(); }
	size_t get_flush_buffer_size() {return flushBuffer.size(); }
};

#endif
