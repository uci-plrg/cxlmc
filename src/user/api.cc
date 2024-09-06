#include "api.h"
#include "action.h"
#include "model.h"

void user_action(std::string s) {
    model->action(new ModelAction(PLACEHOLDER, &s));
}

uint8_t cxlmc_load8(void* addrs) {
    return (uint8_t) model->action(new ModelAction(NONATOMIC_LOAD, addrs));
}

void cxlmc_store8(void* loc, uint8_t val) {
    model->action(new ModelAction(NONATOMIC_STORE, loc, val));
	*((uint8_t *)loc) = val;
}

void cxlmc_mfence(void* loc) {
    model->action(new ModelAction(CACHE_MFENCE, loc));
}

void cxlmc_clflush(void* loc) {
    model->action(new ModelAction(CACHE_CLFLUSH, loc));
}

void* get_cxl_mapping() {
	return model->get_cxl_mapping();
}
