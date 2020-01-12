/* memory_structs.h */
#ifndef MEMORY_STRUCTS
#define MEMORY_STRUCTS

#include <stdbool.h>    // bool
#include <stdint.h>     // uint8_t, uint32_t, size_t
#include <time.h>       // struct timespec

#define NUM_OF_PROCESSES 2

enum algorithm { LRU, WS };     // Page replacement algorithm

struct memory;
struct main_memory;
struct virtual_memory;
struct mmem_entry;
struct vmem_entry;          
struct working_set_comp;    // Forward Declarations


// Memory segment
struct memory
{
  struct main_memory    *mmem;
  struct virtual_memory *vmem;

  size_t hd_reads;            // # Hard Disk Reads/Writes
  size_t hd_writes;
  size_t page_fs;             // # Page Faults
  size_t total_req;           // # Requests to the virtual memory
};


// Main memory segment
struct main_memory
{
  struct mmem_entry *entries;
  size_t mm_size;
};


// Virtual memory segment
struct virtual_memory      
{
  struct vmem_entry *ipt;        //  IPT 
  size_t ipt_size;               //  # frames
  size_t ipt_curr;               //  # occupied frames

  enum algorithm pg_repl;       // Page Replacement Algorithm

  struct working_set_comp *ws;   // Working Set tools
};


// Main Memory Entry
struct mmem_entry         
{
  bool set; 
  bool modified;
  uint16_t offset;
  struct timespec latency;        // Time of last reference
};


// Virtual Memory Entry
struct vmem_entry        
{
  bool set;
  uint8_t  pid;           // Process that owns the page
  uint32_t addr;          // Page address
};


struct working_set_comp
{
  size_t window_s;                // History Window size
  uint8_t *history_index;         // Matches a History Window with a PID

  struct queue *history[NUM_OF_PROCESSES];     // Array of History Windows
  struct queue *set;
};


#endif