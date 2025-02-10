#include "api.h"
#include "action.h"
#include "cxl_allocator.h"
#include "model.h"

memory_order orders[7] = {
	memory_order_relaxed, memory_order_consume, memory_order_acquire,
	memory_order_release, memory_order_acq_rel, memory_order_seq_cst,
};

#define CXLMCLOAD(size) \
	uint ## size ## _t cxlmc_load ## size (void* loc, const char* position) { \
		if (mem_is_cxl(loc)) \
			return (uint ## size ##_t) model->action(new ModelAction(NONATOMIC_LOAD, loc, VALUE_NONE, memory_order_relaxed, size >> 3, position), false); \
		return *(uint ## size ##_t*) loc; \
	}

CXLMCLOAD(8)
CXLMCLOAD(16)
CXLMCLOAD(32)
CXLMCLOAD(64)

#define CXLMCSTORE(size) \
	void cxlmc_store ## size (void* loc, uint ## size ## _t val, const char* position) { \
	if (mem_is_cxl(loc)) \
		model->action(new ModelAction(NONATOMIC_STORE, loc, val, memory_order_relaxed, size >> 3, position), false); \
	*((uint ## size ##_t *)loc) = val; \
}

CXLMCSTORE(8)
CXLMCSTORE(16)
CXLMCSTORE(32)
CXLMCSTORE(64)

#define VOLATILELOAD(size) \
	uint ## size ## _t cxlmc_volatile_load ## size (void* loc, const char *position) { \
		if (mem_is_cxl(loc)) \
			return (uint ## size ##_t) model->action(new ModelAction(ATOMIC_LOAD, loc, VALUE_NONE, memory_order_volatile_load, size >> 3, position), false); \
		return *(uint ## size ##_t*) loc; \
	}

VOLATILELOAD(8)
VOLATILELOAD(16)
VOLATILELOAD(32)
VOLATILELOAD(64)

#define VOLATILESTORE(size) \
	void cxlmc_volatile_store ## size (void* loc, uint ## size ## _t val, const char *position) { \
	if (mem_is_cxl(loc)) \
		model->action(new ModelAction(ATOMIC_STORE, loc, val, memory_order_volatile_store, size >> 3, position), false); \
	*((uint ## size ##_t *)loc) = val; \
}

VOLATILESTORE(8)
VOLATILESTORE(16)
VOLATILESTORE(32)
VOLATILESTORE(64)

#define CXLMCATOMICINT(size)                                              \
	void cxlmc_atomic_init ## size (void* loc, uint ## size ## _t val, const char * position) { \
		if (mem_is_cxl(loc)) \
			model->action(new ModelAction(ATOMIC_INIT, loc, val, memory_order_relaxed, size>>3, position)); \
		*((uint ## size ## _t *)loc) = val; \
    }

CXLMCATOMICINT(8)
CXLMCATOMICINT(16)
CXLMCATOMICINT(32)
CXLMCATOMICINT(64)

#define ATOMICLOAD(size) \
	uint ## size ## _t cxlmc_atomic_load ## size (void* loc, int atomic_index, const char *position) { \
		if (mem_is_cxl(loc)) \
			return (uint ## size ##_t) model->action(new ModelAction(ATOMIC_LOAD, loc, VALUE_NONE, orders[atomic_index], size >> 3, position)); \
		return *(uint ## size ##_t*) loc; \
	}

ATOMICLOAD(8)
ATOMICLOAD(16)
ATOMICLOAD(32)
ATOMICLOAD(64)

#define ATOMICSTORE(size) \
	void cxlmc_atomic_store ## size (void* loc, uint ## size ## _t val, int atomic_index, const char *position) { \
	if (mem_is_cxl(loc)) \
		model->action(new ModelAction(ATOMIC_STORE, loc, val, orders[atomic_index], size >> 3, position)); \
	*((uint ## size ##_t *)loc) = val; \
}

ATOMICSTORE(8)
ATOMICSTORE(16)
ATOMICSTORE(32)
ATOMICSTORE(64)


uint64_t model_rmw_read_action(void *addrs, int atomic_index, const char *position, int size) {
	return model->action(new ModelAction(ATOMIC_RMWR, addrs, 0, orders[atomic_index], size, position));
}

void model_rmw_action(void *addrs, uint64_t val, int atomic_index, const char * position, int size) {
	model->action(new ModelAction(ATOMIC_RMW, addrs, val, orders[atomic_index], size, position));
}

#define _ATOMIC_RMW_(__op__, size, addr, val, atomic_index, position) \
	{ \
		uint ## size ## _t _old; \
		if (mem_is_cxl(addr)) \
			_old = model_rmw_read_action(addr, atomic_index, position, size>>3); \
		else \
			_old = *(uint ## size ## _t *) addr; \
		uint ## size ## _t _copy = _old; \
		uint ## size ## _t _val = val; \
		_copy __op__ _val; \
		if (mem_is_cxl(addr)) \
			model_rmw_action(addr, (uint64_t) _copy, atomic_index, position, size>>3); \
		else \
			*(uint ## size ## _t *) addr = _copy; \
		return _old; \
	}

// cxlmc atomic exchange
#define CXLMCATOMICEXCHANGE(size) \
	uint ## size ## _t cxlmc_atomic_exchange ## size(void* addr, uint ## size ## _t val, int atomic_index, const char * position) { \
		_ATOMIC_RMW_( =, size, addr, val, atomic_index, position); \
	}

CXLMCATOMICEXCHANGE(8)
CXLMCATOMICEXCHANGE(16)
CXLMCATOMICEXCHANGE(32)
CXLMCATOMICEXCHANGE(64)


// cxlmc atomic fetch add
#define CXLMCATOMICADD(size) \
	uint ## size ## _t cxlmc_atomic_fetch_add ## size(void* addr, uint ## size ## _t val, int atomic_index, const char * position) { \
		_ATOMIC_RMW_( +=, size, addr, val, atomic_index, position); \
	}

CXLMCATOMICADD(8)
CXLMCATOMICADD(16)
CXLMCATOMICADD(32)
CXLMCATOMICADD(64)

// cxlmc atomic fetch sub
#define CXLMCATOMICSUB(size) \
	uint ## size ## _t cxlmc_atomic_fetch_sub ## size(void* addr, uint ## size ## _t val, int atomic_index, const char * position) { \
		_ATOMIC_RMW_( -=, size, addr, val, atomic_index, position); \
	}

CXLMCATOMICSUB(8)
CXLMCATOMICSUB(16)
CXLMCATOMICSUB(32)
CXLMCATOMICSUB(64)

// cxlmc atomic fetch and
#define CXLMCATOMICAND(size) \
	uint ## size ## _t cxlmc_atomic_fetch_and ## size(void* addr, uint ## size ## _t val, int atomic_index, const char * position) { \
		_ATOMIC_RMW_( &=, size, addr, val, atomic_index, position); \
	}

CXLMCATOMICAND(8)
CXLMCATOMICAND(16)
CXLMCATOMICAND(32)
CXLMCATOMICAND(64)

// cxlmc atomic fetch or
#define CXLMCATOMICOR(size) \
	uint ## size ## _t cxlmc_atomic_fetch_or ## size(void* addr, uint ## size ## _t val, int atomic_index, const char * position) { \
		_ATOMIC_RMW_( |=, size, addr, val, atomic_index, position); \
	}

CXLMCATOMICOR(8)
CXLMCATOMICOR(16)
CXLMCATOMICOR(32)
CXLMCATOMICOR(64)

// cxlmc atomic fetch xor
#define CXLMCATOMICXOR(size) \
	uint ## size ## _t cxlmc_atomic_fetch_xor ## size(void* addr, uint ## size ## _t val, int atomic_index, const char * position) { \
		_ATOMIC_RMW_( ^=, size, addr, val, atomic_index, position); \
	}

CXLMCATOMICXOR(8)
CXLMCATOMICXOR(16)
CXLMCATOMICXOR(32)
CXLMCATOMICXOR(64)

void model_rmw_cas_fail_action(void *addrs, int atomic_index, const char *position, int size) {
	model->action(new ModelAction(ATOMIC_CAS_FAILED, addrs, 0, orders[atomic_index], size, position));
}

// cxlmc atomic compare and exchange
// In order to accomodate the LLVM PASS, the return values are not true or false.

#define _ATOMIC_CMPSWP_WEAK_ _ATOMIC_CMPSWP_
#define _ATOMIC_CMPSWP_(size, addr, expected, desired, atomic_index_succ, atomic_index_fail, position) \
	{ \
		uint ## size ## _t _desired = desired; \
		uint ## size ## _t _expected = expected; \
		uint ## size ## _t _old; \
		bool is_cxl = mem_is_cxl(addr); \
		if (is_cxl) \
			_old = model_rmw_read_action(addr, atomic_index_succ, position, size>>3); \
		else \
			_old = *(uint ## size ## _t *) addr; \
		if (_old == _expected) { \
			if (is_cxl) \
				model_rmw_action(addr, (uint64_t) _desired, atomic_index_succ, position, size>>3); \
			*(uint ## size ## _t*) addr = _desired; \
			return _expected; \
		} else { \
			if (is_cxl) \
				model_rmw_cas_fail_action(addr, atomic_index_fail, position, size>>3); \
			return _old; \
		} \
	}

// atomic_compare_exchange version 1: the CmpOperand (corresponds to expected)
// extracted from LLVM IR is an integer type.
#define CDSATOMICCASV1(size) \
	uint ## size ## _t cxlmc_atomic_compare_exchange ## size ## _v1(void* addr, uint ## size ## _t expected, uint ## size ## _t desired, int atomic_index_succ, int atomic_index_fail, const char *position) { \
		_ATOMIC_CMPSWP_(size, addr, expected, desired, atomic_index_succ, atomic_index_fail, position); \
	}

CDSATOMICCASV1(8)
CDSATOMICCASV1(16)
CDSATOMICCASV1(32)
CDSATOMICCASV1(64)

// atomic_compare_exchange version 2
#define CDSATOMICCASV2(size) \
	bool cxlmc_atomic_compare_exchange ## size ## _v2(void* addr, uint ## size ## _t* expected, uint ## size ## _t desired, int atomic_index_succ, int atomic_index_fail, const char *position) { \
		uint ## size ## _t ret = cxlmc_atomic_compare_exchange ## size ## _v1(addr, *expected, desired, atomic_index_succ, atomic_index_fail, position); \
		if (ret == *expected) { \
			return true; \
		} else { \
			*expected = ret; \
			return false; \
		} \
	}

CDSATOMICCASV2(8)
CDSATOMICCASV2(16)
CDSATOMICCASV2(32)
CDSATOMICCASV2(64)

void cxlmc_sfence(void* loc) {
    model->action(new ModelAction(CACHE_SFENCE, loc));
}

void cxlmc_mfence(void* loc) {
    model->action(new ModelAction(CACHE_MFENCE, loc));
}

void cxlmc_clflush(void* loc) {
    model->action(new ModelAction(CACHE_CLFLUSH, loc));
}

void cxlmc_clflushopt(void* loc) {
    model->action(new ModelAction(CACHE_CLFLUSHOPT, loc));
}

void cxlmc_clwb(void* loc) {
	cxlmc_clflushopt(loc);
}

void* get_cxl_mapping() {
	return model->get_cxl_mapping();
}

bool mem_is_cxl(const void *address) {
	if (!model)
		return false;
	return model->mem_is_cxl(address);  
}

void init_cxl_space(size_t offset, size_t size) {
	assert (offset + size <= CXL_MEM_SIZE);
	cxl_space = create_mspace_with_base((char *)model->get_cxl_mapping() + offset, size, 1);
}

process_id_t get_process_count() {
	return model->get_scheduler()->get_process_count();
}

process_id_t get_process_id() {
	return process_id;
}

process_id_t get_crashed_process_count() {
	return model->get_crashed_process_count();
}

bool is_process_crashed(process_id_t process_id) {
	return model->is_crashed(process_id);
}

inline ExtPtr make_ext_ptr(void *ptr) {
	return ExtPtr{ptr, model->mem_is_cxl(ptr) ? -1: process_id};
}

int mutex_lock_crashed(pthread_mutex_t *p_mutex) {
	ExtPtr ep = make_ext_ptr(p_mutex);
    auto mutex_map = model->get_mutex_map();
	if (mutex_map->find(ep) == mutex_map->end()) {
		pthread_mutex_init(p_mutex, NULL);
	}

	Mutex* m = mutex_map->at(ep);

	if (m != NULL) {
		return m->lock() + 1;
	} else {
		return 0;
	}
}