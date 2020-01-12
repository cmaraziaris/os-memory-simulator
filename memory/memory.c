/* memory.c */
#include <assert.h>       // for malloc check
#include <stdio.h>        // printf
#include <stdbool.h>      // bool
#include <stdint.h>       // size_t, uint32_t, uint8_t
#include <stdlib.h>       // malloc, calloc, free, NULL
#include <time.h>         // timespec, clock_gettime

#include "memory.h"          // enum algorithm, NUM_OF_PROCESSES
#include "queue.h"
#include "page_repl.h"       // ws_update_history_window()
#include "ipt_management.h"  // ipt_*()

#define FAILED     0
#define SUCCESSFUL 1

/* ========================================================================== */

void mem_retrieve(struct memory *mem, uint32_t addr, char mode, uint8_t pid)
{
  ++mem->total_req;

  uint16_t offset = (addr << 20) >> 20;
  uint32_t page = addr >> 12;             // Remove offset

  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);     // Keep the time of reference

  if (mem->vmem->pg_repl == WS) 
    ws_update_history_window(mem->vmem, pid, page);     // History window rolls

  if (ipt_search(mem, page, pid, mode, t, offset) == SUCCESSFUL)  // Already in the IPT
    return;
  
  ++mem->hd_reads;          // Page not found in main memory,
  ++mem->page_fs;           // so it will be read from the HD

  if (ipt_fit(mem, page, pid, mode, t, offset) == SUCCESSFUL)     // Can fit in the IPT
    return;

  ipt_replace_page(mem, page, pid, mode, t, offset);   // IPT full, perform a page replacement algorithm
}

/* ========================================================================== */

struct memory *mem_init(size_t frames, enum algorithm alg, uint8_t *pids, size_t ws_wnd_s)
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

  vm->ipt = calloc(frames, sizeof(struct vmem_entry));    // Create the IPT
  assert(vm->ipt);

  vm->pg_repl  = alg;
  vm->ipt_size = frames;
  vm->ipt_curr = 0;

  if (alg == WS)            // Create the Working Set components
  {
    vm->ws = malloc(sizeof(struct working_set_comp));  
    assert(vm->ws);

    vm->ws->window_s = ws_wnd_s;
    vm->ws->history_index = malloc(NUM_OF_PROCESSES * sizeof(uint8_t));
    assert(vm->ws->history_index);

    for (size_t i = 0; i < NUM_OF_PROCESSES; ++i)
      vm->ws->history_index[i] = pids[i];

    for (size_t i = 0; i < NUM_OF_PROCESSES; ++i)
      vm->ws->history[i] = queue_initialize();
  }

  return mem;
}

/* ========================================================================== */

void mem_clean(struct memory *mem)
{
  struct virtual_memory *vm = mem->vmem;

  if (vm->pg_repl == WS)          // Deallocate Working Set components
  {
    free(vm->ws->history_index);

    for (int i = 0; i < NUM_OF_PROCESSES; ++i)
      queue_destroy(vm->ws->history[i]);

    free(vm->ws);
  }

  free(vm->ipt);         // Deallocate the virtual memory segment
  free(vm);

  struct main_memory *mm = mem->mmem;

  free(mm->entries);     // Deallocate main memory segment
  free(mm);

  free(mem);
}

/* ========================================================================== */

void mem_stats(struct memory *mem)
{
  char red[] = "\033[0;31m";
  char yel[] = "\033[0;33m";
  char cyn[] = "\033[0;36m";
  char res[] = "\033[0m";

  printf("> Printing simulation results!\n");
  printf("\n%s    Page Fault Rate%s = %1.6lf\n\n", 
    cyn, res, (double) mem->page_fs / mem->total_req );
  
  printf("%s    Page Faults:%s %lu\n",       red, res, mem->page_fs  );
  printf("%s    HardDrive Reads:%s %lu\n",   yel, res, mem->hd_reads );
  printf("%s    HardDrive Writes:%s %lu\n\n",yel, res, mem->hd_writes);
}
/* ========================================================================== */