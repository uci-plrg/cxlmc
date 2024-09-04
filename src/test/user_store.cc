#include "api.h"
#include "user.h"
#include <stdio.h>

int main() {
	void* cxl = get_cxl_mapping();
    for (int i = 0; i < 4; i++) {
        printf("iter %d\n", i);
		void *addr = (char*)cxl + i * CACHELINE_SIZE;
        cxlmc_store(addr , i);
		if (i%2==0)
			cxlmc_clflush(addr);
    }
    cxlmc_mfence(cxl);
    return 0;
}
