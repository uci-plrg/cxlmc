#ifndef _SHARED_ADT_H
#define _SHARED_ADT_H

#include <list>
#include <string>
#include <vector>
#include <unordered_map>
#include "allocators.h"

namespace shared {
    using string = std::basic_string<char, std::char_traits<char>, model_allocator<char>>;   
    template <typename T>
    using vector = std::vector<T, model_allocator<T>>;
    template <typename T>
    using list = std::list<T, model_allocator<T>>;
	template<typename K, typename T>
	using hashmap = std::unordered_map<K, T, std::hash<K>, std::equal_to<K>, model_allocator<std::pair<const K, T>>>;
}
#endif

