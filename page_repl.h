/* page_repl.h */

#ifndef PAGE_REPL_MODULE
#define PAGE_REPL_MODULE

#include <stdint.h>

#include "memory.h"

/* Return the index of the page to be replaced in the IPT *
 * according to the LRU page replacement algorithm.       */
size_t lru(struct memory *mem);

/* Update the page's PID Working Set based on the last reference. *
 * Required for the  Working Set replacement algorithm.           */
void working_set(struct memory *mem, struct mem_entry *page_ref);


#endif