#include "mem_alloc.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <inttypes.h>
#include <assert.h>
#include <time.h> // For clock_gettime
#include <sys/time.h> // For gettimeofday
#include <string.h>
#include <cpuid.h>

#define ONE     p = (uint64_t *)*p;
#define FIVE    ONE ONE ONE ONE ONE
#define	TEN	FIVE FIVE
#define	FIFTY	TEN TEN TEN TEN TEN
#define	HUNDRED	FIFTY FIFTY

void usage(const char *prog_name) {
  printf ("Usage %s: size_in_kb mode (where mode is either seq or rand)\n", prog_name);
}

int main(int argc, char **argv) {

 /**
  * Check and get arguments.
  */
  if (argc != 3) {
    usage(argv[0]);
    return -1;
  }
  size_t size = atol(argv[1]) * 1024;
  enum access_mode_t access_mode;
  if (!strcmp(argv[2], "seq")) {
    access_mode = access_seq;
  } else if (!strcmp(argv[2], "rand")) {
    access_mode = access_rand;
  } else {
    printf("Unknown access_mode %s\n", argv[3]);
    usage(argv[0]);
    return -1;
  }

  /**
   * Allocate and fill memory
   */
  uint64_t *memory = malloc(size);
  assert(memory);
  fill_memory(memory, size, access_mode);
  register uint64_t *p = memory;

  /**
   * Infinite loop
   */
  while (1) {
    HUNDRED
  }

  return 0;
}
