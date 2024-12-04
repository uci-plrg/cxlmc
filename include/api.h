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

void cxlmc_atomic_init8(void * obj, uint8_t val, const char * position);
void cxlmc_atomic_init16(void * obj, uint16_t val, const char * position);
void cxlmc_atomic_init32(void * obj, uint32_t val, const char * position);
void cxlmc_atomic_init64(void * obj, uint64_t val, const char * position);

uint8_t cxlmc_atomic_load8(void* loc, int atomic_index, const char *position);
uint16_t cxlmc_atomic_load16(void* loc, int atomic_index, const char *position);
uint32_t cxlmc_atomic_load32(void* loc, int atomic_index, const char *position);
uint64_t cxlmc_atomic_load64(void* loc, int atomic_index, const char *position);

void cxlmc_atomic_store8(void* loc, uint8_t val, int atomic_index, const char *position);
void cxlmc_atomic_store16(void* loc, uint16_t val, int atomic_index, const char *position);
void cxlmc_atomic_store32(void* loc, uint32_t val, int atomic_index, const char *position);
void cxlmc_atomic_store64(void* loc, uint64_t val, int atomic_index, const char *position);

// cxlmc atomic exchange
uint8_t cxlmc_atomic_exchange8(void* addr, uint8_t val, int atomic_index, const char * position);
uint16_t cxlmc_atomic_exchange16(void* addr, uint16_t val, int atomic_index, const char * position);
uint32_t cxlmc_atomic_exchange32(void* addr, uint32_t val, int atomic_index, const char * position);
uint64_t cxlmc_atomic_exchange64(void* addr, uint64_t val, int atomic_index, const char * position);
// cxlmc atomic fetch add
uint8_t  cxlmc_atomic_fetch_add8(void* addr, uint8_t val, int atomic_index, const char * position);
uint16_t cxlmc_atomic_fetch_add16(void* addr, uint16_t val, int atomic_index, const char * position);
uint32_t cxlmc_atomic_fetch_add32(void* addr, uint32_t val, int atomic_index, const char * position);
uint64_t cxlmc_atomic_fetch_add64(void* addr, uint64_t val, int atomic_index, const char * position);
// cxlmc atomic fetch sub
uint8_t  cxlmc_atomic_fetch_sub8(void* addr, uint8_t val, int atomic_index, const char * position);
uint16_t cxlmc_atomic_fetch_sub16(void* addr, uint16_t val, int atomic_index, const char * position);
uint32_t cxlmc_atomic_fetch_sub32(void* addr, uint32_t val, int atomic_index, const char * position);
uint64_t cxlmc_atomic_fetch_sub64(void* addr, uint64_t val, int atomic_index, const char * position);
// cxlmc atomic fetch and
uint8_t cxlmc_atomic_fetch_and8(void* addr, uint8_t val, int atomic_index, const char * position);
uint16_t cxlmc_atomic_fetch_and16(void* addr, uint16_t val, int atomic_index, const char * position);
uint32_t cxlmc_atomic_fetch_and32(void* addr, uint32_t val, int atomic_index, const char * position);
uint64_t cxlmc_atomic_fetch_and64(void* addr, uint64_t val, int atomic_index, const char * position);
// cxlmc atomic fetch or
uint8_t cxlmc_atomic_fetch_or8(void* addr, uint8_t val, int atomic_index, const char * position);
uint16_t cxlmc_atomic_fetch_or16(void* addr, uint16_t val, int atomic_index, const char * position);
uint32_t cxlmc_atomic_fetch_or32(void* addr, uint32_t val, int atomic_index, const char * position);
uint64_t cxlmc_atomic_fetch_or64(void* addr, uint64_t val, int atomic_index, const char * position);
// cxlmc atomic fetch xor
uint8_t cxlmc_atomic_fetch_xor8(void* addr, uint8_t val, int atomic_index, const char * position);
uint16_t cxlmc_atomic_fetch_xor16(void* addr, uint16_t val, int atomic_index, const char * position);
uint32_t cxlmc_atomic_fetch_xor32(void* addr, uint32_t val, int atomic_index, const char * position);
uint64_t cxlmc_atomic_fetch_xor64(void* addr, uint64_t val, int atomic_index, const char * position);

// cxlmc atomic compare and exchange (strong)
uint8_t cxlmc_atomic_compare_exchange8_v1(void* addr, uint8_t expected, uint8_t desire, int atomic_index_succ, int atomic_index_fail, const char *position);
uint16_t cxlmc_atomic_compare_exchange16_v1(void* addr, uint16_t expected, uint16_t desire, int atomic_index_succ, int atomic_index_fail, const char *position);
uint32_t cxlmc_atomic_compare_exchange32_v1(void* addr, uint32_t expected, uint32_t desire, int atomic_index_succ, int atomic_index_fail, const char *position);
uint64_t cxlmc_atomic_compare_exchange64_v1(void* addr, uint64_t expected, uint64_t desire, int atomic_index_succ, int atomic_index_fail, const char *position);

bool cxlmc_atomic_compare_exchange8_v2(void* addr, uint8_t* expected, uint8_t desired, int atomic_index_succ, int atomic_index_fail, const char *position);
bool cxlmc_atomic_compare_exchange16_v2(void* addr, uint16_t* expected, uint16_t desired, int atomic_index_succ, int atomic_index_fail, const char *position);
bool cxlmc_atomic_compare_exchange32_v2(void* addr, uint32_t* expected, uint32_t desired, int atomic_index_succ, int atomic_index_fail, const char *position);
bool cxlmc_atomic_compare_exchange64_v2(void* addr, uint64_t* expected, uint64_t desired, int atomic_index_succ, int atomic_index_fail, const char *position);

void cxlmc_sfence(void* loc);
void cxlmc_mfence(void* loc);
void cxlmc_clflush(void* loc);
void cxlmc_clflushopt(void* loc);
void* get_cxl_mapping();

#if __cplusplus
}
#endif

#endif
