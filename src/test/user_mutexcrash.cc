#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <atomic>

#include "cxl_allocator.h"
#include "api.h"
#include "user.h"

struct CXL_Root {
	std::atomic<pthread_mutex_t *> mutex;
};
CXL_Root *root = NULL;

int main() {
    init_cxl_space(sizeof(CXL_Root), CXL_MEM_SIZE - sizeof(CXL_Root));
	root = (CXL_Root*) get_cxl_mapping();
    // ht
    pthread_mutex_t* mutex;
    auto mutex_ptr = root->mutex.load();
    if (mutex_ptr == NULL) {
        mutex = (pthread_mutex_t*)cxl_malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(mutex, NULL);
        if (!root->mutex.compare_exchange_strong(mutex_ptr, mutex)) {
            pthread_mutex_destroy(mutex);
            cxl_free(mutex);
            mutex = root->mutex.load();
        }
    } else {
        mutex = mutex_ptr;
    }
    
    pthread_mutex_lock(mutex);
    for (int i = 0; i < 4; i++) {
		printf("user A iter %d\n", i);
        model->insert_crash();
    }
    pthread_mutex_unlock(mutex);
    
    return 0;
}
