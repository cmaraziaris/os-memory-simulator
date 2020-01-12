/* memory.h */
#ifndef MEMORY_MODULE
#define MEMORY_MODULE

#include <stdint.h>     // uint8_t, uint32_t, size_t

#include "memory_structs.h"


/* Initializes the memory segment and returns a pointer to it.    *
 * Requires: 1) # of frames    2) Page Replacement algrorithm     *
 * 3) Array of associated PIDs 4) Working Set window size         *
 * Note: 4th arg is ignored if not for the Working Set algorithm  */
struct memory* mem_init(size_t frames, enum algorithm alg, uint8_t *pids, size_t ws_wnd_s);


/* Requests an address from the memory, and applies `mode` operation to it. *
 * Requires: 1) ptr to memory segment 2) Address to retrieve                *
 * 3) Mode ('R'/'W') 4) PID of the process making the request               */
void mem_retrieve(struct memory *mem, uint32_t addr, char mode, uint8_t pid);


/* Outputs stats about memory usage. */
void mem_stats(struct memory *mem);


/* Deallocates space used for the memory segment */
void mem_clean(struct memory *mem);



#endif