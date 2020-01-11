/* memory.h */

#ifndef MEMORY_MODULE
#define MEMORY_MODULE

#include <stdbool.h>    // bool
#include <stdint.h>     // uint8_t, uint32_t, size_t
#include <time.h>       // struct timespec

#define NUM_OF_PROCESSES 2

enum pg_rep_alg { LRU, WS };     /* Page replacement algorithm */

struct vmem_entry;          
struct virtual_memory;
struct working_set_comp;    /* Forward Declarations */
struct mmem_entry;
struct main_memory;
struct memory;

struct mmem_entry         /* Main Memory Entry */
{
  bool set; 
  bool modified;
  uint16_t offset;
  struct timespec latency;  // Time of last reference
};

struct main_memory
{
  struct mmem_entry *entries;
  size_t mm_size;
};

struct memory
{
  struct main_memory    *mmem;
  struct virtual_memory *vmem;

  size_t hd_reads;       // # Hard Disk Reads/Writes
  size_t hd_writes;
  size_t page_fs;        // # Page Faults
  size_t total_req;      // # Requests to the virtual memory
};

struct vmem_entry        /* Virtual Memory Entry */
{
  bool set;
  uint8_t  pid;          // Process that owns the page
  uint32_t addr;         // Page address
};


struct virtual_memory      /* Virtual memory segment */
{
  struct vmem_entry *ipt;        //  IPT 
  size_t ipt_size;               //  # frames
  size_t ipt_curr;               //  # occupied frames

  enum pg_rep_alg pg_repl;       // Page Replacement Algorithm

  struct working_set_comp *ws;   // Working Set tools
};

struct working_set_comp
{
  size_t window_s;                // History Window size
  uint8_t *history_index;         // Matches a History Window with a PID

  struct queue *history[NUM_OF_PROCESSES];     // Array of History Windows
  struct queue *set;
};

/* Initializes the memory segment and returns a pointer to it.    *
 * Requires: 1) # of frames    2) Page Replacement algrorithm     *
 * 3) Array of associated PIDs 4) Working Set window size         *
 * Note: 4th arg is ignored if not for the Working Set algorithm  */
struct memory* mem_init(size_t frames, enum pg_rep_alg alg, uint8_t *pids, size_t ws_wnd_s);

/* Requests an address from the memory, and applies `mode` operation to it. *
 * Requires: 1) ptr to memory segment 2) Address to retrieve                *
 * 3) Mode ('R'/'W') 4) PID of the process making the request               */
void mem_retrieve(struct memory *mem, uint32_t addr, char mode, uint8_t pid);

/* Outputs stats about memory usage. */
void mem_stats(struct memory *mem);

/* Deallocates space used for the memory segment */
void mem_clean(struct memory *mem);


#endif