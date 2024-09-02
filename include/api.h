#ifndef _API_H
#define _API_H

#include <string>

void user_action(std::string s);
void cxlmc_store(void* loc, uint64_t val);
void cxlmc_mfence(void* loc);
void* get_cxl_mapping();

#endif
