#include "api.h"
#include "action.h"
#include "model.h"

void user_action(std::string s) {
    model->action(new ModelAction(PLACEHOLDER, &s));
}

#define VOLATILELOAD(size) \
	uint ## size ## _t cxlmc_load ## size (void* loc) { \
		return (uint ## size ##_t) model->action(new ModelAction(NONATOMIC_LOAD, loc, VALUE_NONE, size >> 3)); \
	}

VOLATILELOAD(8)
VOLATILELOAD(16)
VOLATILELOAD(32)
VOLATILELOAD(64)

#define VOLATILESTORE(size) \
	void cxlmc_store ## size (void* loc, uint ## size ## _t val) { \
    model->action(new ModelAction(NONATOMIC_STORE, loc, val, size >> 3)); \
	*((uint ## size ##_t *)loc) = val; \
}

VOLATILESTORE(8)
VOLATILESTORE(16)
VOLATILESTORE(32)
VOLATILESTORE(64)

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
