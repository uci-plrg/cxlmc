#include "api.h"
#include "user.h"

int main() {
	void* cxl = get_cxl_mapping();
	uint8_t read = 0;
	for (int i = 0; read != 3 && i < 3; i++) {
		read = cxlmc_load8(cxl);
		printf("read %d\n", read);
	}
    cxlmc_store8(cxl, 4);
    printf("store %d\n", 4);
	read = cxlmc_load8(cxl);
    printf("read %d\n", read);
    return 0;
}
