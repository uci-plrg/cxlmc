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
        if(waitpid(-1, &status, 0) == -1) {
            std::cerr << "waitpid error " << strerror(errno) << std::endl;
			exit(EXIT_FAILURE);
        }

        if (WIFEXITED(status)) {
            int ret;
            if ((ret = WEXITSTATUS(status) != 0)) {
                std::cerr << "execution " << execution_num << " exited with error value " << WTERMSIG(status) << std::endl;
                model->terminate_early();
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
