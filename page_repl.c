/* page_repl.c */

#include <stdbool.h>  // bool
#include <stddef.h>   // NULL
#include <stdint.h>   // size_t, int8_t
#include <time.h>     // struct timespec

#include "memory.h"
#include "page_repl.h"

#if 0
/* Remove a page from the IPT, write in HD if necessary.  */
static void   rm_page(struct memory *mem, struct mem_entry *entry);

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
#if 0
/* ========================================================================= */

/* Update the working set of a specifid PID,    *
 * based on the latest page that was referenced */
void working_set(struct memory *mem, struct mem_entry *entry)
{
  size_t ws_pos = find_wset(mem, entry->pid);    /* Get WS index */

  /* Check if the page is already in the WS */
  for (size_t i = 0; i < mem->ws[ws_pos].window_s; ++i)
  {
    struct mem_entry *rec = mem->ws[ws_pos].pages[i];

    if (rec != NULL && rec->addr == entry->addr)  /* Found */
      return;
  }

  /* The page is not in the WS */

  /* If the WS is not full, place the page in */
  if (mem->ws[ws_pos].curr != mem->ws[ws_pos].window_s)
  {
    for (size_t i = 0; i < mem->ws[ws_pos].window_s; ++i)
    {
      if (mem->ws[ws_pos].pages[i] == NULL) /* Found empty slot */
      {
        mem->ws[ws_pos].pages[i] = entry;
        ++mem->ws[ws_pos].curr;

        return;
      }
    }
  }

  /* If the WS is full, remove the oldest entry and place the new one */
  size_t victim = find_oldest(mem, entry, ws_pos);
  if (victim != -1)
  {
    /* Remove page from main memory */
    rm_page(mem, mem->ws[ws_pos].pages[victim]);

    struct mem_entry *new_entry = mem->ws[ws_pos].pages[victim];

    /* Place the new page to the IPT + WS, in the old one's position */
    new_entry->addr = entry->addr;
    new_entry->pid  = entry->pid;
    new_entry->latency  = entry->latency;
    new_entry->modified = entry->modified;
  }

}

/* ========================================================================= */

/* Remove a page from the IPT, write in HD if necessary */
static void rm_page(struct memory *mem, struct mem_entry *entry)
{
  if (entry->modified == 1) ++mem->hd_writes;

  entry->set = 0;

  --mem->ipt_curr;
}

/* ========================================================================= */

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