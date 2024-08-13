#include <iostream>
#include <unistd.h>
#include <sys/mman.h>
#include <atomic>
#include <sys/wait.h>
#include <errno.h>
#include <dlfcn.h>

#include "shared_data.h"

#define MAP_SIZE 8192

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
     
    shared_data *sd = (shared_data_t*) mapping;
    sd->process_count = processes;
    shared::shared_space = create_mspace_with_base((char *)mapping + sizeof(shared_data_t), MAP_SIZE - sizeof(shared_data_t), 1);
    if (!shared::shared_space) { 
        perror("create_mspace_with_base");
        return 1;
    }
    sd->process_status = (std::atomic_int*)mspace_calloc(shared::shared_space, processes, sizeof(std::atomic_int));
   
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

        void(*fork_init)(int, shared_data_t*, mspace) = (void(*)(int, shared_data_t*, mspace)) dlsym(handle, "fork_init");
        if (!fork_init) {
            std::cerr << dlerror() << std::endl;
            exit(1);
        }

        int(*fork_main)() = (int(*)()) dlsym(handle, "main");
        if (!fork_main) {
            std::cerr << dlerror() << std::endl;
            exit(1);
        }

        void(*done)() = (void(*)()) dlsym(handle, "done");
        if (!done) {
            std::cerr << dlerror() << std::endl;
            exit(1);
        }

        fork_init(id, sd, shared::shared_space);
        fork_main();
        done();
    } else {
        int status;
        while (waitpid(-1, &status, 0) != -1) {
            if(WIFSIGNALED(status))
                std::cerr << "child terminated by sig " << WTERMSIG(status) << std::endl;
        }

        std::cout << "user strings: ";
        for (auto s: sd->user_strings)
            std::cout << s << " ";
        std::cout << std::endl;
        munmap(mapping, MAP_SIZE);
    }

    return 0;
}
