#ifndef _SHARED_ADT_H
#define _SHARED_ADT_H

#include <list>
#include <string>
#include <vector>
#include "allocators.h"

namespace shared {
    using string = std::basic_string<char, std::char_traits<char>, model_allocator<char>>;   
    template <typename T>
    using vector = std::vector<T, model_allocator<T>>;
    template <typename T>
    using list = std::list<T, model_allocator<T>>;
}
#endif

