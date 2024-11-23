#include "api.h"
#include "action.h"
#include "model.h"

void user_action(std::string s) {
    model->action(new ModelAction(PLACEHOLDER, &s));
}

memory_order orders[7] = {
	memory_order_relaxed, memory_order_consume, memory_order_acquire,
	memory_order_release, memory_order_acq_rel, memory_order_seq_cst,
};

#define CXLMCLOAD(size) \
	uint ## size ## _t cxlmc_load ## size (void* loc, const char* position) { \
		return (uint ## size ##_t) model->action(new ModelAction(NONATOMIC_LOAD, loc, VALUE_NONE, memory_order_seq_cst, size >> 3, position)); \
	}

CXLMCLOAD(8)
CXLMCLOAD(16)
CXLMCLOAD(32)
CXLMCLOAD(64)

#define CXLMCSTORE(size) \
	void cxlmc_store ## size (void* loc, uint ## size ## _t val, const char* position) { \
    model->action(new ModelAction(NONATOMIC_STORE, loc, val, memory_order_seq_cst, size >> 3, position)); \
	*((uint ## size ##_t *)loc) = val; \
}

CXLMCSTORE(8)
CXLMCSTORE(16)
CXLMCSTORE(32)
CXLMCSTORE(64)

#define VOLATILELOAD(size) \
	uint ## size ## _t cxlmc_volatile_load ## size (void* loc, const char *position) { \
		return (uint ## size ##_t) model->action(new ModelAction(ATOMIC_LOAD, loc, VALUE_NONE, memory_order_volatile_load, size >> 3, position)); \
	}

VOLATILELOAD(8)
VOLATILELOAD(16)
VOLATILELOAD(32)
VOLATILELOAD(64)

#define VOLATILESTORE(size) \
	void cxlmc_volatile_store ## size (void* loc, uint ## size ## _t val, const char *position) { \
    model->action(new ModelAction(ATOMIC_STORE, loc, val, memory_order_seq_cst, size >> 3, position)); \
	*((uint ## size ##_t *)loc) = val; \
}

VOLATILESTORE(8)
VOLATILESTORE(16)
VOLATILESTORE(32)
VOLATILESTORE(64)

#define ATOMICLOAD(size) \
	uint ## size ## _t cxlmc_atomic_load ## size (void* loc, int atomic_index, const char *position) { \
		return (uint ## size ##_t) model->action(new ModelAction(ATOMIC_LOAD, loc, VALUE_NONE, orders[atomic_index], size >> 3, position)); \
	}

ATOMICLOAD(8)
ATOMICLOAD(16)
ATOMICLOAD(32)
ATOMICLOAD(64)

#define ATOMICSTORE(size) \
	void cxlmc_atomic_store ## size (void* loc, uint ## size ## _t val, int atomic_index, const char *position) { \
    model->action(new ModelAction(ATOMIC_STORE, loc, val, orders[atomic_index], size >> 3, position)); \
	*((uint ## size ##_t *)loc) = val; \
}

ATOMICSTORE(8)
ATOMICSTORE(16)
ATOMICSTORE(32)
ATOMICSTORE(64)

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

void* get_cxl_mapping() {
	return model->get_cxl_mapping();
}
