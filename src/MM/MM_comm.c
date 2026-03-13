#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "MM_comm.h"
#include "MM_tools.h"

// Initializes the communication between processes
// parameters:
// processes - How many processes to fork and initialize IPC with
// Returns an array of file descriptors (pipe read ends) that can be used to read information from the other processes 
int MM_comm_init(MemoryManager *memory_manager) {
    
    /*
    Logic:
    1) For every process:
        1.1) Initialize the shm and semaphores
        1.2) Open the trace file
        1.3) Fork process
            1.4.1) Child - redirect stdin to the file fd - exec the PM bin
            1.4.2) Parent - Add the pipe shm and semaphores to the process array
    */

    char *file_array[] = {"traces/bzip.trace", "traces/gcc.trace"};
    int files = 2;

    for (int i = 0; i < memory_manager->total_processes && i < files; i++) {
        // Open the file with open
        int file_fd;
        if ((file_fd = open(file_array[i], O_RDONLY)) < 0) {
            perror("open");
            return -1;
        }

        // Create a new shm
        key_t key = get_key_with_process_id(memory_manager->processes[i].id);
        ShmData *addr;
        int id;
        if (create_and_attach_shm(&id, (void *)&addr, key) != 0) {
            fprintf(stderr, "Error: Failed to initialize shm");
            return -1;
        }

        // Create 2 new semaphores for the shm (producer and consumer)
        if (create_sem(&(addr->producer_sem), 1) != 0) {
            fprintf(stderr, "Error: Failed to initialize producer semaphore");
            return -1;
        }
        if (create_sem(&(addr->consumer_sem), 0) != 0) {
            fprintf(stderr, "Error: Failed to initialize consumer semaphore");
            return -1;
        }

        // Fork
        pid_t pid; 
        if ((pid = fork()) == 0) {
            // Child
            // Redirect stdin to the file descriptor
            if (dup2(file_fd, STDIN_FILENO) < 0) {
                perror("dup2");
                return -1;
            }
            if (close(file_fd) < 0) {
                perror("close");
                return -1;
            }

            // format the arguments
            char max_as_str[32];
            char shm_id_as_str[32];
            sprintf(max_as_str, "%d", i == memory_manager->total_processes - 1 
                ? memory_manager->max / memory_manager->total_processes + memory_manager->max % memory_manager->total_processes 
                : memory_manager->max / memory_manager->total_processes);
            sprintf(shm_id_as_str, "%d", id);

            char *args[32] = {
                "./bin/PM",
                max_as_str, 
                shm_id_as_str, 
                NULL
            };
            
            // exec
            execv("./bin/PM", args);
            perror("execl");
            return -1;
        }
        else {
            // Parent
            // Close the file fd
            if (close(file_fd) < 0) {
                perror("close");
                return -1;
            }
            memory_manager->processes[i].pid = pid;
            memory_manager->processes[i].shm_addr = addr;
            memory_manager->processes[i].shm_id = id;
        }   
    }
    
    return 0;
}

// Reads a trace line from the current process shared memory segment 
int MM_comm_read_trace_line(MemoryManager *memory_manager, char *operation, unsigned int *memory_address) {
    // wait for the producer to write
    sem_down(memory_manager->active_process->shm_addr->consumer_sem);
    
    if (sscanf(memory_manager->active_process->shm_addr->trace_line, "%x %c", memory_address, operation) != 2) {
        perror("sscanf");
        return -1;
    }

    // signal the producer that the data has been read
    sem_up(memory_manager->active_process->shm_addr->producer_sem);
    return 0;
}

void MM_comm_destroy(MemoryManager *memory_manager) {
    for (int i = 0; i < memory_manager->total_processes; i++) {
        waitpid(memory_manager->processes[i].pid, NULL, 0);
        destroy_sem(memory_manager->processes[i].shm_addr->producer_sem);
        destroy_sem(memory_manager->processes[i].shm_addr->consumer_sem);
        destroy_and_dettach_shm(memory_manager->processes[i].shm_id, memory_manager->processes[i].shm_addr);
    }
}