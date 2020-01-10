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
#include "queue_types.h"  // queue

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

  if (vm->pg_repl == WS) //TODO : break it to funcs
  {
    struct vmem_entry entry = { 1, pid, page };
    
    if (is_queue_full(vm->ws->history[pid], vm->ws->window_s))
      queue_remove_first(vm->ws->history[pid]);

    queue_insert_last(vm->ws->history[pid], entry);
  }

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

      vm->ipt[pos].set = 0;
 
      /* Place the new page */
      set_new_entry(mem, pos, page, pid, mode, t, offset);  
    }

    else if (vm->pg_repl == WS) /* Working Set algorithm */
    {
      //for (size_t i = 0; i < NUM_OF_PROCESSES; ++i) /* Init the Sets */
      vm->ws->set = queue_initialize();

      //size_t window = vm->ws->history->size;

      
      //for (size_t i = 0; i < window; ++i)
      //{
        //struct vmem_entry item = queue_remove_first(vm->ws->history);
        
        //if (item.pid == pid) queue_sorted_insert(vm->ws->set, item);

        //queue_insert_last(vm->ws->history, item); // xd

        // create the set!
        struct queue_node *curr = vm->ws->history[pid]->front;
        while (curr)
        {
          //if (is_queue_full(vm->ws->set, 1)) break;
          if (curr->data.pid == pid) queue_sorted_insert(vm->ws->set, curr->data);
          curr = curr->next;
        }

      size_t empty = -1;
      size_t last = -1;

      for (size_t i = 0; i < vm->ipt_size; ++i)
      {
        if (vm->ipt[i].set == 0 || vm->ipt[i].pid != pid) continue; // process doesnt own this IPT block, dont touch

        last = i;
        struct vmem_entry entry = { 1, vm->ipt[i].pid, vm->ipt[i].addr };

        if (queue_search(vm->ws->set, entry) == 0) // if not in the set, remove from the page table
        {
          if (mm->entries[i].modified == 1) 
            ++mem->hd_writes;

          vm->ipt[i].set = 0; // removed
          mm->entries[i].set = 0;
          --vm->ipt_curr;

          empty = i;
        }
      }

      if (last == -1) { printf("starvation!"); exit(0); }
      else if (empty == -1) {
        --vm->ipt_curr;
        empty = last;
      } 
        // just insert the new page at an empty spot on the ipt
      set_new_entry(mem, empty, page, pid, mode, t, offset);
      
      ++vm->ipt_curr;
      
      /* Update IPT + Working Set */
      queue_destroy(vm->ws->set);
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