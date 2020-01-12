/* page_repl.h */
#ifndef PAGE_REPL_MODULE
#define PAGE_REPL_MODULE

#include <stdint.h>

#include "memory.h"


/* Remove the oldest reference stored in the IPT.     *
 * Return the index of the *empty* IPT/MainMem slot.  */
size_t lru(struct memory *mem);


/* Remove pages of process `pid` decided by the Working Set algorithm.  *
 * Return the index of an empty position in the IPT/MainMem.            */
size_t working_set(struct memory *mem, uint8_t pid);


/* If the window is full, adds the last reference in the window, and removes the oldest one. *
 * Else, inserts the last reference in the history window.                                   *
 * Reference is represented by `pid` and `page`                                              */
void ws_update_history_window(struct virtual_memory *vm, uint8_t pid, uint32_t page);


#endif