#include "shared_ADT.h"

void * shared_malloc(size_t bytes) {
	return mspace_malloc(shared_space, bytes);
}
void shared_free(void* mem) {
	mspace_free(shared_space, mem);
}
void * shared_realloc(void* mem, size_t newsize) {
	return mspace_realloc(shared_space, mem, newsize);
}