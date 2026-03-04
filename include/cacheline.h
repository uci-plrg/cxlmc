#ifndef CACHELINE_H
#define CACHELINE_H

#include <cstdint>
#include <climits>
#include "assert.h"

#include "types.h"
#include "config.h"
#include "shared_ADT.h"

class Range {
public:
	Range() : begin(0), end(0) {}
	Range(modelclock_t b, modelclock_t e): begin(b), end(e) {}
	Range(const Range &other) = default;
	modelclock_t getBegin() const { return begin; }
	modelclock_t getEnd() const { return end; }
	void setBegin(modelclock_t b) { begin = b; }
	void setEnd(modelclock_t e) { end = e; }

	SHAREDALLOC;
private:
	//begin is inclusive and end is exclusive
	modelclock_t begin;
	modelclock_t end;
};

using cacheline = Range;

class CacheLineStore {
	using store_t = shared::vector<shared::hashmap<uintptr_t, cacheline>>;
	store_t  _store; 

public:
	CacheLineStore() = default;

	CacheLineStore(const CacheLineStore &other) = default;

    void init(process_id_t p_count) {
    	_store.resize(p_count);
		for (int i=0; i<p_count; i++)
			_store.insertAt(i, shared::hashmap<uintptr_t, cacheline>()); 
	}

	cacheline &get_cacheline(process_id_t pid, uintptr_t addr) {
		assert((size_t)pid < _store.size());
		if (_store[pid].find(addr) == _store[pid].end())
			return _store[pid][addr] = cacheline{};
		return _store[pid][addr];
	}

	void set_cacheline(process_id_t pid, uintptr_t addr, const cacheline &cl) {
		assert((size_t)pid < _store.size());
		_store[pid][addr] = cl;
	}	

	const store_t &get_store() const {
		return _store;
	}

	void clear() {
        for (auto &p: _store)
		    p.clear();
	}

	void dump() {
		for (uint i=0; i<_store.size(); i++) {
			if (_store[i].size() == 0)
				continue;
			printf("process %u: {", i);
			for (const auto &pair: _store[i])
				printf("%p: (%d, %d), ", (void*) pair.first, pair.second.getBegin(), pair.second.getEnd()); 
			printf("}\n");
		}
	}

	void dump(uintptr_t addr) {
		for (uint i=0; i<_store.size(); i++) {
			auto itr = _store[i].find(addr);
			if (itr != _store[i].end()) {
				auto &cl = itr->second;
				printf("process %u: (%d, %d),", i, cl.getBegin(), cl.getEnd());
			}
		}
	}
};

inline uintptr_t getCacheID(const void *address) {
	return ((uintptr_t)address) & ~(CACHELINE_SIZE - 1);
}

inline shared::Pair<uintptr_t, uintptr_t> getCacheRange(const uintptr_t id) {
	return shared::Pair(id, id | (CACHELINE_SIZE -1));
}
#endif
