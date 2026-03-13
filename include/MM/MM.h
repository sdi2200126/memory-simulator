#include <stdbool.h>
#include <sys/types.h>
#include "../shared/shared.h"

#ifndef DEBUG
#define DEBUG 0
#endif

typedef struct page_t {
    int page_index; // Page index on the main memory 
    int process_id; // ID of the process that owns this page
    bool dirty; // Indicates if the page has been modified
} Page;

typedef struct frame_t {
    Page *page; // Pointer to the page stored in this frame
    bool valid; // Indicates if the frame is valid (contains a page)
} Frame;

typedef struct statistics_t {
    int reads_from_disk; // Total number of read operations
    int writes_to_disk; // Total number of write operations
    int page_faults; // Total number of page faults
    int pages_occupied; // Total number of pages currently occupied
    int total_flushes; // Number of total flushes
    int flushes_due_to_collisions; // Number of flushes due to hashtable collisions
    int flushes_due_to_k; // Number of flushes due to exceeded page fault limit
} Statistics;

typedef struct process_t {
    int shm_id; // Id of the shared memory segment
    ShmData *shm_addr; // Address of the shared memory segment
    pid_t pid; // Forked process id
    int id; // Process id in the application (indexes starting from 0)
    int trace_lines_read; // Number of trace lines read without context switching
    int block_page_faults; // Number of page faults in current trace block
    int total_trace_lines_read; // Number of total trace lines read 
    bool has_any_more_lines; // Indicates if the process is finished reading trace lines
} Process;

typedef struct memory_manager_t {
    int total_frames; // Total number of frames available
    Frame *frames; // Array of frames managed by the memory manager
    int k; // Number of page faults before flushing
    int q; // Number of trace lines to read before changing process
    int max; // Maximum number of lines to read from trace files
    int total_processes; // Number of active processes
    Process *processes; // Array of the active processes
    Process *active_process; // The currently active process
    Statistics statistics; // Statistics of memory manager operations
} MemoryManager;

int MM_run(MemoryManager *memory_manager);
void MM_print_results(MemoryManager *memory_manager);
MemoryManager *MM_init(int k, int frames, int q, int max, int processes);
void MM_destroy(MemoryManager *memory_manager);