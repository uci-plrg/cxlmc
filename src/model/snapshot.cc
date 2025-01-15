#include <iostream>

#include "snapshot.h"

pid_t take_snapshot() {
    int execution_num = model->get_execution_num();
    while (true) {
		std::cout << "-------------------------- execution "<< execution_num << "--------------------------\n";

		pid_t forkedID;

		forkedID = fork();

		if (0 == forkedID) {
            return forkedID;
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
            std::cerr << "execution " << execution_num << " terminated by sig " << WTERMSIG(status) << std::endl;
            model->terminate_early();
        }
        
        if (!model->wait_for_next_execution(++execution_num)) {	
            return forkedID;
		}

    }
}
