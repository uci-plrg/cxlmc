#ifndef _SNAPSHOT_H
#define _SNAPSHOT_H

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdio.h>

#include "model.h"

extern Model *model;

void take_snapshot();

#endif
