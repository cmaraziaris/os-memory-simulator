/* page_repl.c */

#include <stdbool.h>  // bool
#include <stddef.h>   // NULL
#include <stdlib.h>   // exit
#include <stdint.h>   // size_t, int8_t
#include <stdio.h>
#include <time.h>     // struct timespec

#include "memory.h"
#include "page_repl.h"
#include "queue.h"


/* Remove a page from the IPT, write in HD if necessary.  */
static void   rm_page(struct memory *mem, size_t index);
#if 0
/* Get the WS index associated with the current PID. */
static size_t find_wset(struct memory *mem, int8_t pid);

/* Get the index of the oldest page in the WS.  */
static size_t find_oldest(struct memory *mem, struct mem_entry *entry, size_t ws_pos);
#endif

/* ========================================================================= */
/* Return the index of the oldest entry in the main memory */
size_t lru(struct memory *mem)
{
  struct main_memory *mm = mem->mmem;

  size_t pos = 0;

  struct timespec min_t;
  min_t.tv_sec  = mm->entries[0].latency.tv_sec;
  min_t.tv_nsec = mm->entries[0].latency.tv_nsec;

  /* Compare every entry's timestamps */
  for (size_t i = 1; i < mm->mm_size; ++i)
  {
    time_t sec  = mm->entries[i].latency.tv_sec;
    long   nsec = mm->entries[i].latency.tv_nsec;

    if (sec < min_t.tv_sec || (sec == min_t.tv_sec && nsec < min_t.tv_nsec))
    {
      min_t.tv_sec  = sec;
      min_t.tv_nsec = nsec;

      pos = i;
    }
  }

  return pos;
}

/* ========================================================================= */

void ws_update_history_window(struct virtual_memory *vm, uint8_t pid, uint32_t page)
{
  struct vmem_entry entry = { 1, pid, page };
    
  if (is_queue_full(vm->ws->history[pid], vm->ws->window_s))
    queue_remove_first(vm->ws->history[pid]);

  queue_insert_last(vm->ws->history[pid], entry);
}

/* ========================================================================= */

static void ws_make_set(struct queue *set, struct queue **history, uint8_t pid)
{
  // TODO: better match pids with history queues
  struct queue_node *curr = history[pid]->front;
  while (curr)
  {
    if (curr->data.pid == pid) queue_sorted_insert(set, curr->data);
    curr = curr->next;
  }
}
/* ========================================================================= */

size_t working_set(struct memory *mem, uint8_t pid)
{
  struct virtual_memory *vm = mem->vmem;
  struct main_memory    *mm = mem->mmem;

  vm->ws->set = queue_initialize();

  ws_make_set(vm->ws->set, vm->ws->history, pid);

  size_t empty = -1;
  size_t last  = -1;

  for (size_t i = 0; i < vm->ipt_size; ++i)
  {
    if (vm->ipt[i].pid != pid) continue; // process doesnt own this IPT block, dont touch

    last = i;
    struct vmem_entry entry = { 1, vm->ipt[i].pid, vm->ipt[i].addr };

    if (queue_search(vm->ws->set, entry) == 0) // if not in the set, remove from the page table
    {
      rm_page(mem, i);

      empty = i;
    }
  }

  if (last == -1) 
  { 
    printf("Starvation!"); 
    exit(EXIT_FAILURE); 
  }
  
  if (empty == -1) 
  {
    //--vm->ipt_curr;
    rm_page(mem, last);
    empty = last;
  }

  queue_destroy(vm->ws->set);
  return empty;
}

/* ========================================================================= */

/* Remove a page from the IPT, write in HD if necessary */
static void rm_page(struct memory *mem, size_t index)
{
  if (mem->mmem->entries[index].modified == 1) 
    ++mem->hd_writes;

  mem->vmem->ipt[index].set = 0; // removed
  mem->mmem->entries[index].set = 0;
    
  --mem->vmem->ipt_curr;
}

/* ========================================================================= */
#if 0
/* Get the WS index associated with the current PID */
static size_t find_wset(struct memory *mem, int8_t pid)
{
  for (size_t i = 0; i < NUM_OF_PROCESSES; ++i)
    if (mem->ws[i].pid == pid)
      return i;
  return 0;
}

/* ========================================================================= */
#endif