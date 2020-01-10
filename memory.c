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
#include "queue.h"        // queue


#define FAILED     0
#define SUCCESSFUL 1


static int  ipt_search(struct memory *, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t ofs);
static int  ipt_fit   (struct memory *, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t ofs);
static void ipt_replace_page(struct memory *, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t ofs);
static void set_new_entry   (struct memory *, size_t index, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t ofs);
/* ========================================================================== */

/* Initializes a memory entry with given values */
static void set_new_entry(struct memory *mem, size_t index, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t ofs)
{
  mem->vmem->ipt[index] = (struct vmem_entry) { .set = 1, .addr = page, .pid = pid };

  mem->mmem->entries[index] = (struct mmem_entry) { .set = 1, .offset = ofs, .latency = t };
  mem->mmem->entries[index].modified = (mode == 'W' ? 1 : 0);
}
/* ========================================================================== */

static int ipt_search(struct memory *mem, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t ofs)
{
  struct virtual_memory *vm = mem->vmem;
  struct main_memory    *mm = mem->mmem;

  /* Linear IPT search */
  for (size_t i = 0; i < vm->ipt_size; ++i)
  {
    if (vm->ipt[i].set && vm->ipt[i].addr == page && vm->ipt[i].pid == pid)
    {                                         /* Already in the IPT */
      if (mode == 'W')
        mm->entries[i].modified = 1;  /* Write operation */

      mm->entries[i].latency = t;        /* Update timestamp */
      mm->entries[i].offset  = ofs;   /* Update offset    */

      return SUCCESSFUL; /* Page was found in the IPT */
    }
  }
  return FAILED;
}
/* ========================================================================== */

static int ipt_fit(struct memory *mem, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t ofs)
{
  struct virtual_memory *vm = mem->vmem;

  if (vm->ipt_curr == vm->ipt_size) return FAILED; // IPT is full

  /* If IPT is not full, find a slot for the page */
  size_t pos;

  for (size_t i = 0; i < vm->ipt_size; ++i) 
  {
    if (vm->ipt[i].set == 0)   /* Empty page */
    { 
      pos = i;
      break;
    }
  }

  set_new_entry(mem, pos, page, pid, mode, t, ofs);       /* Place the new page */

  ++vm->ipt_curr;      /* Update occupied slots */
  
  return SUCCESSFUL; /* Page was read from the HD and written to the IPT */
}

/* ========================================================================== */
static void ipt_replace_page(struct memory *mem, uint32_t page, uint8_t pid, char mode, struct timespec t, uint16_t ofs)
{
  struct virtual_memory *vm = mem->vmem;

  if (vm->pg_repl == LRU)  /* Least Recently Used algorithm */
  {
    size_t pos = lru(mem);    /* Decide a victim page */

    if (mem->mmem->entries[pos].modified == 1) 
      ++mem->hd_writes;       /* Remove the victim */
 
    set_new_entry(mem, pos, page, pid, mode, t, ofs);      /* Place the new page */
  }

  else if (vm->pg_repl == WS) /* Working Set algorithm */
  {
    size_t pos = working_set(mem, pid);

    set_new_entry(mem, pos, page, pid, mode, t, ofs);  // just insert the new page at an empty spot on the ipt
      
    ++vm->ipt_curr;
  }
}

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

  if (mem->vmem->pg_repl == WS) 
    ws_update_history_window(mem->vmem, pid, page);

  if (ipt_search(mem, page, pid, mode, t, offset) == SUCCESSFUL) 
    return;
  
  ++mem->hd_reads;   /* Page not found in the main memory, so it will be read from the HD */
  ++mem->page_fs;

  if (ipt_fit(mem, page, pid, mode, t, offset) == SUCCESSFUL) 
    return;

  ipt_replace_page(mem, page, pid, mode, t, offset);   /* If IPT is full, perform a page replacement algorithm */
}


/* ========================================================================== */

/* Initialize the virtual and main memory segment. Requires an array of PIDs used. */
struct memory *mem_init(size_t frames, enum pg_rep_alg alg, uint8_t *pids, size_t ws_wnd_s)
{
  struct memory *mem = malloc(sizeof(struct memory));
  assert(mem);

  mem->hd_reads = mem->hd_writes = mem->page_fs = mem->total_req = 0;

  /* Set up the main memory segment */
  mem->mmem = malloc(sizeof(struct main_memory));
  assert(mem->mmem);
  
  mem->mmem->entries = calloc(frames, sizeof(struct mmem_entry));
  assert(mem->mmem->entries);

  mem->mmem->mm_size = frames;

  /* Set up the virtual memory segment */
  mem->vmem = malloc(sizeof(struct virtual_memory));
  assert(mem->vmem);

  struct virtual_memory *vm = mem->vmem;

  vm->ipt = calloc(frames, sizeof(struct vmem_entry)); /* Create the IPT */
  assert(vm->ipt);

  vm->pg_repl  = alg;
  vm->ipt_size = frames;
  vm->ipt_curr = 0;

  if (alg == WS) /* Working Set algorithm */
  {
    vm->ws = malloc(sizeof(struct working_set_comp));  /* Create the Working Set components */
    assert(vm->ws);

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
  
  printf("Number of Page Faults: %lu\n",      mem->page_fs  );
  printf("Number of HardDrive Reads: %lu\n",  mem->hd_reads );
  printf("Number of HardDrive Writes: %lu\n", mem->hd_writes);

  printf("\nTotal frames: %lu\n", mem->vmem->ipt_size);
}
/* ========================================================================== */

/* Deallocate space used from the memory segment */
void mem_clean(struct memory *mem)
{
  struct virtual_memory *vm = mem->vmem;

  if (vm->pg_repl == WS)  /* Deallocate Working Set components */
  {
    for (int i = 0; i < NUM_OF_PROCESSES; ++i)
      queue_destroy(vm->ws->history[i]);

    free(vm->ws);
  }

  free(vm->ipt);   /* Deallocate the virtual memory segment */
  free(vm);

  struct main_memory *mm = mem->mmem;

  free(mm->entries);    /* Deallocate main memory segment */
  free(mm);

  free(mem);
}
/* ========================================================================== */