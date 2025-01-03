#ifndef _ALLOCATORS_H
#define _ALLOCATORS_H

#include "assert.h"
#include "types.h"
#include "mspace_malloc.h"

    //shared_space needs to be defined before shared allocator can be used
extern mspace shared_space;
extern mspace snapshot_space;

#define TEMPLATEALLOC \
	void * operator new(size_t size, std::align_val_t al) { \
				return mspace_memalign(*msp, (size_t)al, size); \
			} \
	void * operator new(size_t size) { \
				void* addr = mspace_malloc(*msp, size); \
				if (!addr) \
					assert(false && "bad alloc"); \
				return addr; \
			} \
	void operator delete(void *p, size_t size) { \
				mspace_free(*msp, p); \
			} \
	void * operator new[](size_t size) { \
				void* addr = mspace_malloc(*msp, size); \
				if (!addr) \
					assert(false && "bad alloc"); \
				return addr; \
			} \
	void operator delete[](void *p, size_t size) { \
				mspace_free(*msp, p); \
			} \
	void * operator new(size_t size, void *p) {	/* placement new */ \
				return p; \
			}

#define SHAREDALLOC \
	void * operator new(size_t size, std::align_val_t al) { \
				return mspace_memalign(shared_space, (size_t)al, size); \
			} \
	void * operator new(size_t size) { \
				void* addr = mspace_malloc(shared_space, size); \
				if (!addr) \
					assert(false && "bad alloc"); \
				return addr; \
			} \
	void operator delete(void *p, size_t size) { \
				mspace_free(shared_space, p); \
			} \
	void * operator new[](size_t size) { \
				void* addr = mspace_malloc(shared_space, size); \
				if (!addr) \
					assert(false && "bad alloc"); \
				return addr; \
			} \
	void operator delete[](void *p, size_t size) { \
				mspace_free(shared_space, p); \
			} \
	void * operator new(size_t size, void *p) {	/* placement new */ \
				return p; \
			}

template <typename T> 
class shared_allocator { 
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
		typedef shared_allocator<U> other;
	};

	// return address of values
	pointer address(reference value) const {
		return &value;
	}
	const_pointer address(const_reference value) const {
		return &value;
	}

    //constructors and destructors
    shared_allocator() throw() {}

	shared_allocator(const shared_allocator&) throw() {}

	template<typename T2>
    shared_allocator(const shared_allocator<T2> &alloc) throw() {}
 
    ~shared_allocator() throw() {}

	//operators
	bool operator!=(const shared_allocator<T> other) {return true;} 

	// Allocate memory for n objects of type T 
    pointer allocate(size_t n) {
        //mspace_malloc_stats(shared_space);
        void *addr = mspace_malloc(shared_space, n * sizeof(T));
        if (!addr)
            assert(false && "bad alloc");
        return static_cast<pointer>(addr);
    }

	template<typename... _Args>
	// initialize elements of allocated storage p with value value
	void construct(pointer p, _Args&&... args) {
		// initialize memory with placement new
		new((void*)p)T(static_cast<_Args&&>(args)...);
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
        if (!addr)
            assert(false && "bad alloc");
        return static_cast<pointer>(addr);
    }

	template<typename... _Args>
	// initialize elements of allocated storage p with value value
	void construct(pointer p, _Args&&... args) {
		// initialize memory with placement new
		new((void*)p)T(static_cast<_Args&&>(args)...);
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
