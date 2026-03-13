#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "PM.h"

int main(int argc, char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Not enough arguments");
    }

    int max = atoi(argv[1]);
    int shm_id = atoi(argv[2]);

    PM_run(max, shm_id);

    return 0;
}