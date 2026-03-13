#include "MM.h"

int MM_comm_read_trace_line(MemoryManager *memory_manager, char *operation, unsigned int *memory_address);
int MM_comm_init(MemoryManager *memory_manager);
void MM_comm_destroy(MemoryManager *memory_manager);