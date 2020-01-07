/* memory.h */

#ifndef MEMORY_MODULE
#define MEMORY_MODULE

#include <stdbool.h>    // bool
#include <stdint.h>     // uint32_t, size_t
#include <time.h>       // struct timespec

#define NUM_OF_PROCESSES 2

enum pg_rep_alg { LRU, WS }; /* Page replacement algorithm */


struct mem_entry  /* Memory entry / frame info stored */
{
  bool set; 
  bool modified;
  int8_t pid;     /* Occupied by process pid - WS only */
  uint32_t addr;
  struct timespec latency;  /* Time of last reference */
};


struct working_set
{
  int8_t pid;         /* WS for process pid */
  size_t curr;        /* # of slots filled  */
  size_t window_s;    /* Window size        */
  struct mem_entry **pages;  /* Array of pointers to pages/frames */
};


struct memory     /* Main memory segment */
{
  struct mem_entry *ipt; /* Inverse Page Table */
  size_t ipt_size;       /* # frames           */
  size_t ipt_curr;       /* # occupied frames  */
  
  size_t hd_reads;       /* # Hard Disk Reads/Writes */
  size_t hd_writes;
  size_t page_fs;        /* # Page Faults */

  size_t total_req;      /* # Requests to the virtual memory */

  enum pg_rep_alg pg_repl;  /* Page Replacement Algorithm */

  struct working_set *ws;   /* Array of Working Sets - WS algorithm only */
};

/* Initialize main memory segment and returns a pointer to it              *
 * Requires: 1) # of frames    2) Page Replacement algrorithm              *
 * 3) Array of associated PIDs 4) Working Set window size                  *
 * Note: 3rd and 4th args are ignored if not for the Working Set algorithm */
struct memory* mem_init(size_t frames, enum pg_rep_alg alg, int8_t *pids, size_t ws_wnd_s);

/* Search in main memory, and then in the HD, for an address requested   *
 * Requires: 1) ptr to memory segment 2) Address to search 3) Mode (R/W) *
 * 4) PID of the page referenced. Note: 4th arg is ignored if not WS     */
void mem_retrieve(struct memory *mem, uint32_t addr, char mode, int pid);

/* Outputs stats about memory usage:          *
 * # HD Reads/Writes, # Page Faults, # Frames */
void mem_stats(struct memory *mem);

/* Deallocates spaces used from the memory segment */
void mem_clean(struct memory *mem);


#endif