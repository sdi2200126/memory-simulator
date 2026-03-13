#include "MM_tools.h"
#include "MM_comm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Flushes all the pages for the active process
void MM_flush_memory(MemoryManager *memory_manager) {
    int process_frames_start = (memory_manager->total_frames / memory_manager->total_processes) * memory_manager->active_process->id;
    int process_frames_end = process_frames_start + (memory_manager->total_frames / memory_manager->total_processes);

    int pages_removed = 0;
    for (int i = process_frames_start; i < process_frames_end; i++) {
        if (memory_manager->frames[i].valid) {
            if (memory_manager->frames[i].page->dirty) {
                // Write the dirty page to the disk
                memory_manager->statistics.writes_to_disk++;
            }
            free(memory_manager->frames[i].page);
            memory_manager->frames[i].page = NULL;                        
            memory_manager->frames[i].valid = 0;

            pages_removed++;
        }
    }

    memory_manager->statistics.pages_occupied -= pages_removed;
    memory_manager->statistics.total_flushes++;
}

// Switches the process for which the memory manager reads trace lines 
void MM_context_switch(MemoryManager *memory_manager) {
    int next_process_index = (memory_manager->active_process->id + 1) % memory_manager->total_processes;
    Process *next_process = &(memory_manager->processes[next_process_index]);

    // Update the old process counters
    memory_manager->active_process->trace_lines_read = 0;

    // Set the new process as the active process
    memory_manager->active_process = next_process;
}

// Prints the results of the simulation
void MM_print_results(MemoryManager *memory_manager) {

    printf("\nMemory Manager Results\n");

    printf("\n");

    printf("Starting conditions:\n");
    printf("\tframes: %d\n", memory_manager->total_frames);
    printf("\tk: %d\n", memory_manager->k);
    printf("\tq: %d\n", memory_manager->q);
    printf("\tmax: %d\n", memory_manager->max);
    printf("\tprocesses: %d\n", memory_manager->total_processes);

    printf("\n");

    printf("Statistics:\n");
    printf("\tpage faults: %d\n", memory_manager->statistics.page_faults);
    printf("\treads from disk: %d\n", memory_manager->statistics.reads_from_disk);
    printf("\twrites to disk: %d\n", memory_manager->statistics.writes_to_disk);
    printf("\tpages occupied (at exit): %d\n", memory_manager->statistics.pages_occupied);
    printf("\tflushes: %d\n", memory_manager->statistics.total_flushes);
    if (memory_manager->statistics.total_flushes > 0) {
        printf("\t  from HT colisions: %.2f%%\n", ((float) memory_manager->statistics.flushes_due_to_collisions / (float) memory_manager->statistics.total_flushes) * 100);
        printf("\t  from k + 1 PFs: %.2f%%\n", ((float) memory_manager->statistics.flushes_due_to_k / (float) memory_manager->statistics.total_flushes) * 100);
    }

    printf("\n");

    printf("Processes:\n");
    for (int i = 0; i < memory_manager->total_processes; i++) {
        printf("\n");
        printf("\tpid: %d\n", memory_manager->processes[i].pid);
        printf("\ttrace lines read: %d\n", memory_manager->processes[i].total_trace_lines_read);
    }

}

// Reads a new trace line and making context changes when needed
// Returns 0 on success, -1 on failure
int MM_read_new_trace_line(MemoryManager *memory_manager, char *operation, unsigned int *memory_address) {
    
    // When enough trace lines have been read - switch process
    if (memory_manager->active_process->trace_lines_read == memory_manager->q) {
        MM_context_switch(memory_manager);
    }

    // Read the new trace line
    return MM_comm_read_trace_line(memory_manager, operation, memory_address);
}

// Handles the page faults
int MM_page_fault(MemoryManager *memory_manager, Frame *frame, int page_index, char operation) {
    
    // Determine if the page fault is the k + 1 from the last flush
    if (memory_manager->active_process->block_page_faults == memory_manager->k) {
        // Flush the memory
        MM_flush_memory(memory_manager);
        memory_manager->statistics.flushes_due_to_k++;
        memory_manager->active_process->block_page_faults = 0;
    }

    // Add the new page
    Page *page = malloc(sizeof(Page));
    if (!page) {
        perror("malloc");
        return -1;
    }
    page->page_index = page_index;
    page->dirty = operation == 'W' ? true : false;
    page->process_id = memory_manager->active_process->id; 
    
    // Update the frame
    frame->page = page;
    frame->valid = true;
    
    // Update the memory manager
    memory_manager->statistics.page_faults++;
    memory_manager->statistics.pages_occupied++;
    if (operation == 'R') 
        memory_manager->statistics.reads_from_disk++;
    memory_manager->active_process->block_page_faults++;

    return 0;
}

// Runs the memory manager
// parameters: 
// memory_manager - The MemoryManager instance to run
// Returns 0 on success, -1 on error
int MM_run(MemoryManager *memory_manager) {
    
    /*
    Logic: 
    1) Read new trace line (outsource the logic of where from and how that line is read)
    2) Find the page index and the frame index of it
        2.1) Frame is non empty
            2.1.1) The page that is loaded is not the same as the new one - hashtable colision - apply replacement policy 
            2.2.2) The page that is loaded is the same as the new one - not a PAGE FAULT - go to (1) 
    3) place the new page - PAGE FAULT
    4) go to (1)
    */
    
    if (!memory_manager) {
        fprintf(stderr, "Error: Memory manager is not initialized\n");
        return -1; 
    }

    unsigned int memory_address;
    char operation;

    int count = 0;
    while (count++ < memory_manager->max && MM_read_new_trace_line(memory_manager, &operation, &memory_address) == 0) {

        // No more trace lines to read - context switch to see if there are more trace lines for the next process
        if (operation == 'E') {
            memory_manager->active_process->has_any_more_lines = false;
            bool any_processes_have_lines = false;
            for (int i = 0; i < memory_manager->total_processes; i++) {
                if (memory_manager->processes[i].has_any_more_lines) {
                    // There are more lines for the next process
                    any_processes_have_lines = true;
                    MM_context_switch(memory_manager);
                    break;
                }
            }
            if (any_processes_have_lines) {
                count--; // Decrement the count to not count the 'E' line
                continue; // Continue to read the next trace line for the next process
            }
            break; // No more lines to read for any process - exit the loop
        }

        memory_manager->active_process->trace_lines_read++;
        memory_manager->active_process->total_trace_lines_read++;

        int page_index = calculate_page_index(memory_address);
        int frame_index = calculate_frame_index(page_index, memory_manager->total_frames, memory_manager->total_processes, memory_manager->active_process->id);
    
        Frame *frame = &(memory_manager->frames[frame_index]);
    
        // Determine if the frame at this index is free
        if (frame->valid) {
            // Non empty frame
            // Determine if that page is the same as the new page 
            if (frame->page->page_index != page_index) {
                // It is not the same page - apply the replacing policy (FWF) due to hash table collision (not due to k + 1 PFs)
                // Flush the memory
                MM_flush_memory(memory_manager);
                memory_manager->statistics.flushes_due_to_collisions++;
            }
            else {
                // It is the same page - do nothing 
                // Determine if the operation was a write and set dirty bit accordingly
                if (operation == 'W') {
                    frame->page->dirty = true;
                }
                continue;
            }
        }

        // Page fault
        if (MM_page_fault(memory_manager, frame, page_index, operation) != 0) {
            return -1;
        }
        
    }

    return 0; // Success

}

// Initializes the memory manager
// parameters: 
// k - The amount of page faults before flushing (k <= floor(frames/2))
// frames - The total number of frames available in the memory manager
// q - The number of trace lines to read before changing process
// max - The total number of trace lines to read
// processes - The number of active processes
// Returns a MemoryManager pointer on success, null on error
MemoryManager *MM_init(int k, int frames, int q, int max, int processes) {
    
    if (k <= 0 || frames <= 0 || q <= 0) {
        return NULL;
    }

    MemoryManager *memory_manager = malloc(sizeof(MemoryManager));
    if (!memory_manager) {
        return NULL;
    }

    // Initialize the MemoryManager structure
    memory_manager->frames = malloc(frames * sizeof(Frame));
    if (!memory_manager->frames) {
        free(memory_manager);
        return NULL;
    }
    for (int i = 0; i < frames; i++) {
        memory_manager->frames[i].page = NULL;
        memory_manager->frames[i].valid = false;
    }

    memory_manager->processes = malloc(processes * sizeof(Process));
    if (!memory_manager->processes) {
        free(memory_manager->frames);
        free(memory_manager);
        return NULL;
    }
    for (int i = 0; i < processes; i++) {
        memory_manager->processes[i].id = i;
        memory_manager->processes[i].trace_lines_read = 0;
        memory_manager->processes[i].total_trace_lines_read = 0;
        memory_manager->processes[i].block_page_faults = 0;
        memory_manager->processes[i].has_any_more_lines = true;
    }
    memory_manager->active_process = memory_manager->processes; // Set the first process in the array as the active one

    memory_manager->total_frames = frames;
    memory_manager->k = k;
    memory_manager->q = q;
    memory_manager->max = max;
    memory_manager->total_processes = processes;
    memory_manager->statistics.reads_from_disk = 0;
    memory_manager->statistics.writes_to_disk = 0;
    memory_manager->statistics.page_faults = 0;
    memory_manager->statistics.pages_occupied = 0;
    memory_manager->statistics.total_flushes = 0;
    memory_manager->statistics.flushes_due_to_collisions = 0;
    memory_manager->statistics.flushes_due_to_k = 0;

    // Initialize the communication
    if (MM_comm_init(memory_manager) != 0) {
        return NULL;
    }

    return memory_manager; // Initialization successful
}

void MM_destroy(MemoryManager *memory_manager) {
    // Wait for the children processes 
    MM_comm_destroy(memory_manager);

    for (int i = 0; i < memory_manager->total_frames; i++) {
        if (memory_manager->frames[i].valid) {
            free(memory_manager->frames[i].page);
        }
    }
    free(memory_manager->frames);
    free(memory_manager->processes);
    free(memory_manager);
}