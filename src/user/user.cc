#include "user.h"

//process local data
mspace shared::shared_space;
Model *model;

void user_action(std::string s) {
    model->action(s);
}

void user_init(int pid, Model *m, mspace ms) {
    model = m;
    shared::shared_space = ms;
    model->get_scheduler()->process_init(pid);
}

void user_done() {
    model->get_scheduler()->finalize();
}
