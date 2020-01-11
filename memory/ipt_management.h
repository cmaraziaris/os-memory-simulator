/* ipt_management.h */
#ifndef IPT_MANAGEMENT
#define IPT_MANAGEMENT

#include <stdint.h>       // size_t, uint32_t, uint8_t
#include <time.h>         // timespec

#include "memory.h"

/* Search for a `page` owned by `pid` in the IPT.                  *
 * Returns 1 if such entry is found and updates the entry, else 0. */
int  ipt_search(struct memory *, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t ofs);


/* If the IPT is full, returns 0.                                                           *
 * Else, inserts the values given as an entry in the IPT and the Main Memory and returns 1. */
int  ipt_fit   (struct memory *, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t ofs);


/* Creates space in the IPT by removing 1 or more pages, depending on the page replacement algorithm used. *
 * Then, stores the new entry in the *not full* IPT and Main Memory.                                       */
void ipt_replace_page(struct memory *, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t ofs);


#endif