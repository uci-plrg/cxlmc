#include <iostream>

#include "snapshot.h"

void take_snapshot() {
    int execution_num = 1;
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
        
        if (!model->wait_for_next_execution(++execution_num))
            exit(EXIT_SUCCESS);

        std::cout << "restart process " << process_id << std::endl;
    }
}
