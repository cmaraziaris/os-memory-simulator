/* simulator.c */
// TODO: Error handle when WS is given but with 3 args
#include <errno.h>    // perror
#include <stdbool.h>  // bool
#include <stdint.h>   // int32, int8
#include <stdio.h>
#include <stdlib.h>   // atoi, exit
#include <string.h>   // strcpy

/* Reads a reference and its mode (R/W) from a file. *
 * Stores the values to the pointers passed as args. *
 * Returns 0 if we reached the end of file, else 1.  */
static int read_ref(FILE *file, int32_t *paddr, char *pmode)
{
  //char ref[12];
  //if (fgets(ref, 12, file) == NULL)
  //  return 0;

  //char addr[9];
  //strncpy(addr, ref, 8);
  //addr[8] = '\0';

  //char mode = ref[9];

  //*pmode = mode;
 // uint32_t addr;

  fscanf(file, "%x ", paddr);
  fscanf(file, "%c ", pmode);

  //*paddr = (int32_t) addr;
  //sscanf(addr, "%x", paddr);
  //*paddr = (int32_t) strtol(addr, NULL, 16);
  return 1;
}

#define PATH1 "./traces/bzip.trace"   /* 1st file of memory references */
#define PATH2 "./traces/gcc.trace"    /* 2nd file of memory references */

int main(int argc, char const *argv[])
{
  FILE *tr1 = fopen(PATH1, "r");
  FILE *tr2 = fopen(PATH2, "r");

  int32_t addr;
  char mode;

  for (int i = 0; i < 89000; ++i)
  {
    for (int j = 0; j < 5; ++j)
    {
      read_ref(tr1, &addr, &mode);
      printf("%10d %c\n", addr, mode);
    }

    for (int k = 0; k < 5; ++k)
    {    
      read_ref(tr2, &addr, &mode);
      printf("%10d %c\n", addr, mode);
    }
  }

  fclose(tr1);
  fclose(tr2);

  return 0;
}