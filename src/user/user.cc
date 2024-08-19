#include "user.h"
#include "snapshot.h"

void user_action(std::string s) {
    model->action(s);
}

void user_init(int pid, Model *m, mspace ms) {
    model = m;
    shared::shared_space = ms;
    take_snapshot();
    model->get_scheduler()->process_init(pid);
    real_init_all();
}

void user_done() {
    model->finishExecution();
}
