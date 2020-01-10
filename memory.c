/* memory.c */
// TODO: Na to spasw se synarthseis ++ update page_repl.c
#include <assert.h>     // for malloc check
#include <stdio.h>      // printf
#include <stdbool.h>    // bool
#include <stdint.h>     // size_t, uint32_t, uint8_t
#include <stdlib.h>     // malloc, calloc, free, NULL
#include <time.h>       // timespec, clock_gettime

#include "memory.h"       // enum pg_rep_alg, NUM_OF_PROCESSES
#include "page_repl.h"    // lru(), working_set()
#include "queue.h"  // queue

/* Initializes a memory entry with given values */

static void set_new_entry(struct memory *mem, size_t index, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t offset);

/* ========================================================================== */

/* Search in virtual memory, and then in the HD, for an address requested *
 * from process `pid` to perform the `mode` operation (R/W)            */
void mem_retrieve(struct memory *mem, uint32_t addr, char mode, uint8_t pid)
{
  ++mem->total_req;

  uint16_t offset = (addr << 20) >> 20;
  uint32_t page = addr >> 12;  /* Remove offset */

  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t); /* Keep the time of reference */

  struct virtual_memory *vm = mem->vmem;
  struct main_memory    *mm = mem->mmem;

  if (vm->pg_repl == WS) 
    ws_update_history_window(vm, pid, page);


  /* Linear IPT search */
  for (size_t i = 0; i < vm->ipt_size; ++i)
  {
    if (vm->ipt[i].set && vm->ipt[i].addr == page && vm->ipt[i].pid == pid)
    {                                         /* Already in the IPT */
      if (mode == 'W')
        mm->entries[i].modified = 1;  /* Write operation */

      mm->entries[i].latency = t;     /* Update timestamp */

      return; /* Page was found in the IPT */
    }
  }
  /* Page not found in the main memory, so it will be read from the HD */
  ++mem->hd_reads; 
  ++mem->page_fs;

  /* If IPT is not full, find a slot for the page */
  if (vm->ipt_curr != vm->ipt_size)
  {
    for (size_t i = 0; i < vm->ipt_size; ++i)
    {
      if (vm->ipt[i].set == 0)   /* Empty page */
      { 
      /* Place the new page */
        set_new_entry(mem, i, page, pid, mode, t, offset);

        ++vm->ipt_curr;      /* Update occupied slots */
      
        break; /* Page was read from the HD and written to the IPT */
      }
    }
  }
  else   /* If IPT is full, perform a page replacement algorithm */
  { 
    
    if (vm->pg_repl == LRU)  /* Least Recently Used algorithm */
    {
      size_t pos = lru(mem);    /* Decide a victim page */

       /* Remove the victim */
      if (mm->entries[pos].modified == 1) ++mem->hd_writes;
 
      /* Place the new page */
      set_new_entry(mem, pos, page, pid, mode, t, offset);  
    }

    else if (vm->pg_repl == WS) /* Working Set algorithm */
    {
  
      size_t empty = working_set(mem, pid);

        // just insert the new page at an empty spot on the ipt
      set_new_entry(mem, empty, page, pid, mode, t, offset);
      
      ++vm->ipt_curr;

    }
  }
}
/* ========================================================================== */
#if 1
/* Initializes a virtual memory entry */
static void set_new_entry(struct memory *mem, size_t index, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t offset)
{
  struct vmem_entry *ventr = &mem->vmem->ipt[index];
  struct mmem_entry *mentr = &mem->mmem->entries[index];

  ventr->set  = 1;
  ventr->addr = page;
  ventr->pid  = pid;

  mentr->set = 1; // Questionable field
  mentr->offset = offset;
  mentr->latency = t;
  mentr->modified = (mode == 'W' ? 1 : 0);
}
#endif
/* ========================================================================== */

/* Initialize the virtual and main memory segment. Requires an array of PIDs used. */
struct memory *mem_init(size_t frames, enum pg_rep_alg alg, uint8_t *pids, size_t ws_wnd_s)
{
  struct memory *mem = malloc(sizeof(struct memory));
  assert(mem != NULL);

  mem->hd_reads = mem->hd_writes = mem->page_fs = mem->total_req = 0;

  /* Set up the main memory segment */
  mem->mmem = malloc(sizeof(struct main_memory));
  mem->mmem->entries = calloc(frames, sizeof(struct mmem_entry));

  mem->mmem->mm_size = frames;

  /* Set up the virtual memory segment */
  mem->vmem = malloc(sizeof(struct virtual_memory));

  struct virtual_memory *vm = mem->vmem;

  vm->ipt = calloc(frames, sizeof(struct vmem_entry)); /* Create the IPT */
  assert(vm->ipt != NULL);

  vm->ipt_size = frames;

  vm->ipt_curr  = 0;

  vm->pg_repl = alg;

  if (alg == WS) /* Working Set algorithm */
  {
    /* Create the Working Set Manager */
    vm->ws = malloc(sizeof(struct working_set_manager));
    assert(vm->ws != NULL);

    vm->ws->window_s = ws_wnd_s;

    for (int i = 0; i < NUM_OF_PROCESSES; ++i)
      vm->ws->history[i] = queue_initialize();
  }

  return mem;
}

/* ========================================================================== */

/* Print some stats about memory usage */
void mem_stats(struct memory *mem)
{
  printf("\nPage Fault Rate = %1.6lf\n\n", (double) mem->page_fs / mem->total_req );
  
  printf("Number of HardDrive Reads: %lu\n",  mem->hd_reads );
  printf("Number of HardDrive Writes: %lu\n", mem->hd_writes);
  printf("Number of Page Faults: %lu\n",      mem->page_fs  );

  printf("\nTotal frames: %lu\n", mem->vmem->ipt_size);
}
/* ========================================================================== */

/* Deallocate space used from the memory segment */
void mem_clean(struct memory *mem)
{
  /* Deallocate virtual memory */
  struct virtual_memory *vm = mem->vmem;

  if (vm->pg_repl == WS) /* Deallocate Working Set components */
  {
    for (int i = 0; i < NUM_OF_PROCESSES; ++i)
      queue_destroy(vm->ws->history[i]);

    free(vm->ws);
  }

  free(vm->ipt);   /* Deallocate the vmemory segment */
  free(vm);

  /* Deallocate main memory */
  struct main_memory *mm = mem->mmem;

  free(mm->entries);
  free(mm);

  free(mem);
}
/* ========================================================================== */