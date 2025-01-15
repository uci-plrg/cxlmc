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
#include "snapshot.h"
#include "allocators.h"
#include "config.h"

int main(int argc, char* argv[]) {
    int opt;
    char *ns_save = NULL;
    char *ns_load = NULL;
    int processes = 0;
    char *file_path = NULL;
    int execution_num_save = 0;
    while ((opt = getopt(argc, argv, "e:f:l:n:s:")) != -1) {
        switch (opt) {
            case 'e':
                execution_num_save = atoi(optarg);
                break;
            case 'f':
                file_path = optarg;
                break;
            case 'l':
                ns_load = optarg;
                break;
            case 'n':
                processes = atoi(optarg);
                break;
            case 's':
                ns_save = optarg;
                break;
        }
    }
    
    if (processes < 1) {
        std::cerr << "Less than 1 processs" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (file_path == NULL) {
        std::cerr << "No file path specified" << std::endl;
        exit(EXIT_FAILURE);
    }

    int reserved = sizeof(Scheduler) + sizeof(Model);
    void* mapping = mmap(NULL, SHARED_MAP_SIZE + reserved, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
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

    Scheduler *scheduler = new ((char *)mapping + SHARED_MAP_SIZE) Scheduler(processes);
	model = new((char*)mapping + SHARED_MAP_SIZE + sizeof(Scheduler)) Model(scheduler);

    if (ns_save != NULL) {
        model->save_execution(execution_num_save, ns_save);
    }

    if (ns_load != NULL) {
        model->load_nodestack(ns_load);
    }
    
	char* cur_prog = file_path;
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
    
	if (take_snapshot() == 0) {

        void *cxl_mapping = mmap(NULL, CXL_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (cxl_mapping == MAP_FAILED) {
            perror("mmap");
            return 1;
        }
        model->set_cxl_mapping(cxl_mapping);
        
        pid_t pid;
        int id;
        for (id = 0; id < processes; id++) {
            pid = fork();
            if (pid == 0) {
                break;
            } else {
				std::cout << "spawn process " << id << " with system pid " << pid << "\n";
			}
        }
 
        if (pid == 0) { 
            user_init(id, model, shared_space);
            user_main(user_argc, user_argv);
            user_done();
        } else {
            int status;
			pid_t child;
            while ((child = waitpid(-1, &status, 0)) != -1) {
                if(WIFSIGNALED(status))
                    std::cerr << "process " << child << " terminated by sig " << WTERMSIG(status) << std::endl;
                else if (WIFSTOPPED(status))
                    std::cerr << "process " << child << " stopped by sig " << WSTOPSIG(status) << std::endl;
            }
 
            munmap(cxl_mapping, CXL_MEM_SIZE); 
        }
	} else {
        free(user_argv);
        free(cur_prog_cpy);
        munmap(mapping, SHARED_MAP_SIZE + reserved);
	}

    return 0;
}
