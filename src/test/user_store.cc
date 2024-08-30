#include "user.h"
#include <stdio.h>

int main() {
    for (int i = 0; i < 3; i++) {
        printf("iter %d\n", i);
        cxlmc_store(get_cxl_mapping(), i);
    }
    return 0;
}
