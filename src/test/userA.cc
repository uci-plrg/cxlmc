#include <sstream>
#include <pthread.h>
#include "user.h"

thread_local int tls_i = 0;

void* thread(void* arg) {
    for (int i = 0; i < 4; i++) {
        std::ostringstream oss;
        oss << "thread iter " << i << " tls " << tls_i++;
        user_action(oss.str());
    }
    return nullptr;
}

int main() {
    pthread_t pid[2];
    for (int i = 0; i < 2; i++) {
        pthread_create(&pid[i], nullptr, &thread, nullptr);
    }

    for (int i = 0; i < 2; i++) {
        std::ostringstream oss;
        oss << "user A iter " << i;
        user_action(oss.str());
    }

    for (int i = 0; i < 2; i++) {
        pthread_join(pid[i], nullptr);
    }
    return 0;
}
