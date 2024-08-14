#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <string>

#include "model.h"
#include "scheduler.h"

void Model::action(std::string s) {
    scheduler->wait();
    std::cout << "process " << scheduler->get_process_id() << ", " << s << std::endl;
    user_data.push_back(shared::string(s.c_str()));
    scheduler->yield();
}
