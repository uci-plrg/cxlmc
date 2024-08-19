#ifndef _SHARED_MEM_H
#define _SHARED_MEM_H

#include <string>
#include <vector>
#include "mspace_malloc.h"

    //shared_space needs to be defined before model allocator can be used
extern mspace shared_space;
extern mspace snapshot_space;

template <typename T> 
class model_allocator { 
public:
    typedef T value_type;
    // Constructor 
    model_allocator() noexcept {}
 
    // Allocate memory for n objects of type T 
    T* allocate(std::size_t n) {
        //mspace_malloc_stats(shared_space);
        void *addr = mspace_malloc(shared_space, n * sizeof(T));
        if (!addr) {
            std::__throw_bad_alloc();
        }
        return static_cast<T*>(addr);
    }

    // Deallocate memory 
    void deallocate(T* p, std::size_t n) noexcept
    {
        mspace_free(shared_space, p);
    }
};

template <typename T> 
class snapshot_allocator { 
public:
    typedef T value_type;
    // Constructor 
    snapshot_allocator() noexcept {}
 
    // Allocate memory for n objects of type T 
    T* allocate(std::size_t n) {
        //mspace_malloc_stats(shared_space);
        void *addr = mspace_malloc(snapshot_space, n * sizeof(T));
        if (!addr) {
            std::__throw_bad_alloc();
        }
        return static_cast<T*>(addr);
    }

    // Deallocate memory 
    void deallocate(T* p, std::size_t n) noexcept
    {
        mspace_free(snapshot_space, p);
    }
};

#endif
