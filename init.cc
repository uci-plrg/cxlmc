#include <iostream>
#include <unistd.h>
#include <sys/mman.h>
#include <atomic>
#include <sys/wait.h>
#include <errno.h>
#include <dlfcn.h>

#include "scheduler.h"
#include "shared_data.h"
#include "allocators.h"
#include "config.h"

mspace shared::shared_space;

int main(int argc, char* argv[]) {
    int processes = 16;
    if (argc > 1) {
        processes = atoi(argv[1]);
    }

    if (processes < 1) {
        std::cerr << "Less than 1 processs" << std::endl;
    }
    
    int user_progs = argc - 2;

    if (user_progs < 1) {
        std::cerr << "Less than 1 user program" << std::endl;
    }

    void* mapping = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (mapping == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    
    int reserved = sizeof(Scheduler) + sizeof(shared::vector<shared::string>);
    //shared space needs to be initialized first
    shared::shared_space = create_mspace_with_base((char *)mapping + reserved, MAP_SIZE - reserved, 1);
    if (!shared::shared_space) { 
        perror("create_mspace_with_base");
        return 1;
    }
    Scheduler *scheduler = new (mapping) Scheduler(processes);
    shared::vector<shared::string> *user_data = new((char*)mapping + sizeof(Scheduler)) shared::vector<shared::string>();

    pid_t pid;
    int id;
    for (id = 0; id < processes; id++) {
        pid = fork();
        if (pid == 0) {
            break;
        }
    }

    if (pid == 0) {
        char* cur_prog = argv[2+(id%user_progs)];
        void* handle = dlopen(cur_prog, RTLD_LAZY);
        if (!handle) {
            std::cerr << dlerror() << std::endl;
            exit(1);
        }

        void(*model_init)(int, Scheduler*, mspace, shared::vector<shared::string>*) = (void(*)(int, Scheduler*, mspace, shared::vector<shared::string>*)) dlsym(handle, "model_init");
        if (!model_init) {
            std::cerr << dlerror() << std::endl;
            exit(1);
        }

        int(*user_main)() = (int(*)()) dlsym(handle, "main");
        if (!user_main) {
            std::cerr << dlerror() << std::endl;
            exit(1);
        }

        void(*model_done)() = (void(*)()) dlsym(handle, "model_done");
        if (!model_done) {
            std::cerr << dlerror() << std::endl;
            exit(1);
        }

        model_init(id, scheduler, shared::shared_space, user_data);
        user_main();
        model_done();
    } else {
        int status;
        while (waitpid(-1, &status, 0) != -1) {
            if(WIFSIGNALED(status))
                std::cerr << "child terminated by sig " << WTERMSIG(status) << std::endl;
        }
        std::cout << "user data: ";
        for (auto s: *user_data)
            std::cout << s << " ";
        std::cout << std::endl;
        munmap(mapping, MAP_SIZE);
    }

    return 0;
}
