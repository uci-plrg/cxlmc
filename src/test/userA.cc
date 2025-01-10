#include <pthread.h>
#include "api.h"
#include "user.h"
#include <stdio.h>

thread_local int tls_i = 0;
pthread_mutex_t mutex;

void* thread(void* arg) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < 10; i++) {
        printf("thread iter %d, tls %d\n", i, tls_i++);
    }
    int* a = new int(97);
    pthread_mutex_unlock(&mutex);
    return a;
}

int main() {
    printf("main %d tls %d\n", process_id, tls_i++);

    pthread_mutex_init(&mutex, nullptr);

    pthread_mutex_lock(&mutex);
    pthread_t pid[2];
    for (int i = 0; i < 2; i++) {
        pthread_create(&pid[i], nullptr, &thread, nullptr);
    }
    for (int i = 0; i < 2; i++) {
		printf("user A iter %d\n", i);
    }
    pthread_mutex_unlock(&mutex);

    for (int i = 0; i < 2; i++) {
        int* ptr;
        pthread_join(pid[i], (void**)&ptr);
        printf("join returned %d\n", *ptr);
        delete ptr;
    }

    printf("main %d tls %d\n", process_id, tls_i++);
    return 0;
}
