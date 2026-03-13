#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/sem.h>

#define PAGE_SIZE 4096

int calculate_page_index(const unsigned int address);
int calculate_frame_index(const unsigned int page_index, const unsigned int total_frames, const unsigned int number_of_processes, const unsigned int process_id);
int create_sem(int *sem_id, int initial_value);
void destroy_sem(int sem_id);
key_t get_key_with_process_id(int process_id);
int create_and_attach_shm(int *shm_id, void** addr, key_t key);
void destroy_and_dettach_shm(int id, void *addr);