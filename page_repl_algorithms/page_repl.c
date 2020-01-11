/* page_repl.c */
// TODO: comments here and to the respective .h
#include <stdlib.h>         // exit
#include <stdint.h>         // size_t, int8_t
#include <stdio.h>
#include <time.h>           // struct timespec

#include "memory.h"
#include "page_repl.h"
#include "queue.h"


/* Remove a page from the IPT/Main Memory, write it in HD if necessary.  */
static void   rm_page(struct memory *mem, size_t index);

/* Create a set of every distinct page found in the history window. */
static void   make_set(struct queue *set, struct queue **history, size_t index);

/* Get the WS History Window index associated with the `pid` given. */
static size_t find_history_window(struct virtual_memory *vm, int8_t pid);

/* ========================================================================= */
/* Return the index of the oldest entry in the main memory */
size_t lru(struct memory *mem)
{
  struct main_memory *mm = mem->mmem;

  size_t pos = 0;

  struct timespec min_t;
  min_t.tv_sec  = mm->entries[0].latency.tv_sec;
  min_t.tv_nsec = mm->entries[0].latency.tv_nsec;

  for (size_t i = 1; i < mm->mm_size; ++i)      /* Compare every entry's timestamps */
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

  rm_page(mem, pos);
  return pos;
}

/* ========================================================================= */

void ws_update_history_window(struct virtual_memory *vm, uint8_t pid, uint32_t page)
{
  struct vmem_entry entry = { 1, pid, page };

  size_t index = find_history_window(vm, pid);
    
  if (queue_is_full(vm->ws->history[index], vm->ws->window_s))
    queue_emplace_last(vm->ws->history[index], entry);
  else
    queue_insert_last(vm->ws->history[index], entry);
}

/* ========================================================================= */

static size_t find_history_window(struct virtual_memory *vm, int8_t pid)
{
  for (size_t i = 0; i < NUM_OF_PROCESSES; ++i) {
    if (vm->ws->history_index[i] == pid)
      return i;
  }
}
/* ========================================================================= */

size_t working_set(struct memory *mem, uint8_t pid)
{
  struct virtual_memory *vm = mem->vmem;

  vm->ws->set = queue_initialize();

  make_set(vm->ws->set, vm->ws->history, find_history_window(vm, pid));

  size_t empty = (size_t) -1; // 0xFFFFFFFF, max size_t value
  size_t last  = (size_t) -1;

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

  if (last == (size_t)-1) 
  { 
    printf("Starvation!"); 
    exit(EXIT_FAILURE); 
  }
  
  if (empty == (size_t)-1) 
  {
    rm_page(mem, last);
    empty = last;
  }

  queue_destroy(vm->ws->set);

  return empty;
}

/* ========================================================================= */

static void make_set(struct queue *set, struct queue **history, size_t index)
{
  struct queue_node *curr = history[index]->front;
  while (curr)
  {
    queue_sorted_insert(set, curr->data);     // Insert in the set
    curr = curr->next;
  }
}

/* ========================================================================= */

static void rm_page(struct memory *mem, size_t index)
{
  if (mem->mmem->entries[index].modified == 1)     // Write in the HD
    ++mem->hd_writes;

  mem->vmem->ipt[index].set = 0;            // Remove from the IPT 
  mem->mmem->entries[index].set = 0;        // Remove from Main Memory
    
  --mem->vmem->ipt_curr;
}

/* ========================================================================= */