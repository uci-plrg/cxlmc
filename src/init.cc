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
struct user_prog {
    char *file_str_copy;
    int argc;
    char **argv;
    void(*user_init)(int pid, Model *m, mspace ms); 
    int(*user_main)(int, char**); 
    void(*user_done)();

    user_prog(char *file_str) {
        file_str_copy = (char*)malloc(sizeof(char) * (strlen(file_str) + 1));
        strcpy(file_str_copy, file_str);
        char* save_ptr = file_str_copy;
        argc = 0;
        int argv_capacity = 5;
        argv = (char**)malloc(sizeof(char*) * argv_capacity);
        char* token;
        while ((token = strtok_r(save_ptr, " ", &save_ptr)) != NULL) {
            if (argc == argv_capacity) {
                argv_capacity *= 2;
                argv = (char**)realloc(argv, sizeof(char*) * argv_capacity);
            }
            argv[argc++] = token;
        }
        if (argc == 0) {
            std::cerr << "no program path" << std::endl;
            exit(EXIT_FAILURE);
        }

        void* handle = dlopen(argv[0], RTLD_LAZY);
        if (!handle) {
            std::cerr << dlerror() << std::endl;
            exit(EXIT_FAILURE);
        }

        user_init = (void(*)(int pid, Model *m, mspace ms)) dlsym(handle, "user_init");
        if (!user_init) {
            std::cerr << dlerror() << std::endl;
            exit(EXIT_FAILURE);
        }

        user_main = (int(*)(int, char**)) dlsym(handle, "main");
        if (!user_main) {
            std::cerr << dlerror() << std::endl;
            exit(EXIT_FAILURE);
        }

        user_done = (void(*)()) dlsym(handle, "user_done");
        if (!user_done) {
            std::cerr << dlerror() << std::endl;
            exit(EXIT_FAILURE);
        }
    }
};

int main(int argc, char* argv[]) {
    int opt;
    char *ns_save = NULL;
    char *ns_load = NULL;
    int processes = 0;
    int progc = 0;
    int progv_cap = 2;
    user_prog *progv = (user_prog *) malloc(sizeof (user_prog) * progv_cap);
    int execution_num_save = 0;
    while ((opt = getopt(argc, argv, "e:f:l:n:s:")) != -1) {
        switch (opt) {
            case 'e':
                execution_num_save = atoi(optarg);
                break;
            case 'f':
                if (progc == progv_cap) {
                    progv_cap *= 2;
                    progv = (user_prog*)realloc(progv, sizeof(user_prog) * progv_cap);
                }
                progv[progc++] = user_prog(optarg);
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

    if (progc == 0) {
        std::cerr << "No file path specified" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (progc > processes) {
        std::cerr << "File paths must be no more than number of processes" << std::endl;
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
    
    if (take_snapshot() == 0) {

        void *cxl_mapping = mmap(NULL, CXL_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

        if (cxl_mapping == MAP_FAILED) {
            perror("mmap");
            exit(EXIT_FAILURE);
        }
        model->set_cxl_mapping(cxl_mapping);
        
        pid_t pid;
        int id;
        pid_t* pids = (pid_t*)malloc(sizeof(pid_t) * processes);
        for (id = 0; id < processes; id++) {
            fflush(stdout);
            fflush(stderr);
            pid = fork();
            if (pid == 0) {
                break;
            } else {
                pids[id] = pid;
                std::cout << "spawn process " << id << " with system pid " << pid << "\n";
            }
        }
 
        if (pid == 0) { 
            user_prog *prog = &progv[id %progc];
            prog->user_init(id, model, shared_space);
            prog->user_main(prog->argc, prog->argv);
            prog->user_done();
        } else {
            int status;
            pid_t child;
            while ((child = waitpid(-1, &status, 0)) != -1) {
                if(WIFSIGNALED(status)) {
                    std::cerr << "process " << child << " terminated by sig " << WTERMSIG(status) << std::endl;
                    for (int i = 0; i < progc; i++) {
                        kill(pids[i], SIGKILL);
                    }
                    exit(EXIT_FAILURE);
                } else if (WIFSTOPPED(status)) {
                    std::cerr << "process " << child << " stopped by sig " << WSTOPSIG(status) << std::endl;
                    for (int i = 0; i < progc; i++) {
                        kill(pids[i], SIGKILL);
                    }
                    exit(EXIT_FAILURE);
                }
            }

            free(pids);
            munmap(cxl_mapping, CXL_MEM_SIZE); 
        }
    } else {
        for (int i = 0; i < progc; i++) {
            free(progv[i].argv);
            free(progv[i].file_str_copy);
        }
        free(progv);
        munmap(mapping, SHARED_MAP_SIZE + reserved);
    }

    return 0;
}
