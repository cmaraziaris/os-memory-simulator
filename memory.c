/* memory.c */

#include <assert.h>     // for malloc check
#include <stdio.h>      // printf
#include <stdbool.h>    // bool
#include <stdint.h>     // size_t, uint32_t, int8_t
#include <stdlib.h>     // malloc, calloc, free, NULL
#include <time.h>       // timespec, clock_gettime

#include "memory.h"       // enum pg_rep_alg, NUM_OF_PROCESSES
#include "page_repl.h"    // lru(), working_set()

/* Initializes a memory entry with given values */
static void set_new_entry(struct mem_entry *entry, uint32_t addr, int8_t pid, char mode, struct timespec t);

/* ========================================================================== */

/* Search in main memory, and then in the HD, for an address requested *
 * from process `pid` to perform the `mode` operation (R/W)            */
void mem_retrieve(struct memory *mem, uint32_t addr, char mode, int pid)
{
  ++mem->total_req;

  uint32_t page = addr >> 12;  /* Remove offset */

  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t); /* Keep the time of reference */

  /* Linear IPT search */
  for (size_t i = 0; i < mem->ipt_size; ++i)
  {
    if (mem->ipt[i].set && mem->ipt[i].addr == page)  /* Already in the IPT */
    {
      if (mode == 'W')
        mem->ipt[i].modified = 1;  /* Write operation */

      mem->ipt[i].latency = t;     /* Update timestamp */

      if (mem->pg_repl == WS)      /* Update Working Set */
        working_set(mem, &mem->ipt[i]);

      return; /* Page was found in the IPT */
    }
  }

  /* Page not found in the main memory, so it will be read from the HD */
  ++mem->hd_reads; 
  ++mem->page_fs;

  /* If IPT is not full, find a slot for the page */
  if (mem->ipt_curr != mem->ipt_size)
  {
    size_t i;
    for (i = 0; i < mem->ipt_size; ++i)
    {
      if (mem->ipt[i].set == 0)   /* Empty page */
      { 
      /* Place the new page */
        set_new_entry(&mem->ipt[i], page, pid, mode, t);

        ++mem->ipt_curr;      /* Update occupied slots */
      
        break; /* Page was read from the HD and written to the IPT */
      }
    }

    if (mem->pg_repl == WS)        /* Update Working Set */
      working_set(mem, &mem->ipt[i]);
  }

  else   /* If IPT is full, perform a page replacement algorithm */
  {
    if (mem->pg_repl == LRU)  /* Least Recently Used algorithm */
    {
      size_t pos = lru(mem);    /* Decide a victim page */

       /* Remove the victim */
      if (mem->ipt[pos].modified == 1) ++mem->hd_writes;
      mem->ipt[pos].set = 0;
 
      /* Place the new page */
      set_new_entry(&mem->ipt[pos], page, pid, mode, t);
    }

    else if (mem->pg_repl == WS) /* Working Set algorithm */
    {
      struct mem_entry entry;   /* Create a page */
      set_new_entry(&entry, page, pid, mode, t); 

      working_set(mem, &entry); /* Update IPT + Working Set */
    }
  }
}
/* ========================================================================== */

/* Initializes a memory entry */
static void set_new_entry(struct mem_entry *entry, uint32_t addr, int8_t pid, char mode, struct timespec t)
{
  entry->set  = 1;
  entry->addr = addr;
  entry->pid  = pid;
  entry->latency = t; 
  
  if (mode == 'W')
    entry->modified = 1;
  else
    entry->modified = 0;   
}

/* ========================================================================== */

/* Initialize the main memory segment. Requires an array of PIDs used. */
struct memory *mem_init(size_t frames, enum pg_rep_alg alg, int8_t *pids, size_t ws_wnd_s)
{
  struct memory *mem = malloc(sizeof(struct memory));
  assert(mem != NULL);

  mem->ipt = malloc(frames * sizeof(struct mem_entry)); /* Create the IPT */
  assert(mem->ipt != NULL);

  mem->ipt_size = frames;

  for (size_t i = 0; i < mem->ipt_size; ++i) /* Initialize IPT entries */
    mem->ipt[i].set = 0; 

  mem->hd_reads  = mem->hd_writes = mem->page_fs = 0; /* Initialize counters */
  mem->total_req = mem->ipt_curr  = 0;

  mem->pg_repl = alg;

  if (alg == WS) /* Working Set algorithm */
  {
    /* Create a Working Set for each process */
    mem->ws = malloc(NUM_OF_PROCESSES * sizeof(struct working_set));
    assert(mem->ws != NULL);
    
    for (size_t i = 0; i < NUM_OF_PROCESSES; ++i) /* Init the W.Sets */
    {
      mem->ws[i].curr = 0;
      mem->ws[i].window_s = ws_wnd_s;
      mem->ws[i].pid = pids[i];
      mem->ws[i].pages = calloc(mem->ws[i].window_s, sizeof(struct mem_entry *));
      assert(mem->ws[i].pages != NULL);
    }
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

  printf("\nTotal frames: %lu\n", mem->ipt_size);
}
/* ========================================================================== */

/* Deallocate space used from the memory segment */
void mem_clean(struct memory *mem)
{
  if (mem->pg_repl == WS) /* Deallocate Working Set components */
  {
    for (size_t i = 0; i < NUM_OF_PROCESSES; ++i)
      free(mem->ws[i].pages);

    free(mem->ws);
  }

  free(mem->ipt);   /* Deallocate the memory segment */
  free(mem);
}
/* ========================================================================== */