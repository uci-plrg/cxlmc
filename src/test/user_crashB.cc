#include "api.h"
#include "user.h"

int main() {
	void* cxl = get_cxl_mapping();
	uint8_t read;
	int i = 0;
	do {
		read = cxlmc_load8(cxl);
		printf("read %d\n", read);
	} while (read != 3 && i++ < 5);
    cxlmc_store8(cxl, 4);
    printf("store %d\n", 4);
	read = cxlmc_load8(cxl);
    printf("read %d\n", read);
    return 0;
}
