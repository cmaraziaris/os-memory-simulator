/* page_repl.c */
// TODO: comments here and to the respective .h
#include <stdlib.h>         // exit
#include <stdint.h>         // size_t, int8_t
#include <stdio.h>
#include <time.h>           // struct timespec

#include "memory.h"
#include "page_repl.h"
#include "queue.h"


// Remove an entry from the IPT/Main Memory, write the page in HD if necessary.
static void   rm_entry(struct memory *mem, size_t index);

// Create a set of every distinct page found in the history window.
static void   make_set(struct queue *set, struct queue **history, size_t index);

// Get the WS History Window index associated with the `pid` given.
static size_t find_history_window(struct virtual_memory *vm, int8_t pid);

/* ========================================================================= */

size_t lru(struct memory *mem)
{
  struct main_memory *mm = mem->mmem;

  size_t pos = 0;          // Contains the index of the oldest entry

  struct timespec min_t;
  min_t.tv_sec  = mm->entries[0].latency.tv_sec;
  min_t.tv_nsec = mm->entries[0].latency.tv_nsec;

  for (size_t i = 1; i < mm->mm_size; ++i)      // Compare every entry's time
  {                                             // of last reference
    time_t sec  = mm->entries[i].latency.tv_sec;
    long   nsec = mm->entries[i].latency.tv_nsec;

    if (sec < min_t.tv_sec || (sec == min_t.tv_sec && nsec < min_t.tv_nsec))
    {
      min_t.tv_sec  = sec;
      min_t.tv_nsec = nsec;

      pos = i;
    }
  }

  rm_entry(mem, pos);     // Remove the oldest page
  return pos;             // Return the index of an empty IPT slot
}

/* ========================================================================= */

void ws_update_history_window(struct virtual_memory *vm, uint8_t pid, uint32_t page)
{
  struct vmem_entry entry = { 1, pid, page };

  size_t index = find_history_window(vm, pid);
    
  if (queue_is_full(vm->ws->history[index], vm->ws->window_s))
    queue_emplace_last(vm->ws->history[index], entry);        // Removes first ref, adds current ref as last
  else
    queue_insert_last(vm->ws->history[index], entry);         // Add refs until it's full
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

  vm->ws->set = queue_initialize();       // Create the set

  make_set(vm->ws->set, vm->ws->history, find_history_window(vm, pid));

  size_t empty = (size_t) -1;         // Index of an empty IPT slot 
  size_t last  = (size_t) -1;         // Greatest IPT index occupied by proccess `pid`

  for (size_t i = 0; i < vm->ipt_size; ++i)
  {
    if (vm->ipt[i].pid != pid) continue;    // Process doesn't own this IPT entry

    last = i;
    struct vmem_entry entry = { 1, vm->ipt[i].pid, vm->ipt[i].addr };

    if (queue_search(vm->ws->set, entry) == 0)       // Ref not in the set
    {
      rm_entry(mem, i);         // Remove it from the IPT
      empty = i;
    }
  }

  // Edge cases
  if (last == (size_t)-1)       // IPT is full with refs from the other process
  {                             // so the current process has to be suspended/terminated
    printf("Starvation!"); 
    exit(EXIT_FAILURE);         // Termination (demonstration purposes)
  }
  
  if (empty == (size_t)-1)      // Every distinct ref in the History Window is also in the set 
  {
    rm_entry(mem, last);        // So just remove the last IPT entry owned by `pid` found
    empty = last;
  }

  queue_destroy(vm->ws->set);       // Destroy the set

  return empty;       // Return the index of an empty IPT slot
}

/* ========================================================================= */

static void make_set(struct queue *set, struct queue **history, size_t index)
{
  struct queue_node *curr = history[index]->front;   // Get `pid` history window
  while (curr)
  {
    queue_sorted_insert(set, curr->data);     // Insert in the set
    curr = curr->next;
  }
}

/* ========================================================================= */

static void rm_entry(struct memory *mem, size_t index)
{
  if (mem->mmem->entries[index].modified == 1)     // Write in the HD
    ++mem->hd_writes;

  mem->vmem->ipt[index].set = 0;            // Remove from the IPT 
  mem->mmem->entries[index].set = 0;        // Remove from Main Memory
    
  --mem->vmem->ipt_curr;
}

/* ========================================================================= */