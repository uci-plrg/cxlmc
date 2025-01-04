#include <iostream>

#include "snapshot.h"

bool is_fork = false;

void take_snapshot() {
    int execution_num = 1;
    while (true) {
		pid_t forkedID;

		forkedID = fork();

		if (0 == forkedID) {
            is_fork = true;
            return;
        }
            
        int status;
        while(waitpid(-1, &status, 0) < 0) {
            /* waitpid() may be interrupted */
			if (errno != EINTR) {
				perror("waitpid");
				exit(EXIT_FAILURE);
			}
        }

        if(WIFSIGNALED(status)) {
            std::cerr << "process " << process_id << " terminated by sig " << WTERMSIG(status) << std::endl;
            model->terminate_early();
        }
        
        if (!model->wait_for_next_execution(++execution_num)) {	
            exit(EXIT_SUCCESS);
		}

        std::cout << "restart process " << process_id << std::endl;
    }
}
