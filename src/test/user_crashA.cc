#include "api.h"
#include "user.h"

int main() {
	void* cxl = get_cxl_mapping();
    cxlmc_store8(cxl, 1, NULL);
    printf("store %d\n", 1);
    cxlmc_store8(cxl, 2, NULL);
    printf("store %d\n", 2);
    cxlmc_clflush(cxl);
    printf("clflush\n");
    cxlmc_store8(cxl, 3, NULL);
    printf("store %d\n", 3);
    return 0;
}
