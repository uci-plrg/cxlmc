#ifndef _API_H
#define _API_H

#include <stdint.h>
#include <string>

void user_action(std::string s);

uint8_t cxlmc_load8(void* loc);
uint16_t cxlmc_load16(void* loc);
uint32_t cxlmc_load32(void* loc);
uint64_t cxlmc_load64(void* loc);

void cxlmc_store8(void* loc, uint8_t val);
void cxlmc_store16(void* loc, uint16_t val);
void cxlmc_store32(void* loc, uint32_t val);
void cxlmc_store64(void* loc, uint64_t val);

void cxlmc_sfence(void* loc);
void cxlmc_mfence(void* loc);
void cxlmc_clflush(void* loc);
void cxlmc_clflushopt(void* loc);
void* get_cxl_mapping();

#endif
