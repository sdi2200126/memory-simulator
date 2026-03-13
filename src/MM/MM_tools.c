#include "MM_tools.h"
#include "shared.h"

int calculate_page_index(const unsigned int address) {
    return address / PAGE_SIZE;
}

int calculate_frame_index(const unsigned int page_index, const unsigned int total_frames, const unsigned int number_of_processes, const unsigned int process_id) {
    unsigned int frames_per_process = total_frames / number_of_processes;
    return (page_index % frames_per_process) + process_id * frames_per_process;
}

int create_sem(int *sem_id, int initial_value) {
    if ((*sem_id = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT)) == -1) {
        perror("semget");
        return -1;
    }
    if (semctl(*sem_id, 0, SETVAL, initial_value) == -1) {
        perror("semctl");
        return -1;
    }
    return 0;
}

void destroy_sem(int sem_id) {
    semctl(sem_id, 0, IPC_RMID);
}

key_t get_key_with_process_id(int process_id) {
    return ftok(".", process_id);
}

int create_and_attach_shm(int *shm_id, void** addr, key_t key) {
    if ((*shm_id = shmget(key, sizeof(ShmData), 0666 | IPC_CREAT)) == -1) {
        perror("shmget");
        return -1;
    }
    if ((*addr = shmat(*shm_id, 0, 0)) == (char *)(-1)) {
        perror("shmat");
        return -1;
    }
    return 0;
}

void destroy_and_dettach_shm(int shm_id, void *addr) {
    shmdt(addr);
    shmctl(shm_id, IPC_RMID, NULL);
}