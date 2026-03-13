Simple Memory Manager (MM) and Trace Provider (PM)

- Build

  - Run `make` in the root directory to compile MM and PM.
  - Run `make run` to execute MM with the default configuration (k=10, f=200, q=10).
  - The binaries will be placed in the `bin/` directory.

- Overview

  - This project implements a simple memory manager simulator (MM) and a trace provider (PM).
  - MM simulates pages/frames, handles page lookups, counts page faults and disk I/O, and prints final statistics.
  - PM reads trace files and forwards lines to MM (MM forks/execs PM for each trace).

- How the MM works

  - MM spawns one PM process per trace file. Each PM writes trace lines to a shm that MM can read from synced with semaphores.
  - MM maps addresses to page indexes (page index derived from the address) and maintains a hash table mapping pages to frames.
  - A page fault occurs when a referenced page is not found in the hash table.
  - MM performs a full flush (writes dirty frames back to disk and clears the working set) in two cases:
    - Hashtable collisions: when inserting a page would collide in the hash table, MM performs a full flush to reset the table.
    - k+1 page faults: MM allows up to k page faults without doing a full flush - on the (k+1)th page fault a full flush occurs.
  - Scheduling / fairness: MM is given a number q. It reads q trace lines from one PM, then switches to the next PM (round-robin).

- How the PM works

  - PM takes the max number of lines to read from stdin as a command-line argument.
  - PM_run reads up to max lines from stdin (stdin is the trace file when MM execs PM) and writes each trace line to the shm.
  - PM prints an "end" marker line when done: "00000000 E" (address 0 with operation 'E') so MM knows the process finished.

- Short description of main functions and their logic

  - MM_main / MM_run

    - Parse CLI args (k, f, q, optional m).
    - Initialize MM data structures and IPC.
    - Run the main simulation loop.
    - Loop reads trace lines from the shm, handles scheduling (read q lines per process), updates page/frame state, detects page faults, and triggers flushes when required.
    - At end, call MM_print_results and cleanup.

  - MM_init / MM_destroy

    - MM_init: allocate and initialize the memory manager structures (frame table, hash table, counters, bookkeeping for dirty bits, etc.).
    - MM_destroy: free memory, close pipes, and ensure child processes are reaped.

  - MM_print_results

    - Print the final statistics: total page faults, disk reads/writes, full flush count, runtime counters needed for the output.

  - MM_comm_init / MM_comm_read_trace_line / MM_comm_destroy

    - MM_comm_init: create pipes, fork/exec PM processes, redirect PM stdin appropriately, initialize shared memory segments and semaphores.
    - MM_comm_read_trace_line: read trace lines from an shm, switching context when enough lines have been read - returns parsed lines.
    - MM_comm_destroy: reap child processes, destroy shm and semaphores.

  - MM_tools

    - calculate_page_index: calculate the page index from the address (address / total pages).
    - calculate_frame_index: calculate the frame index from the page index and the process id given by the MM to the process.
    - create_sem / destroy_sem: create and destroy semaphores for IPC.
    - create_and_attach_shm / destroy_and_detach_shm : create and attach to a shared memory segment, and destroy it when done.
    - get_key_with_process_id: generate a unique key for shm using ftok based on the process id.

  - PM_run (in src/PM/PM.c)

    - Read lines up to the given max from stdin and write them to stdout. After finishing (or EOF) print the end marker "00000000 E".

  - Shared 

    - sem_up / sem_down: semaphore operations for IPC synchronization.

- Notes on test results and filenames

  - Test files in test_results are named like: k_frames_q.out
    - k = number of page faults allowed before a full flush (so flush on k+1)
    - frames = total number of frames in the MM simulation
    - q = number of trace lines read from each PM before switching to the next (quantum)
    - Example files present: 1_200_5.out, 2_200_5.out, 4_200_5.out, 8_200_5.out, 16_200_5.out, 32_200_5.out, 64_200_5.out, 128_200_5.out, 256_200_5.out - all use 200 frames and q=5 and vary k.
  - How to interpret results:
    - Small k (e.g., k=1 or k=2): frequent full flushes & high page fault count.
      - Increased page faults because the working set is repeatedly discarded.
      - Flushes occur often due to the k+1 page fault rule and less often due to hashtable collisions.
    - Large k (e.g., k=128, 256): fewer full flushes.
      - Page faults don't occur as often due to less full flushes.
      - flushes occur mostly because of hashtable collisions.
    - Hashtable collisions seem to be the main reason for flushes when k is not too small.
      - Even for small k compared to the frames (4%) the hashtable collisions can cause more than half of the flushes. 
    - Fixed frames: since all test files use 200 frames, differences in k show how the full-flush policy alone affects performance for the same memory budget.

- Additional notes

  - Trace format expected by MM / PM: "%08x %c\n" (8-hex-digit address, space, operation char).
  - The PM executable prints "00000000 E" to signal EOF; MM treats that as process termination for that trace provider.
  - Trace files are in traces/ and PM is normally executed by MM. PM is not run manually.
  - MM can be expanded to support more than 2 processes by adding more PMs and adding more files to the file array in MM_comm_init.
    - The number of processes (2) is hardcoded in MM_init.
  - Page size is defined as 4KB (4096 bytes) in MM_tools.h - can be changed.

- Source code

  - src/MM/ - MM implementation and helpers:
    - MM_main.c, MM.c, MM_comm.c, MM_tools.c
  - src/PM/ - trace provider:
    - PM_main.c, PM.c
  - include/ - header files with types and function declarations
  - traces/ - sample traces used by PM
  - test_results/ - output files from running MM with different configurations
  - bin/ - compiled binaries for MM and PM