#ifndef _SHARED_MEM_H
#define _SHARED_MEM_H

#include <memory>
#include "mspace_malloc.h"

    //shared_space needs to be defined before model allocator can be used
extern mspace shared_space;
extern mspace snapshot_space;

#define MODELALLOC \
	void * operator new(size_t size) { \
				return mspace_malloc(shared_space, size); \
			} \
	void operator delete(void *p, size_t size) { \
				mspace_free(shared_space, p); \
			} \
	void * operator new[](size_t size) { \
				return mspace_malloc(shared_space, size); \
			} \
	void operator delete[](void *p, size_t size) { \
				mspace_free(shared_space, p); \
			} \
	void * operator new(size_t size, void *p) {	/* placement new */ \
				return p; \
			}

template <typename T> 
class model_allocator { 
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
		typedef model_allocator<U> other;
	};

	// return address of values
	pointer address(reference value) const {
		return &value;
	}
	const_pointer address(const_reference value) const {
		return &value;
	}

    //constructors and destructors
    model_allocator() throw() {}

	model_allocator(const model_allocator&) throw() {}

	template<typename T2>
    model_allocator(const model_allocator<T2> &alloc) throw() {}
 
    ~model_allocator() throw() {}

	//operators
	bool operator!=(const model_allocator<T> other) {return true;} 

	// Allocate memory for n objects of type T 
    pointer allocate(size_t n) {
        //mspace_malloc_stats(shared_space);
        void *addr = mspace_malloc(shared_space, n * sizeof(T));
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
        mspace_free(shared_space, p);
    }
};

template <typename T> 
class snapshot_allocator { 
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
		typedef snapshot_allocator<U> other;
	};

	// return address of values
	pointer address(reference value) const {
		return &value;
	}
	const_pointer address(const_reference value) const {
		return &value;
	}

    //constructors and destructors
    snapshot_allocator() throw() {}

	snapshot_allocator(const snapshot_allocator&) throw() {}

	template<typename T2>
    snapshot_allocator(const snapshot_allocator<T2> &alloc) throw() {}
 
    ~snapshot_allocator() throw() {}

	//operators
	bool operator!=(const snapshot_allocator<T> other) {return true;} 
	
	// Allocate memory for n objects of type T 
    pointer allocate(size_t n) {
        //mspace_malloc_stats(shared_space);
        void *addr = mspace_malloc(snapshot_space, n * sizeof(T));
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
        mspace_free(snapshot_space, p);
    }
};

#endif
