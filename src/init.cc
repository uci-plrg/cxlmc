#include <iostream>
#include <unistd.h>
#include <sys/mman.h>
#include <atomic>
#include <sys/wait.h>
#include <errno.h>
#include <dlfcn.h>
#include <cstring>

#include "scheduler.h"
#include "model.h"
#include "allocators.h"
#include "config.h"

int main(int argc, char* argv[]) {
    int processes = 16;
    if (argc > 1) {
        processes = atoi(argv[1]);
    }

    if (processes < 1) {
        std::cerr << "Less than 1 processs" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    int user_progs = argc - 2;

    if (user_progs < 1) {
        std::cerr << "Less than 1 user program" << std::endl;
        exit(EXIT_FAILURE);
    }

    int reserved = sizeof(Scheduler) + sizeof(Model);
	size_t total_map_size = SHARED_MAP_SIZE + CXL_MEM_SIZE + reserved;
    void* mapping = mmap(NULL, total_map_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (mapping == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    
    //shared space needs to be initialized before scheduler and model
    shared_space = create_mspace_with_base(mapping, SHARED_MAP_SIZE, 1);

    if (!shared_space) { 
        perror("create_mspace_with_base");
        return 1;
    }

	void *cxl_mapping = (char *)mapping + SHARED_MAP_SIZE;	
    Scheduler *scheduler = new ((char *)mapping + SHARED_MAP_SIZE + CXL_MEM_SIZE) Scheduler(processes);
	model = new((char*)mapping + SHARED_MAP_SIZE + CXL_MEM_SIZE + sizeof(Scheduler)) Model(scheduler, cxl_mapping);

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
        char* cur_prog_cpy = (char*)malloc(sizeof(char) * (strlen(cur_prog) + 1));
        strcpy(cur_prog_cpy, cur_prog);
        char* save_ptr = cur_prog_cpy;
        int user_argc = 0;
        int argv_capacity = 5;
        char** user_argv = (char**)malloc(sizeof(char*) * argv_capacity);
        char* token;
        while ((token = strtok_r(save_ptr, " ", &save_ptr)) != NULL) {
            if (user_argc == argv_capacity) {
                argv_capacity *= 2;
                user_argv = (char**)realloc(user_argv, sizeof(char*) * argv_capacity);
            }
            user_argv[user_argc++] = token;
        }
        if (user_argc == 0) {
            std::cerr << "no program path" << std::endl;
            exit(1);
        }

        void* handle = dlopen(user_argv[0], RTLD_LAZY);
        if (!handle) {
            std::cerr << dlerror() << std::endl;
            exit(1);
        }

        void(*user_init)(int pid, Model *m, mspace ms) = (void(*)(int pid, Model *m, mspace ms)) dlsym(handle, "user_init");
        if (!user_init) {
            std::cerr << dlerror() << std::endl;
            exit(1);
        }

        int(*user_main)(int, char**) = (int(*)(int, char**)) dlsym(handle, "main");
        if (!user_main) {
            std::cerr << dlerror() << std::endl;
            exit(1);
        }

         void(*user_done)() = (void(*)()) dlsym(handle, "user_done");
         if (!user_done) {
             std::cerr << dlerror() << std::endl;
             exit(1);
         }
        
        user_init(id, model, shared_space);
        user_main(user_argc, user_argv);
        user_done();
        free(user_argv);
        free(cur_prog_cpy);
    } else {
        int status;
        while (waitpid(-1, &status, 0) != -1) {
            if(WIFSIGNALED(status))
                std::cerr << "child terminated by sig " << WTERMSIG(status) << std::endl;
            else if (WIFSTOPPED(status))
                std::cerr << "child stopped by sig " << WSTOPSIG(status) << std::endl;
        }
        
		//scheduler->~Scheduler();
		//model->~Model();
		//mspace_malloc_stats(shared_space);
        munmap(mapping, total_map_size);
    }

    return 0;
}
