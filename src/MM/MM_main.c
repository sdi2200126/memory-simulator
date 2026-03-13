#include "MM.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define USAGE "\
Usage: MM_main -k <k> -f <frames> -q <q> -m <max>\n \
-k : Number of page faults before flushing\n \
-f : Number of frames available\n \
-q : Number of trace lines to read before changing process\n \
-m : (Optional) Maximum number of lines to read from trace files\n \
"

int parse_args(int argc, char *argv[], int *k, int *frames, int *q, int *max) {

    if (argc < 7) {
        fprintf(stderr, "Error: Not enough arguments\n");
        return -1;
    }

    while (--argc)
    {
        if (argc > 1 && strcmp(argv[argc - 1], "-k") == 0) {
            *k = atoi(argv[argc--]);
        } else if (argc > 1 && strcmp(argv[argc - 1], "-f") == 0) {
            *frames = atoi(argv[argc--]);
        } else if (argc > 1 && strcmp(argv[argc - 1], "-q") == 0) {
            *q = atoi(argv[argc--]);
        } else if (argc > 1 && strcmp(argv[argc - 1], "-m") == 0) {
            *max = atoi(argv[argc--]);
        } else {
            fprintf(stderr, "Error: arguments are invalid\n");
            return -1;
        }
    }
 
    return 0; 

}

int main(int argc, char *argv[]) {
    
    int k = 0, frames = 0, q = 0; 
    int max = 10000000; // Default max value

    if (parse_args(argc, argv, &k, &frames, &q, &max) != 0) {
        fprintf(stderr, "%s", USAGE);
        return 1;
    }

    MemoryManager *memory_manager = MM_init(k, frames, q, max, 2); // Mock initialization     

    if (MM_run(memory_manager) != 0) {
        return 1;
    }

    MM_print_results(memory_manager);

    MM_destroy(memory_manager);

    return 0; // Exit the program
}