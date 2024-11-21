#ifndef _SHARED_ADT_H
#define _SHARED_ADT_H

#include <list>
#include <string>
#include <vector>
#include <unordered_map>
#include "allocators.h"
#include "data_structures.h"

void * shared_malloc(size_t bytes);
void shared_free(void* mem);
void * shared_realloc(void* mem, size_t newsize);

namespace shared { 
    template <typename T>
    using vector = Vector<T, shared_malloc, shared_realloc, shared_free>;
    template <typename T>
    using list = List<T, shared_malloc, shared_realloc, shared_free>;
	template<typename K, typename T>
	using hashmap = std::unordered_map<K, T, std::hash<K>, std::equal_to<K>, model_allocator<std::pair<const K, T>>>;

	template<typename _T1, typename _T2>
	class Pair {
	public:
		Pair(_T1 mp1, _T2 mp2) :
			p1(mp1),
			p2(mp2) {
		}
	
		_T1 p1;
		_T2 p2;
		//SHARED_ALLOC;
	};
}
#endif

