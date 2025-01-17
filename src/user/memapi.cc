#include <string.h>
#include <dlfcn.h>
#include <inttypes.h>
#include <api.h>
#include <config.h>

#include "model.h" 

extern Model *model;
extern bool inside_model;

int FILLBYTE=0;
void * persistentMemoryRegion;
void * (*volatile memcpy_real)(void * dst, const void *src, size_t n) = NULL;
void * (*volatile memmove_real)(void * dst, const void *src, size_t len) = NULL;
void (*volatile bzero_real)(void * dst, size_t len) = NULL;
void * (*volatile memset_real)(void * dst, int c, size_t len) = NULL;
char * (*volatile strcpy_real)(char * dst, const char *src) = NULL;


const void * altRegion = NULL;
uint64_t altSize = 0;

void init_memory_ops() {
	if (!memcpy_real) {
		memcpy_real = (void * (*)(void * dst, const void *src, size_t n)) 1;
		memcpy_real = (void * (*)(void * dst, const void *src, size_t n))dlsym(RTLD_NEXT, "memcpy");
	}
	if (!memmove_real) {
		memmove_real = (void * (*)(void * dst, const void *src, size_t n)) 1;
		memmove_real = (void * (*)(void * dst, const void *src, size_t n))dlsym(RTLD_NEXT, "memmove");
	}

	if (!memset_real) {
		memset_real = (void * (*)(void * dst, int c, size_t n)) 1;
		memset_real = (void * (*)(void * dst, int c, size_t n))dlsym(RTLD_NEXT, "memset");
	}

	if (!strcpy_real) {
		strcpy_real = (char * (*)(char * dst, const char *src)) 1;
		strcpy_real = (char * (*)(char * dst, const char *src))dlsym(RTLD_NEXT, "strcpy");
	}
	if (!bzero_real) {
		bzero_real = (void (*)(void * dst, size_t len)) 1;
		bzero_real = (void (*)(void * dst, size_t len))dlsym(RTLD_NEXT, "bzero");
	}
}

const char * memmovestring = "memmove";
void *cxlmc_memmove(void *dst, const void *src, size_t n) {
	for(unsigned i=0;i<n;) {
		if ((((uintptr_t)src+i)&7)==0 && (((uintptr_t)dst+i)&7)==0 && (i + 8) <= n) {
			uint64_t val = cxlmc_load64(((char *) src) + i, memmovestring);
			cxlmc_store64(((char *) dst)+i, val, memmovestring);
			i+=8;
		} else if ((((uintptr_t)src+i)&3)==0 && (((uintptr_t)dst+i)&3)==0 && (i + 4) <= n) {
			uint32_t val = cxlmc_load32(((char *) src) + i, memmovestring);
			cxlmc_store32(((char *) dst)+i, val, memmovestring);
			i+=4;
		} else if ((((uintptr_t)src+i)&1)==0 && (((uintptr_t)dst+i)&1)==0 && (i + 2) <= n) {
			uint16_t val = cxlmc_load16(((char *) src) + i, memmovestring);
			cxlmc_store16(((char *) dst)+i, val, memmovestring);
			i+=2;
		} else {
			uint8_t val = cxlmc_load8(((char *) src) + i, memmovestring);
			cxlmc_store8(((char *) dst)+i, val, memmovestring);
			i=i+1;
		}
	}
	return dst;
}

const char * memstring = "memcpy";
void *cxlmc_memcpy(void *dst, const void *src, size_t n) {
	for(unsigned i=0;i<n;) {
		if ((((uintptr_t)src+i)&7)==0 && (((uintptr_t)dst+i)&7)==0 && (i + 8) <= n) {
			uint64_t val = cxlmc_load64(((char *) src) + i, memstring);
			cxlmc_store64(((char *) dst)+i, val, memstring);
			i+=8;
		} else if ((((uintptr_t)src+i)&3)==0 && (((uintptr_t)dst+i)&3)==0 && (i + 4) <= n) {
			uint32_t val = cxlmc_load32(((char *) src) + i, memstring);
			cxlmc_store32(((char *) dst)+i, val, memstring);
			i+=4;
		} else if ((((uintptr_t)src+i)&1)==0 && (((uintptr_t)dst+i)&1)==0 && (i + 2) <= n) {
			uint16_t val = cxlmc_load16(((char *) src) + i, memstring);
			cxlmc_store16(((char *) dst)+i, val, memstring);
			i+=2;
		} else {
			uint8_t val = cxlmc_load8(((char *) src) + i, memstring);
			cxlmc_store8(((char *) dst)+i, val, memstring);
			i=i+1;
		}
	}
	return dst;
}

const char * memsetstring = "memset";
void *cxlmc_memset(void *dst, int c, size_t n) {
	uint8_t cs = c&0xff;
	for(unsigned i=0;i<n;) {
		if ((((uintptr_t)dst+i)&7)==0 && (i + 8) <= n) {
			uint16_t cs2 = cs << 8 | cs;
			uint64_t cs3 = cs2 << 16 | cs2;
			uint64_t cs4 = cs3 << 32 | cs3;
			cxlmc_store64(((char *) dst)+i, cs4, memsetstring);
			i+=8;
		} else if ((((uintptr_t)dst+i)&3)==0 && (i + 4) <= n) {
			uint16_t cs2 = cs << 8 | cs;
			uint32_t cs3 = cs2 << 16 | cs2;
			cxlmc_store32(((char *) dst)+i, cs3, memsetstring);
			i+=4;
		} else if ((((uintptr_t)dst+i)&1)==0 && (i + 2) <= n) {
			uint16_t cs2 = cs << 8 | cs;
			cxlmc_store16(((char *) dst)+i, cs2, memsetstring);
			i+=2;
		} else {
			cxlmc_store8(((char *) dst)+i, cs, memsetstring);
			i=i+1;
		}
	}
	return dst;
}

void * memcpy(void * dst, const void * src, size_t n) {
	if (mem_is_cxl(dst,1) && !inside_model) {
		return cxlmc_memcpy(dst, src, n);
	} else {
		if (((uintptr_t)memcpy_real) < 2) {
			for(unsigned i=0;i<n;i++) {
				((volatile char *)dst)[i] = ((char *)src)[i];
			}
			return dst;
		}
		return memcpy_real(dst, src, n);
	}
}

void * memmove(void *dst, const void *src, size_t n) {
	if (mem_is_cxl(dst,1) && !inside_model) {
		return cxlmc_memmove(dst, src, n);
	} else {
		if (((uintptr_t)memmove_real) < 2) {
			if (((uintptr_t)dst) < ((uintptr_t)src))
				for(unsigned i=0;i<n;i++) {
					((volatile char *)dst)[i] = ((char *)src)[i];
				}
			else
				for(unsigned i=n;i!=0; ) {
					i--;
					((volatile char *)dst)[i] = ((char *)src)[i];
				}
			return dst;
		}
		return memmove_real(dst, src, n);
	}
}

void * realmemset(void *dst, int c, size_t n) {
	if (((uintptr_t)memset_real) < 2) {
		//stuck in dynamic linker alloc cycle...
		for(size_t s=0;s<n;s++) {
			((volatile char *)dst)[s] = (char) c;
		}
		return dst;
	}
	return memset_real(dst, c, n);
}

void * memset(void *dst, int c, size_t n) {
	if (mem_is_cxl(dst,1) && !inside_model) {
		return cxlmc_memset(dst, c, n);
	} else {
		return realmemset(dst, c, n);
	}
}


const char * bzerostring = "bzero";
void bzero(void *dst, size_t n) {
	if (mem_is_cxl(dst,1) && !inside_model) {
		for(unsigned i=0;i<n;) {
			if ((((uintptr_t)dst+i)&7)==0 && (i + 8) <= n) {
				cxlmc_store64(((char *) dst)+i, 0, bzerostring);
				i+=8;
			} else if ((((uintptr_t)dst+i)&3)==0 && (i + 4) <= n) {
				cxlmc_store32(((char *) dst)+i, 0, bzerostring);
				i+=4;
			} else if (((((uintptr_t)dst+i))&1)==0 && (i + 2) <= n) {
				cxlmc_store16(((char *) dst)+i, 0, bzerostring);
				i+=2;
			} else {
				cxlmc_store8(((char *) dst)+i, 0, bzerostring);
				i=i+1;
			}
		}

	} else {
		if (((uintptr_t)bzero_real) < 2) {
			for(size_t s=0;s<n;s++) {
				((volatile char *)dst)[s] = 0;
			}
			return;
		}
		bzero_real(dst, n);
	}
}

const char * strcpystring = "strcpy";

char * strcpy(char *dst, const char *src) {
	if (mem_is_cxl(dst,1)) {
		size_t n = 0;
		while(src[n] != '\0') n++;
		for(unsigned i=0;i<n;) {
			if ((((uintptr_t)src+i)&7)==0 && (((uintptr_t)dst+i)&7)==0 && (i + 8) <= n) {
				uint64_t val = cxlmc_load64(((char *) src) + i, strcpystring);
				cxlmc_store64(((char *) dst)+i, val, strcpystring);
				i+=8;
			} else if ((((uintptr_t)src+i)&3)==0 && (((uintptr_t)dst+i)&3)==0 && (i + 4) <= n) {
				uint32_t val = cxlmc_load32(((char *) src) + i, strcpystring);
				cxlmc_store32(((char *) dst)+i, val, strcpystring);
				i+=4;
			} else if ((((uintptr_t)src+i)&1)==0 && (((uintptr_t)dst+i)&1)==0 && (i + 2) <= n) {
				uint16_t val = cxlmc_load16(((char *) src) + i, strcpystring);
				cxlmc_store16(((char *) dst)+i, val, strcpystring);
				i+=2;
			} else {
				uint8_t val = cxlmc_load8(((char *) src) + i, strcpystring);
				cxlmc_store8(((char *) dst)+i, val, strcpystring);
				i=i+1;
			}
		}
		return dst;
	} else {
		if ((uintptr_t) strcpy_real < 2) {
			size_t n = 0;
			while(src[n] != '\0') n++;
			for(unsigned i=0;i<n;i++) {
				((volatile char *)dst)[i] = ((char *)src)[i];
			}
			return dst;
		} else {
			return strcpy_real(dst, src);
		}
	}
}
