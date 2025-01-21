#ifndef CACHELINE_H
#define CACHELINE_H

#include <cstdint>
#include <climits>
#include <map>
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
	// stores a crash point A (or UINT_MAX if no crash) and a map of cacheline constraints.
	// Let B be the crash point before A or 0 if none. the map stores constraints whose begin is in [B, A) 
	// ordered from earliest crash point to latest
	using store_t = shared::vector<shared::Pair<modelclock_t, shared::hashmap<uintptr_t, cacheline>>>;
	store_t  _store; 

	//return the first cacheline and the map index searching from ith map towards the beginning of the vector
	cacheline *find_first(int &i, uintptr_t addr) {
		for (; i>=0; i--) {
			auto &map = _store[i].second; 
			auto itr = map.find(addr);
			if (itr != map.end())
				return &itr->second;
		}
		i++;
		return (cacheline *)nullptr;
	}

	void ensure_init() {
		if (_store.size() == 0)
			_store.push_back(shared::Pair{UINT_MAX, shared::hashmap<uintptr_t, cacheline>{}}); 
	}
public:
	CacheLineStore() = default;
	//{
	//	_store.push_back(shared::Pair{UINT_MAX, shared::hashmap<uintptr_t, cacheline>{}}); 
	//}

	CacheLineStore(const CacheLineStore &other) = default;

	cacheline &get_cacheline(uintptr_t addr, modelclock_t crash_point=UINT_MAX) {
		ensure_init();
		int i = _store.size()-1;
		//can use binary search to optimize
		while (i!=0 &&_store[i].first != crash_point)
			i--;
		assert(_store[i].first == crash_point);

		cacheline *first = find_first(i, addr);
		if (!first)
			return _store[0].second[addr] = cacheline{};
		return *first;
	}

	cacheline &set_cacheline(uintptr_t addr, const cacheline &cl) {
		ensure_init();
		int i = 0;
		//can use binary search to optimize
		while (_store[i].first < cl.getBegin())
			i++;
		return _store[i].second[addr] = cl;
	}

	void insert_crash(modelclock_t crash_point) {
		ensure_init();
		_store[_store.size()-1].first = crash_point;
		_store.push_back(shared::Pair(UINT_MAX, shared::hashmap<uintptr_t, cacheline>{}));
	}

	void copy_at(const CacheLineStore &other, uintptr_t addr) {
		const auto &other_store = other.get_store();
		if (other_store.size() == 0)
			return;
		assert(other_store.size() >= _store.size());

		ensure_init();
		unsigned i = _store.size()-1;
		_store[i].first = other_store[i].first;
		for (i++;i < other_store.size(); i++)
			_store.push_back(shared::Pair{other_store[i].first, shared::hashmap<uintptr_t, cacheline>{}});

		for (i = 0; i < other_store.size(); i++) {
			const auto &map = other_store[i].second;
			auto itr = map.find(addr);
			if (itr != map.end())
				_store[i].second[addr] = itr->second;
		}	
	}

	const store_t &get_store() const {
		return _store;
	}

	void clear() {
		_store.clear();
	}

	void dump() {
		for (const auto &pair: _store) {
			if (pair.second.size() == 0)
				continue;
			if (pair.first == UINT_MAX)
				printf("current: {");
			else 
				printf("before %u: {", pair.first);
			for (const auto &mpair: pair.second)
				printf("%p: (%d, %d), ", (void*) mpair.first, mpair.second.getBegin(), mpair.second.getEnd()); 
			printf("}\n");
		}
	}

	void dump(uintptr_t addr) {
		for (auto &pair: _store) {
			auto itr = pair.second.find(addr);
			if (itr != pair.second.end()) {
				auto &cl = itr->second;
				if (pair.first == UINT_MAX)
					printf("current: ");
				else 
					printf("before %u: ", pair.first);
				printf("(%d, %d), ", cl.getBegin(), cl.getEnd()); 
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
