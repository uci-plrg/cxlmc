#include <time.h>
#include "model.h"

extern "C" {
    unsigned int sleep(unsigned int seconds) {
        model->action(new ModelAction(THREAD_YIELD));
        return 0;
    }

    int usleep(useconds_t useconds) {
        model->action(new ModelAction(THREAD_YIELD));
        return 0;
    }

    int nanosleep(const struct timespec *rqtp, struct timespec *rmtp) {
        model->action(new ModelAction(THREAD_YIELD));
        return 0;
    }
}