#ifndef _SHARED_ADT_H
#define _SHARED_ADT_H

#include "allocators.h"
#include "vector.h"
#include "list.h"
#include "hash_table.h"

namespace shared { 
    template <typename T>
    using vector = Vector<T, &shared_space>;
    template <typename T>
    using list = List<T, &shared_space>;
	template<typename K, typename T, size_t (*H)(K)=default_hash_function>
	using hashmap = HashTable<K, T, &shared_space, H>;
	template<typename T>
	using hashset = HashSet<T, &shared_space>;

	template<typename _T1, typename _T2>
	class Pair {
	public:
		Pair(_T1 mp1, _T2 mp2) : first(mp1), second(mp2) {}
		
		_T1 first;
		_T2 second;
		SHAREDALLOC;

		inline bool operator==(const Pair<_T1, _T2> &other) const { return first == other.first && second == other.second; }

		static inline size_t hash(Pair<_T1, _T2> p) {
			return default_hash_function(p.first) * 31 + default_hash_function(p.second);
		}
	};

}
#endif

