#ifndef _SNAPSHOT_ADT_H
#define _SNAPSHOT_ADT_H

#include <list>
#include <string>
#include <vector>
#include "allocators.h"

namespace snapshot {
    using string = std::basic_string<char, std::char_traits<char>, snapshot_allocator<char>>;   
    template <typename T>
    using vector = std::vector<T, snapshot_allocator<T>>;
    template <typename T>
    using list = std::list<T, snapshot_allocator<T>>;
}
#endif

