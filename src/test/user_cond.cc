#include <pthread.h>
#include <stdio.h>

#define T1_COUNT 5

int count;
pthread_mutex_t mutex;
pthread_cond_t cv;

void* thread1(void* arg) {
    pthread_mutex_lock(&mutex);
    printf("thread1 got lock\n");
    while (count == 0)
        pthread_cond_wait(&cv, &mutex);
    pthread_mutex_unlock(&mutex);
    return nullptr;
}

void* thread2(void* arg) {
    pthread_mutex_lock(&mutex);
    printf("thread2 got lock\n");
    if (count == 0)
        pthread_cond_broadcast(&cv);
    count++;
    pthread_mutex_unlock(&mutex);
    return nullptr;
}

int main() {
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&cv, nullptr);

    pthread_t t1[T1_COUNT], t2;
    for (int i = 0; i < T1_COUNT; i++)
        pthread_create(&t1[i], nullptr, thread1, nullptr);
    pthread_create(&t2, nullptr, thread2, nullptr);

    for (int i = 0; i < T1_COUNT; i++)
        pthread_join(t1[i], nullptr);
    pthread_join(t2, nullptr);
    return 0;
}
