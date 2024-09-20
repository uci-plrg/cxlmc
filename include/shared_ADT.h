#ifndef _SHARED_ADT_H
#define _SHARED_ADT_H

#include <list>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "allocators.h"

namespace shared {
    using string = std::basic_string<char, std::char_traits<char>, model_allocator<char>>;   
    template <typename T>
    using vector = std::vector<T, model_allocator<T>>;
    template <typename T>
    using list = std::list<T, model_allocator<T>>;
	template<typename K, typename T>
	using hashmap = std::unordered_map<K, T, std::hash<K>, std::equal_to<K>, model_allocator<std::pair<const K, T>>>;

	template<typename _T1, typename _T2>
	class Pair {
	public:
		Pair(_T1 mp1, _T2 mp2) : p1(mp1), p2(mp2) {}
		
		_T1 p1;
		_T2 p2;
		MODELALLOC;
	};
}
#endif

