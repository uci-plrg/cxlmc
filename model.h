#ifndef _MODEL_H
#define _MODEL_H

#include <string>
#include "scheduler.h"
#include "shared_data.h"

void action(std::string s);

extern "C" {
    void model_init(int process_id, Scheduler *s, mspace ms, shared::vector<shared::string> *us);
    void model_done();
}

#endif
