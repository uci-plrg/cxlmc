#ifndef _RFENTRY_H
#define _RFENTRY_H

#include "cacheline.h"
#include "shared_ADT.h"
#include "action.h"
#include "types.h"

struct rfEntry {
	shared::vector<ModelAction *> overlaps;
	CacheLineStore cl_store;
	uintptr_t addr;
	shared::hashmap<process_id_t, modelclock_t> crashes;
	uint numslotsleft;

	rfEntry (const rfEntry &other) = default;
	rfEntry (uintptr_t a, const CacheLineStore &cls, const shared::hashmap<process_id_t, modelclock_t> &cr, uint n): overlaps(n), addr(a), crashes(cr), numslotsleft(n) {
		cl_store.copy_at(cls, addr);
	}

	void dump();
	uint64_t get_read_value(void *read_location);
	//returns whether any new action is added to overlaps
	inline bool get_overlaps(ModelAction *write, ModelAction *read) {
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
	
		uintptr_t offset = wbot - rbot;
		//the ith byte of read will be the (i - offset)th byte of write
		for(uint i = offset ;i < offset + wsize && i < rsize; i++) {
			if (overlaps[i] == NULL) {
				overlaps[i] = write;
				numslotsleft--;
				ret = true;
				if (numslotsleft == 0)
					return ret;
			}
		}
		return ret;
	}
};

#endif
