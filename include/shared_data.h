#ifndef _SHARED_DATA_H
#define _SHARED_DATA_H

#include "allocators.h"

namespace shared {
    using string = std::basic_string<char, std::char_traits<char>, model_allocator<char>>;   
    template <typename T>
    using vector = std::vector<T, model_allocator<T>>;
}
#endif

