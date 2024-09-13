#ifndef _COMMON_H
#define _COMMON_H
#include "config.h"


#ifdef DEBUG_LEVEL
#define DEBUG(fmt, ...) do { printf("*** %15s:%-4d %25s() *** " fmt, __FILE__, __LINE__, __func__, ## __VA_ARGS__); } while (0)
#else
#define DEBUG(fmt, ...)
#endif

#endif
