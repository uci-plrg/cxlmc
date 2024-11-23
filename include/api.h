#ifndef _API_H
#define _API_H

#include <stdint.h>
#include <string>

void user_action(std::string s);

#if __cplusplus
extern "C" {
#else
typedef int bool;
#endif

uint8_t cxlmc_load8(void* loc, const char *position);
uint16_t cxlmc_load16(void* loc, const char *position);
uint32_t cxlmc_load32(void* loc, const char *position);
uint64_t cxlmc_load64(void* loc, const char *position);

void cxlmc_store8(void* loc, uint8_t val, const char *position);
void cxlmc_store16(void* loc, uint16_t val, const char *position);
void cxlmc_store32(void* loc, uint32_t val, const char *position);
void cxlmc_store64(void* loc, uint64_t val, const char *position);

uint8_t cxlmc_volatile_load8(void* loc, const char *position);
uint16_t cxlmc_volatile_load16(void* loc, const char *position);
uint32_t cxlmc_volatile_load32(void* loc, const char *position);
uint64_t cxlmc_volatile_load64(void* loc, const char *position);

void cxlmc_volatile_store8(void* loc, uint8_t val, const char *position);
void cxlmc_volatile_store16(void* loc, uint16_t val, const char *position);
void cxlmc_volatile_store32(void* loc, uint32_t val, const char *position);
void cxlmc_volatile_store64(void* loc, uint64_t val, const char *position);

uint8_t cxlmc_atomic_load8(void* loc, int atomic_index, const char *position);
uint16_t cxlmc_atomic_load16(void* loc, int atomic_index, const char *position);
uint32_t cxlmc_atomic_load32(void* loc, int atomic_index, const char *position);
uint64_t cxlmc_atomic_load64(void* loc, int atomic_index, const char *position);

void cxlmc_atomic_store8(void* loc, uint8_t val, int atomic_index, const char *position);
void cxlmc_atomic_store16(void* loc, uint16_t val, int atomic_index, const char *position);
void cxlmc_atomic_store32(void* loc, uint32_t val, int atomic_index, const char *position);
void cxlmc_atomic_store64(void* loc, uint64_t val, int atomic_index, const char *position);

void cxlmc_sfence(void* loc);
void cxlmc_mfence(void* loc);
void cxlmc_clflush(void* loc);
void cxlmc_clflushopt(void* loc);
void* get_cxl_mapping();

#if __cplusplus
}
#endif

#endif
