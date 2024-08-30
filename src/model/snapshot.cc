#include <iostream>

#include "snapshot.h"

void take_snapshot() {
    while (true) {
		pid_t forkedID;

		forkedID = fork();

		if (0 == forkedID)
            return;
            
        int status;
        while(waitpid(-1, &status, 0) < 0) {
            /* waitpid() may be interrupted */
			if (errno != EINTR) {
				perror("waitpid");
				exit(EXIT_FAILURE);
			}
        }

        if(WIFSIGNALED(status)) {
            std::cerr << "child terminated by sig " << WTERMSIG(status) << std::endl;
            exit(EXIT_FAILURE);
        }

        std::cout << "restart process " << process_id << std::endl;
        
        if (!model->should_rollback())
            exit(EXIT_SUCCESS);
    }
}
