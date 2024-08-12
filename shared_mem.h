#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <atomic>
#include <iostream>
#include <string>
#include <vector>

extern "C" {
    /* MSPACE */
    /*
      mspace is an opaque type representing an independent
      region of space that supports mspace_malloc, etc.
    */
    typedef void * mspace;
   
    extern void * mspace_malloc(mspace msp, size_t bytes);
    extern void mspace_free(mspace msp, void* mem);
    extern void * mspace_realloc(mspace msp, void* mem, size_t newsize);
    extern void * mspace_calloc(mspace msp, size_t n_elements, size_t elem_size);
    extern void * mspace_memalign(mspace msp, size_t alignment, size_t bytes);
    extern mspace create_mspace_with_base(void* base, size_t capacity, int locked);
    extern mspace create_mspace(size_t capacity, int locked);
    extern void mspace_malloc_stats(mspace msp);
}

mspace shared_space;

namespace shared {
    template <typename T> class model_allocator: std::allocator<T> { 
    public: 
        typedef T value_type; 
        // Constructor 
        model_allocator() noexcept {} 
        // Allocate memory for n objects of type T 
        T* allocate(std::size_t n) 
        { 
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

    using string = std::basic_string<char, std::char_traits<char>, model_allocator<char>>;   
    template <typename T>
    using vector = std::vector<T, model_allocator<T>>;
}

       
typedef struct shared_data {
    // metadata
    int thread_count;
    std::atomic_int active;
    std::atomic_int *thread_status;

    // user data
    shared::vector<shared::string> user_strings;
} shared_data_t;

shared_data_t *sd;
