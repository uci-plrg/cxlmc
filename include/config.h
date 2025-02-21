#ifndef _CONFIG_H
#define _CONFIG_H

#define MAX_THREADS 64
#define PAGE_SIZE 4096 //4KB
#define CACHELINE_SIZE 64
#define SHARED_MAP_SIZE 0x16000000 //256MB
#define CXL_MEM_SIZE 0x4000000 //64MB
#define SYS_LOCAL_SIZE 0x10000 //64KB
#define STACK_SIZE 0x100000 //1MB
#define MAX_EXECUTION 10000
#define MAX_CRASHES_PER_EXECUTION 1
#define SNAPSHOT_PAGES 10000
#define VALUE_NONE 0xdeadbeef
#define RANDOM_SEED 42
#define EVICT_MAX 100;


/** Inject randomness into CXL-allocated memory */
#define CXL_ALLOC_RAND 0
#define CXL_ALLOC_RAND_VAL 0xdeadbeef

/** Whether to poison cacheline of failed process */
#define MEM_POISON 0

/** Logging options */
#define DEBUG_LEVEL 1
#define VERBOSE 0


/** Define semantics of volatile memory operations. */
#define memory_order_volatile_load memory_order_acquire
#define memory_order_volatile_store memory_order_release
#endif
