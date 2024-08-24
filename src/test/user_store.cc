#include "user.h"
#include <stdio.h>

int main() {
    for (int i = 0; i < 3; i++) {
        printf("iter %d\n", i);
        cxlmc_store(NULL, i);
    }
    return 0;
}
