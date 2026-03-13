#include <stdio.h>
#include <string.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "PM.h"
#include "shared.h"

int PM_run(int max, int shm_id) {

    /*
    Logic:
    1) Read a trace line from stdin (redirected to pipe from MM)
    2) Write the trace line to stdout (redirected to pipe from MM)
    */
    
    // Attach to the shm
    ShmData *shm_addr = shmat(shm_id, 0, 0);
    if (shm_addr == (ShmData *)(-1)) {
        perror("shmat");
        return -1;
    }

    char trace_line[16];
    int count = 0;
    while (count++ < max && fgets(trace_line, 16, stdin) != NULL) {
        // Wait for the consumer to read the previous line
        sem_down(shm_addr->producer_sem);

        // Write the trace line to the shared memory
        if (strlen(trace_line) <= 0) 
            break;
        memcpy(shm_addr->trace_line, trace_line, strlen(trace_line) + 1);

        // Signal the consumer that a new line is available
        sem_up(shm_addr->consumer_sem);
    }

    // Wait for the consumer to read the previous line
    sem_down(shm_addr->producer_sem);
    
    // Indicate that no more trace lines can be read
    char end_line[12];
    sprintf(end_line, "%08x %c\n", 0, 'E');
    memcpy(shm_addr->trace_line, end_line, strlen(end_line) + 1);

    // Signal the consumer that a new line is available
    sem_up(shm_addr->consumer_sem);

    // Detach from the shm
    shmdt(shm_addr);

    return 0;
}