#ifndef _USER_H
#define _USER_H

#include "model.h"

// These functions should only be called by init.cc
extern "C" {
    void user_init(int pid, Model *m, mspace ms);
    void user_done();
}

#endif
