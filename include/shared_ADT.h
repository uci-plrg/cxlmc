#ifndef _SHARED_ADT_H
#define _SHARED_ADT_H

#include <vector>
#include "allocators.h"
#include "vector.h"
#include "list.h"
#include "hash_table.h"



namespace shared { 
    template <typename T>
    //using vector = Vector<T, &shared_space>;
    using vector = std::vector<T, shared_allocator<T>>;
    template <typename T>
    using list = List<T, &shared_space>;
	template<typename K, typename T>
	using hashmap = HashTable<K, T, &shared_space>;
	template<typename T>
	using hashset = HashSet<T, &shared_space>;

	template<typename _T1, typename _T2>
	class Pair {
	public:
		Pair(_T1 mp1, _T2 mp2) : p1(mp1), p2(mp2) {}
		
		_T1 p1;
		_T2 p2;
		SHAREDALLOC;
	};
}
#endif

