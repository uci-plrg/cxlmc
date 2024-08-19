#ifndef _SHARED_MEM_H
#define _SHARED_MEM_H

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

namespace shared {
    //shared_space needs to be defined before model allocator can be used
    extern mspace shared_space;

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
}

#endif
