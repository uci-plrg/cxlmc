#ifndef _CXL_ALLOCATOR_H
#define _CXL_ALLOCATOR_H

#include <memory>
#include "mspace_malloc.h"

//cxl space is not defined by default, to use CXL allocators, initialize through api call
extern mspace cxl_space;

#define CXLALLOC \
	void * operator new(size_t size) { \
				return mspace_malloc(cxl_space, size); \
			} \
	void operator delete(void *p, size_t size) { \
				mspace_free(cxl_space, p); \
			} \
	void * operator new[](size_t size) { \
				return mspace_malloc(cxl_space, size); \
			} \
	void operator delete[](void *p, size_t size) { \
				mspace_free(cxl_space, p); \
			} \
	void * operator new(size_t size, void *p) {	/* placement new */ \
				return p; \
			}

template <typename T> 
class cxl_allocator { 
public:// type definitions
	typedef T value_type;
	typedef T*       pointer;
	typedef const T* const_pointer;
	typedef T&       reference;
	typedef const T& const_reference;
	typedef size_t size_type;
	typedef size_t difference_type;

	// rebind allocator to type U
	template <class U>
	struct rebind {
		typedef cxl_allocator<U> other;
	};

	// return address of values
	pointer address(reference value) const {
		return &value;
	}
	const_pointer address(const_reference value) const {
		return &value;
	}

    //constructors and destructors
    cxl_allocator() throw() {}

	cxl_allocator(const cxl_allocator&) throw() {}

	template<typename T2>
    cxl_allocator(const cxl_allocator<T2> &alloc) throw() {}
 
    ~cxl_allocator() throw() {}

	//operators
	bool operator!=(const cxl_allocator<T> other) {return true;} 

	// Allocate memory for n objects of type T 
    pointer allocate(size_t n) {
        //mspace_malloc_stats(cxl_space);
        void *addr = mspace_malloc(cxl_space, n * sizeof(T));
        if (!addr) {
            std::__throw_bad_alloc();
        }
        return static_cast<pointer>(addr);
    }

	// initialize elements of allocated storage p with value value
	void construct(pointer p, const T& value) {
		// initialize memory with placement new
		new((void*)p)T(value);
	}

	// destroy elements of initialized storage p
	void destroy(pointer p) {
		// destroy objects by calling their destructor
		p->~T();
	}
    // Deallocate memory 
    void deallocate(pointer p, size_t n) {
        mspace_free(cxl_space, p);
    }
};

#endif
