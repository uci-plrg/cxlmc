#include <sstream>
#include <pthread.h>
#include "api.h"
#include "user.h"
#include <stdio.h>

thread_local int tls_i = 0;

void* thread(void* arg) {
    for (int i = 0; i < 10; i++) {
        std::ostringstream oss;
        oss << "thread iter " << i << " tls " << tls_i++;
        user_action(oss.str());

        if (thread_id == 1 && i == 4) {
            int* a = new int(24);
            pthread_exit(a);
        }
    }
    int* a = new int(97);
    return a;
}

int main() {
    printf("main %d tls %d\n", process_id, tls_i++);

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
        int* ptr;
        pthread_join(pid[i], (void**)&ptr);
        printf("join returned %d\n", *ptr);
        delete ptr;
    }

    printf("main %d tls %d\n", process_id, tls_i++);
    return 0;
}
