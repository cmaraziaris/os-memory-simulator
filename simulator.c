/* simulator.c */
// TODO: Error handle when WS is given but with 3 args
#include <errno.h>    // perror
#include <stdbool.h>  // bool
#include <stdint.h>   // int32, int8
#include <stdio.h>
#include <stdlib.h>   // atoi, exit
#include <string.h>   // strcpy

#include "memory.h"   // enum pg_rep_alg, NUM_OF_PROCESSES

#define PATH1 "./traces/bzip.trace"   /* 1st file of memory references */
#define PATH2 "./traces/gcc.trace"    /* 2nd file of memory references */

enum error_t
{ 
  INVALID_NUM_ARGS,      /* Input errors */
  INVALID_ALG, 
  WS_NO_WINDOW_S 
};

/* ========================================================================== */

/* Opens a file and performs error handling. */
static FILE *file_open(char *path, char *mode);

/* Closes a file and perfrorms error handling. */
static void  file_close(FILE *f);

/* Handle logic errors from the user's input. */
static void  error_handle(enum error_t error);

/* Reads a reference and its mode (R/W) from a file. */
static int   read_ref(FILE *file, uint32_t *paddr, char *pmode);

/* ========================================================================== */

/* Arguments: 
 * 1) Page Repl Alg. {"LRU", "WS"}   *
 * 2) #Frames                        *
 * 3) Set of q refs to be read       *
 * 4) Working Set window             *
 * 5) Maximum references to be read  *
 * Note: 4-5 are optional args       */

int main(int argc, char const *argv[])
{
  char repl_alg[4];   /* Replacement algorithm */
  size_t q;  
  size_t frames;
  size_t ws_wind  = 0;   /* Working Set window   */
  size_t max_refs = 0;   /* Maximum # references */

  switch(argc)   /* Decode the command line arguments */
  {
    case 6:
      max_refs = atoi(argv[5]);   /* Optional arg */
    case 5:
      ws_wind  = atoi(argv[4]);   /* Optional arg */
    case 4:
      q = atoi(argv[3]);
      frames = atoi(argv[2]);
      strcpy(repl_alg, argv[1]);
      break;

    default: error_handle(INVALID_NUM_ARGS);
  }

  enum pg_rep_alg page_repl;    /* Set the page replacement algorithm */
  if (!strcmp(repl_alg, "LRU"))
    page_repl = LRU;
  else if (!strcmp(repl_alg, "WS"))
    page_repl = WS;
  else
    error_handle(INVALID_ALG);

  if (page_repl == WS && ws_wind == 0) error_handle(WS_NO_WINDOW_S);

  /* Specify PIDs we're using */
  int8_t pids[NUM_OF_PROCESSES] = { 0, 1 };    /* 2 processes */

  /* Initialize memory segment */
  struct memory *my_mem = mem_init(frames, page_repl, pids, ws_wind);

  FILE *tr1 = file_open(PATH1, "r");
  FILE *tr2 = file_open(PATH2, "r");

  size_t refs_rd = 0; /* References read */
  bool end = 0;       /* EOF Check */

  /* Loop while max_refs is not specified or we haven't reached it */
  while (refs_rd < max_refs || max_refs == 0)
  {
    uint32_t addr;
    char mode;   /* 'R' or 'W' */
    
    for (size_t i = 0; i < q; ++i)  /* Read a total of q references */
    {
      if (read_ref(tr1, &addr, &mode) == 0 && (end = 1))
        break;

      ++refs_rd;
      /* Retrieve address from memory */
      mem_retrieve(my_mem, addr, mode, pids[0]);
    }

    for (size_t i = 0; i < q; ++i)
    {
      if (read_ref(tr2, &addr, &mode) == 0 && (end = 1))
        break;

      ++refs_rd;
      /* Retrieve address from memory */
      mem_retrieve(my_mem, addr, mode, pids[1]);
    }

    if (end == 1) break; /* We reached EOF in at least 1 file  */
  }

  /* Print stats */
  mem_stats(my_mem);
  printf("References examined: %lu\n", refs_rd);

  mem_clean(my_mem);  /* Cleanup the memory used */

  file_close(tr1);
  file_close(tr2);
  
  return EXIT_SUCCESS;
}

/* ========================================================================== */

/* Handle logic errors from the user's input */
static void error_handle(enum error_t error)
{
  switch(error)
  {
    case INVALID_NUM_ARGS:
      fprintf(stderr, "Invalid number of arguments given. Min: 3, Max: 5\n");
      exit(EXIT_FAILURE);

    case INVALID_ALG:
      fprintf(stderr, "Invalid page replacement algorithm given. \
Options are: { LRU, WS }, case sensitive!\n");
      exit(EXIT_FAILURE);

    case WS_NO_WINDOW_S:
      fprintf(stderr, "Working Set algorithm was chosen, \
but no window size specified.\n");
      exit(EXIT_FAILURE); 
  }
}

/* ========================================================================== */

/* Reads a reference and its mode (R/W) from a file. *
 * Stores the values to the pointers passed as args. *
 * Returns 0 if we reached the end of file, else 1.  */
static int read_ref(FILE *file, uint32_t *paddr, char *pmode)
{
  if (feof(file)) return 0;

  fscanf(file, "%x ", paddr);
  fscanf(file, "%c ", pmode);

  return 1;
}
/* ========================================================================== */

/* Opens a file and performs error handling. *
 * Return a pointer to the file.             */
static FILE *file_open(char *path, char *mode)
{
  FILE *f = fopen(path, mode);
  if (f == NULL)
  {
    perror("fopen");
    exit(EXIT_FAILURE);
  }
  return f;
}
/* ========================================================================== */

/* Closes a file and perfrorms error handling. */
static void file_close(FILE *f)
{
  if (fclose(f) != 0)
  {
    perror("fclose");
    exit(EXIT_FAILURE);
  }
}
/* ========================================================================== */