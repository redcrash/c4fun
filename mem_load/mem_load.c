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
#include <numa.h>
//#include <numaif.h>


#define ONE     p = (uint64_t *)*p;
#define FIVE    ONE ONE ONE ONE ONE
#define	TEN	FIVE FIVE
#define	FIFTY	TEN TEN TEN TEN TEN
#define	HUNDRED	FIFTY FIFTY

void usage(const char *prog_name) {
  printf ("Usage: %s size mode node nb_iter\n"
	  "  size is the size of allocated and accessed memory in kb\n"
	  "  mode is either seq or rand for sequential or random accesses\n"
	  "  node is the NUMA node where memory must be explicitely allocated (-1 for standard malloc)\n"
	  "  nb_iter is the number of time the iteration reading over one hundred elements is done (-1 == infinite loop)\n", prog_name);
}

int main(int argc, char **argv) {

  /**
   * Check and get arguments.
   */
  if (argc != 5) {
    usage(argv[0]);
    return -1;
  }
  size_t size_in_bytes = atol(argv[1]) * 1024;
  enum access_mode_t access_mode;
  if (!strcmp(argv[2], "seq")) {
    access_mode = access_seq;
  } else if (!strcmp(argv[2], "rand")) {
    access_mode = access_rand;
  } else {
    printf("Unknown access_mode %s\n", argv[2]);
    usage(argv[0]);
    return -1;
  }
  char node = atoi(argv[3]);
  register int nb_iter = atoi(argv[4]);

  /**
   * Allocate and fill memory
   */
  uint64_t *memory;
  if (node == -1) {
    memory = malloc(size_in_bytes);
  } else {
    memory = numa_alloc_onnode(size_in_bytes, node);
  }
  assert(memory);
  fill_memory(memory, size_in_bytes, access_mode);
  register uint64_t *p = memory;

  if (nb_iter == -1) {
    /**
     * Infinite loop
     */
    while (1) {
      HUNDRED
    }
  } else {
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);
    int register i = 0;
    while (i < nb_iter) {
      i++;
      HUNDRED;
    }
    clock_gettime(CLOCK_REALTIME, &end);
    uint64_t ns = (end.tv_sec * 1E9 + end.tv_nsec) - (start.tv_sec * 1E9 + start.tv_nsec);
    fprintf(stderr, "time = %" PRIu64 " ns\n", ns);
  }

  return 0;
}