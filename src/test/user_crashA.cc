#include "api.h"
#include "user.h"

int main() {
	void* cxl = get_cxl_mapping();
    cxlmc_store8(cxl, 1);
    cxlmc_store8(cxl, 2);
    cxlmc_clflush(cxl);
    cxlmc_store8(cxl, 3);
    model->insert_crash();
    return 0;
}
