#include <sys/mman.h>

#include "action.h"
#include "user.h"
#include "snapshot.h"
#include "mspace_malloc.h"


void user_action(std::string s) {
    model->action(new ModelAction(PLACEHOLDER, &s));
}

void cxlmc_store(void* loc, uint64_t val) {
    model->action(new ModelAction(STORE, loc, val));
}

void user_init(process_id_t pid, Model *m, mspace ms) {
    model = m;
    shared_space = ms;
    model->get_scheduler()->process_init(pid);
    real_init_all();
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

    model->get_scheduler()->wait();
}

void user_done() {
    model->finishExecution();
}
