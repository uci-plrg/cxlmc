#ifndef _RFENTRY_H
#define _RFENTRY_H

#include "cacheline.h"
#include "shared_ADT.h"
#include "action.h"
#include "types.h"

struct rfEntry {
	shared::vector<ModelAction *> *overlaps;
	CacheLineStore cl_store;
	uintptr_t addr;
	shared::hashmap<process_id_t, modelclock_t> crashes;

	~rfEntry() { delete overlaps; }
	rfEntry (shared::vector<ModelAction *> *ov, uintptr_t a, const CacheLineStore &cls, const shared::hashmap<process_id_t, modelclock_t> &cr): overlaps(ov), addr(a), crashes(cr) {
		cl_store.copy_at(cls, addr);
	}

	void dump();
	uint64_t get_read_value(void *read_location);
	//returns whether any new action is added to overlaps
	inline bool get_overlaps(ModelAction *write, ModelAction *read, uint &numslotsleft) {
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
	
		//offset at beginning of read
		uintptr_t offset = (wbot > rbot) ? (wbot - rbot) : 0;
		//the ith byte of read will be the (i - offset)th byte of write
		for(uint i = offset ; i < offset + wsize && i < rsize; i++) {
			if ((*overlaps)[i] == NULL) {
				ret = true;
				(*overlaps)[i] = write;
				numslotsleft--;
				if (numslotsleft == 0)
					return ret;
			}
		}
		return ret;
	}

	shared::vector<ModelAction *> * get_overlaps_save_old(ModelAction *write, ModelAction *read, uint &numslotsleft) {
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
			if ((*overlaps)[i] == NULL) {
				if (ret == NULL)
					ret = new shared::vector<ModelAction *>(*overlaps);
				(*overlaps)[i] = write;
				numslotsleft--;
				if (numslotsleft == 0)
					return ret;
			}
		}
		return ret;
	}
};

#endif
