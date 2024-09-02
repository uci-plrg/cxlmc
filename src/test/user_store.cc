#include "api.h"
#include "user.h"
#include <stdio.h>

int main() {
	void* cxl = get_cxl_mapping();
    for (int i = 0; i < 3; i++) {
        printf("iter %d\n", i);
        cxlmc_store(cxl, i);
    }
    cxlmc_mfence(cxl);
    return 0;
}
