#ifndef _MODEL_H
#define _MODEL_H

#include <string>
#include "shared_data.h"

void action(std::string s);

extern "C" {
    void fork_init(int id, shared_data_t* d, mspace ms);
    void done();
}

#endif
