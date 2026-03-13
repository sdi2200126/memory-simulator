#include "shared.h"
#include <sys/sem.h>

int sem_down(int sem_id) {
    struct sembuf sb = {0, -1, 0};
    return semop(sem_id, &sb, 1);
}

int sem_up(int sem_id) {
    struct sembuf sb = {0, 1, 0};
    return semop(sem_id, &sb, 1);
}