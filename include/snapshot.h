#ifndef _SNAPSHOT_H
#define _SNAPSHOT_H

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdio.h>

#include "model.h"

pid_t take_snapshot();

#endif
