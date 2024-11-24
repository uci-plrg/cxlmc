#ifndef CACHELINE_H
#define CACHELINE_H

#include <cstdint>
#include <map>

#include "types.h"
#include "config.h"
#include "shared_ADT.h"

class Range {
public:
	Range() : begin(0), end(0) {}
	Range(const Range &other) = default;
	modelclock_t getBegin() const { return begin; }
	modelclock_t getEnd() const { return end; }
	void setBegin(modelclock_t b) { begin = b; }
	void setEnd(modelclock_t e) { end = e; }

	SHAREDALLOC;
private:
	//both begin and end are inclusive
	modelclock_t begin;
	modelclock_t end;
};

class CacheLine {
	//TODO: change to shared::vector
	using range_map_t = std::map<modelclock_t, Range, std::greater<modelclock_t>, shared_allocator<std::pair<const modelclock_t, Range>>>;
 
public:
	CacheLine() = default;
	CacheLine(const CacheLine &other) = default;
	CacheLine(uintptr_t ID): id(ID) {}

	uintptr_t getId() { return id; }
	
	Range &get_current() {
		if (range_map.size() == 0)
			return range_map[0];
		return range_map.rbegin()->second;
	}

	Range &get_before(modelclock_t clock) {
		if (range_map.size() == 0)
			return range_map[0];
		auto itr = range_map.upper_bound(clock);
		if (itr != range_map.begin())
			itr--;
		return itr->second; 
	}

	Range &insert_range(modelclock_t clock, const Range &range) {
		return range_map[clock] = range;
	}

	const range_map_t &get_range_map() {
		return range_map;
	}

private: 
	uintptr_t id;
	range_map_t range_map;
};

inline uintptr_t getCacheID(const void *address) {
	return ((uintptr_t)address) & ~(CACHELINE_SIZE - 1);
}

inline shared::Pair<uintptr_t, uintptr_t> getCacheRange(const uintptr_t id) {
	return shared::Pair(id, id | (CACHELINE_SIZE -1));
}
#endif
