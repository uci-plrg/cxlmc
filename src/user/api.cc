#include "api.h"
#include "action.h"
#include "model.h"
#include <cstring>

void user_action(std::string s) {
    char* copy = (char*) mspace_malloc(shared_space, sizeof(char) * (s.length() + 1));
    strcpy(copy, s.c_str());
    model->action(new ModelAction(PLACEHOLDER, copy));
}

uint8_t cxlmc_load8(void* addrs) {
    return (uint8_t) model->action(new ModelAction(NONATOMIC_LOAD, addrs));
}

void cxlmc_store8(void* loc, uint8_t val) {
    model->action(new ModelAction(NONATOMIC_STORE, loc, val));
	*((uint8_t *)loc) = val;
}

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
