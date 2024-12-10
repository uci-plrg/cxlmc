#include <sys/mman.h>

#include "user.h"
#include "snapshot.h"
#include "mspace_malloc.h"

void init_memory_ops();

void user_exit() {
	model->finish_execution();
}

void user_init(process_id_t pid, Model *m, mspace ms) {
	srand(42 + pid);
    model = m;
    shared_space = ms;
    model->get_scheduler()->process_init(pid);
    real_init_all();
    init_memory_ops();
    take_snapshot();
    
    void* mapping = mmap(NULL, SNAPSHOT_PAGES * PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (mapping == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    snapshot_space = create_mspace_with_base(mapping, SNAPSHOT_PAGES * PAGE_SIZE, 1);

    if (!snapshot_space) { 
        perror("create_mspace_with_base");
        exit(1);
    }

    atexit(user_exit);
    model->get_scheduler()->wait();
}

void user_done() {
    model->get_scheduler()->process_shutdown();
}
