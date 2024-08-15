#ifndef _SNAPSHOT_H
#define _SNAPSHOT_H

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdio.h>

#include <iostream>

#include "model.h"

extern Model *model;

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

        std::cout << "restart process " << process_id << std::endl;
        
        if (!model->should_rollback_again())
            exit(EXIT_SUCCESS);
    }
}

#endif
