#ifndef _USER_H
#define _USER_H

#include <string>

#include "model.h"

void user_action(std::string s);

extern "C" {
    void user_init(int pid, Model *m, mspace ms);
    void user_done();
}

#endif
