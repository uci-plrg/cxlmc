#include <unistd.h>
#include <time.h>

#include "api.h"
#include "user.h"

int main() {
    sleep(1);
    usleep(1000);
    struct timespec ts = { 1, 20 };
    nanosleep(&ts, &ts);
    
    for (int i = 0; i < 4; i++) {
		printf("user A iter %d\n", i);

        model->insert_crash();
    }
    
    return 0;
}
