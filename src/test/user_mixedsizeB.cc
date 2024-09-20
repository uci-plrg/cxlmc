#include "api.h"
#include "user.h"

int main() {
	void* cxl = get_cxl_mapping();
	uint16_t read = 0;
	for (int i = 0; read != 0x0203 && i < 3; i++) {
		read = cxlmc_load16(cxl);
		printf("read %04x\n", read);
	}
    cxlmc_store8(cxl, 4);
    printf("store %d\n", 4);
	read = cxlmc_load16(cxl);
    printf("read %04x\n", read);
    return 0;
}
