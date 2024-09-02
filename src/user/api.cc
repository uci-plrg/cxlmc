#include "api.h"
#include "action.h"
#include "model.h"

void user_action(std::string s) {
    model->action(new ModelAction(PLACEHOLDER, &s));
}

void cxlmc_store(void* loc, uint64_t val) {
    model->action(new ModelAction(STORE, loc, val));
	*((uint64_t *)loc) = val;
}

void cxlmc_mfence(void* loc) {
    model->action(new ModelAction(MFENCE, loc));
}

void* get_cxl_mapping() {
	return model->get_cxl_mapping();
}
