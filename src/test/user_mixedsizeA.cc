#include "api.h"
#include "user.h"

int main() {
	//int a[] {1,2,3,4};
	//printf("%d, %d, %d, %d\n", cxlmc_load32(&a[0], NULL), cxlmc_load32(&a[1], NULL),cxlmc_load32(&a[2], NULL),cxlmc_load32(&a[3], NULL));
	//exit(0);
	void* cxl = get_cxl_mapping();
    cxlmc_store8(cxl, 2, NULL);
    printf("store %d\n", 2);
    cxlmc_store8((char *)cxl + 1, 2, NULL);
    printf("store %d, offset 8\n", 2);
    cxlmc_clflush(cxl);
    printf("clflush\n");
    cxlmc_store8(cxl, 3, NULL);
    printf("store %d\n", 3);
}
