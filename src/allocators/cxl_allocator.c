#include <string.h>
#include "config.h"
#include "cxl_allocator.h"

void * cxl_malloc(size_t bytes) {
	void* ret = mspace_malloc(cxl_space, bytes);
#if CXL_ALLOC_RAND
	memset(ret, CXL_ALLOC_RAND_VAL, bytes);
#endif
	return ret;
}

void cxl_free(void* mem) {
	mspace_free(cxl_space, mem);
}

void * cxl_realloc(void* mem, size_t newsize) {
	return mspace_realloc(cxl_space, mem, newsize);
}

void * cxl_calloc(size_t n_elements, size_t elem_size) {
	return mspace_calloc(cxl_space, n_elements, elem_size);
}

void * cxl_memalign(size_t alignment, size_t bytes) {
	void* ret = mspace_memalign(cxl_space, alignment, bytes);
#if CXL_ALLOC_RAND
	memset(ret, CXL_ALLOC_RAND_VAL, bytes);
#endif
	return ret;
}
