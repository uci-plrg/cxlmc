#ifndef _API_H
#define _API_H

#include <stdint.h>
#include <string>

void user_action(std::string s);
uint8_t cxlmc_load8(void* loc);
void cxlmc_store8(void* loc, uint8_t val);
void cxlmc_mfence(void* loc);
void cxlmc_clflush(void* loc);
void* get_cxl_mapping();

#endif
