#include <sstream>
#include <pthread.h>
#include "user.h"

void* thread(void* arg) {
    for (int i = 0; i < 4; i++) {
        std::ostringstream oss;
        oss << "user A iter (in thread) " << i;
        user_action(oss.str());
    }
    return nullptr;
}

int main() {
    pthread_t pid;
    pthread_create(&pid, nullptr, &thread, nullptr);

    for (int i = 0; i < 2; i++) {
        std::ostringstream oss;
        oss << "user A iter " << i;
        user_action(oss.str());
    }

    pthread_join(pid, nullptr);
    return 0;
}
