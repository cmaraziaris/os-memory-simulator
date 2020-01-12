/* ipt_management.c */
#include <stdint.h>          // size_t, uint32_t, uint8_t
#include <time.h>            // timespec, clock_gettime

#include "ipt_management.h"
#include "memory.h"          // enum algorithm, NUM_OF_PROCESSES
#include "page_repl.h"       // lru(), working_set()

#define FAILED     0
#define SUCCESSFUL 1


// Initializes a memory entry (IPT + Main Memory) with the given values
static void set_new_entry(struct memory *mem, size_t index, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t ofs);

/* ========================================================================== */

// Search for a specific reference in the IPT. If found, update fields.
int ipt_search(struct memory *mem, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t ofs)
{
  struct virtual_memory *vm = mem->vmem;
  struct main_memory    *mm = mem->mmem;

  for (size_t i = 0; i < vm->ipt_size; ++i)        // Linear IPT search
  {
    if (vm->ipt[i].set && vm->ipt[i].addr == page && vm->ipt[i].pid == pid)
    {
      if (mode == 'W')
        mm->entries[i].modified = 1;          // Write operation

      mm->entries[i].latency = t;             // Update timestamp
      mm->entries[i].offset  = ofs;           // Update offset

      return SUCCESSFUL;      // Page found in the IPT and updated
    }
  }
  return FAILED;
}

/* ========================================================================== */

// Check if a reference can fit in the IPT. If yes, place it in the IPT/MainMem.
int ipt_fit(struct memory *mem, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t ofs)
{
  struct virtual_memory *vm = mem->vmem;

  if (vm->ipt_curr == vm->ipt_size)       // IPT full
    return FAILED; 

  size_t pos;

  for (size_t i = 0; i < vm->ipt_size; ++i) {         
    if (vm->ipt[i].set == 0)
    { 
      pos = i;          // Found an empty slot
      break;
    }
  }
  set_new_entry(mem, pos, page, pid, mode, t, ofs);    // Place the new page

  ++vm->ipt_curr;
  
  return SUCCESSFUL;    // Page is written to the IPT and the Main Memory
}

/* ========================================================================== */

// Place a reference in the IPT using a page replacement algorithm
void ipt_replace_page(struct memory *mem, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t ofs)
{
  size_t empty_pos;            // Index of an empty IPT slot

  if (mem->vmem->pg_repl == LRU)
    empty_pos = lru(mem);

  else if (mem->vmem->pg_repl == WS)
    empty_pos = working_set(mem, pid);    
    
  set_new_entry(mem, empty_pos, page, pid, mode, t, ofs);  // Place the new page
    
  ++mem->vmem->ipt_curr;
}

/* ========================================================================== */

static void set_new_entry(struct memory *mem, size_t index, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t ofs)
{
  mem->vmem->ipt[index] = (struct vmem_entry) { .set = 1, .addr = page, .pid = pid };          // Init the IPT entry
  mem->mmem->entries[index] = (struct mmem_entry) { .set = 1, .offset = ofs, .latency = t };   // Init the Main Memory entry  
  mem->mmem->entries[index].modified = (mode == 'W' ? 1 : 0);
}

/* ========================================================================== */