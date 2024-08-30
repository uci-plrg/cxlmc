#ifndef _USER_H
#define _USER_H

#include <string>

#include "model.h"

void user_action(std::string s);
void cxlmc_store(void* loc, uint64_t val);
void* get_cxl_mapping();

extern "C" {
    void user_init(int pid, Model *m, mspace ms);
    void user_done();
}

#endif
