#define TRACE_LINE_SIZE 12

typedef struct shm_data_t {
    char trace_line[TRACE_LINE_SIZE];
    int producer_sem; 
    int consumer_sem; 
} ShmData;

int sem_down(int sem_id);
int sem_up(int sem_id);
