#ifndef CACHELINE_H
#define CACHELINE_H

#include <cstdint>

#include "types.h"
#include "config.h"
#include "shared_ADT.h"

class CacheLine {
public:
	CacheLine(): begin(0), end(0) {}
	CacheLine(uintptr_t ID): begin(0), end(0), id(ID)  {}
	uintptr_t getId() { return id; }
	modelclock_t getBegin() { return begin; }
	void setBegin(modelclock_t b) { begin = b;} 	
	modelclock_t getEnd() { return end; }
	void setEnd(modelclock_t e) { end = e;}
 
private:
	//begin: earliest point at which the cache line could have been written, inclusive.
	modelclock_t begin;
	//end: latest point at which the cache line could have been written, inclusive.
	modelclock_t end;
	uintptr_t id;

};

inline uintptr_t getCacheID(const void *address) {
	return ((uintptr_t)address) & ~(CACHELINE_SIZE - 1);
}

#endif
