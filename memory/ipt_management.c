/* ipt_management.c */
#include <stdint.h>          // size_t, uint32_t, uint8_t
#include <time.h>            // timespec, clock_gettime

#include "ipt_management.h"
#include "memory.h"          // enum pg_rep_alg, NUM_OF_PROCESSES
#include "page_repl.h"       // lru(), working_set()

#define FAILED     0
#define SUCCESSFUL 1


/* Initializes a memory entry (IPT + Main Memory) with the given values */
static void set_new_entry(struct memory *mem, size_t index, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t ofs);

/* ========================================================================== */

int ipt_search(struct memory *mem, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t ofs)
{
  struct virtual_memory *vm = mem->vmem;
  struct main_memory    *mm = mem->mmem;

  for (size_t i = 0; i < vm->ipt_size; ++i)        /* Linear IPT search */
  {
    if (vm->ipt[i].set && vm->ipt[i].addr == page && vm->ipt[i].pid == pid)
    {
      if (mode == 'W')
        mm->entries[i].modified = 1;        /* Write operation */

      mm->entries[i].latency = t;           /* Update timestamp */
      mm->entries[i].offset  = ofs;         /* Update offset    */

      return SUCCESSFUL;      /* Page was found in the IPT */
    }
  }
  return FAILED;
}
/* ========================================================================== */

int ipt_fit(struct memory *mem, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t ofs)
{
  struct virtual_memory *vm = mem->vmem;

  if (vm->ipt_curr == vm->ipt_size)       /* IPT full */
    return FAILED; 

  size_t pos;

  for (size_t i = 0; i < vm->ipt_size; ++i) {          /* Find an empty slot */
    if (vm->ipt[i].set == 0)
    { 
      pos = i;
      break;
    }
  }
  set_new_entry(mem, pos, page, pid, mode, t, ofs);    /* Place the new page */

  ++vm->ipt_curr;
  
  return SUCCESSFUL;    /* Page is written to the IPT and the Main Memory */
}

/* ========================================================================== */

void ipt_replace_page(struct memory *mem, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t ofs)
{
  struct virtual_memory *vm = mem->vmem;

  if (vm->pg_repl == LRU)
  {
    size_t pos = lru(mem);        /* Decide a victim page */

    if (mem->mmem->entries[pos].modified == 1)    /* Remove the victim */
      ++mem->hd_writes;            
 
    set_new_entry(mem, pos, page, pid, mode, t, ofs);
  }

  else if (vm->pg_repl == WS)
  {
    size_t pos = working_set(mem, pid);    /* Get an empty page */

    set_new_entry(mem, pos, page, pid, mode, t, ofs);  /* Place the new page */
      
    ++vm->ipt_curr;
  }
}

/* ========================================================================== */

static void set_new_entry(struct memory *mem, size_t index, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t ofs)
{
  mem->vmem->ipt[index] = (struct vmem_entry) { .set = 1, .addr = page, .pid = pid };          /* Init the IPT entry */

  mem->mmem->entries[index] = (struct mmem_entry) { .set = 1, .offset = ofs, .latency = t };   /* Init the Main Memory entry */
  mem->mmem->entries[index].modified = (mode == 'W' ? 1 : 0);
}

/* ========================================================================== */