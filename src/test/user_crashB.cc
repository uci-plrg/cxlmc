#include "api.h"
#include "user.h"

int main() {
	void* cxl = get_cxl_mapping();
	uint8_t read = cxlmc_load8(cxl);
	printf("read: %d\n", read);
    cxlmc_store8(cxl, 4);
    cxlmc_clflush(cxl);
    cxlmc_store8(cxl, 5);
	read = cxlmc_load8(cxl);
    printf("read: %d\n", read);
    return 0;
}
