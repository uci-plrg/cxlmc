#ifndef _SHARED_DATA_H
#define _SHARED_DATA_H

#include <atomic>

#include "allocators.h"

namespace shared {
    using string = std::basic_string<char, std::char_traits<char>, model_allocator<char>>;   
    template <typename T>
    using vector = std::vector<T, model_allocator<T>>;
}

typedef struct shared_data {
    // metadata
    int process_count;
    std::atomic_int active;
    std::atomic_int *process_status;

    // user data
    shared::vector<shared::string> user_strings;
} shared_data_t;

#endif
