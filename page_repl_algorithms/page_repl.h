/* page_repl.h */

#ifndef PAGE_REPL_MODULE
#define PAGE_REPL_MODULE

#include <stdint.h>

#include "memory.h"

/* Return the index of the page to be replaced in the IPT *
 * according to the LRU page replacement algorithm.       */
size_t lru(struct memory *mem);

size_t working_set(struct memory *mem, uint8_t pid);

void ws_update_history_window(struct virtual_memory *vm, uint8_t pid, uint32_t page);




#endif