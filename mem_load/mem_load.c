#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <time.h> // For clock_gettime()
#include <numa.h>
#include <sys/sysinfo.h> // For get_nprocs()
#include <sched.h> // For sched_setaffinity

#include "mem_alloc.h"

/* #define ONE      asm("movq (%%rbx), %%rbx;"\ */
/* :		      \ */
/* :		      \ */
/* :"%rbx"); */
#define ONE       p = (uint64_t *)*p;
#define FOUR      ONE ONE ONE ONE
#define FIVE      FOUR ONE
#define EIGHT     FOUR FOUR
#define	TEN	  FIVE FIVE
#define SIXTEEN   EIGHT EIGHT
#define THIRTYTWO SIXTEEN SIXTEEN
#define	FIFTY	  TEN TEN TEN TEN TEN
#define SIXTYFOUR THIRTYTWO THIRTYTWO
#define	HUNDRED	  FIFTY FIFTY

#define MAX_NB_NUMA_NODES     16
#define DEFAULT_MEM_SIZE 64 * 1024 * 1024

void usage(const char *prog_name) {
  printf ("Usage: %s -a <access mode> -c <core> [-m <size>] [-n <node>] [-i <nb_iter>]\n"
	  "\t -a: access mode is either seq or rand for sequential or random accesses\n"
	  "\t -c: the core where the thread loading memory is pinned\n"
	  "\t -m: memory size in kb of allocated and accessed memory\n"
	  "\t -n: the NUMA node where memory must be explicitely allocated (-1 for local allocation)\n"
	  "\t -i: the number of time the iteration reading over 100 elements is done (-1 for infinite loop)\n",
	  prog_name);
}

int main(int argc, char **argv) {

  //Get numa configuration
  unsigned int nb_numa_nodes;
  int numa_node_to_cpu[MAX_NB_NUMA_NODES];
  int available = numa_available();
  if (available == -1) {
    nb_numa_nodes = -1;
  } else {
    nb_numa_nodes = numa_num_configured_nodes();
    int nb_cpus = numa_num_configured_cpus();
    struct bitmask *mask = numa_bitmask_alloc(nb_cpus);
    for (int node = 0; node < nb_numa_nodes; node++) {
      numa_node_to_cpus(node, mask);
      numa_node_to_cpu[node] = -1;
      for (int cpu = 0; cpu < nb_cpus; cpu++) {
	if (*(mask->maskp) & (1 << cpu)) {
	  numa_node_to_cpu[node] = cpu;
	  break;
	}
      }
      if (numa_node_to_cpu[node] == -1) {
	nb_numa_nodes = -1; // to be handled properly
      }
    }
  }

  // Check and get arguments.
  size_t size_in_bytes = DEFAULT_MEM_SIZE;
  char core = -1;
  enum access_mode_t access_mode = access_undef;
  char node = -1;
  register int nb_iter = -1;
  for (int i = 1; i < argc; i+=2) {
    if (!strcmp(argv[i], "-a")) {
      if (!strcmp(argv[i+1], "seq")) {
	access_mode = access_seq;
      } else if (!strcmp(argv[i+1], "rand")) {
	access_mode = access_rand;
      } else {
	printf("Unknown access_mode %s\n", argv[i+1]);
	usage(argv[0]);
	return -1;
      }
    }
    if (!strcmp(argv[i], "-c")) {
      core = atoi(argv[i+1]);
      if (core >= get_nprocs()) {
	printf("Invalid core number %d, must be between 0 and %d\n", core, get_nprocs() - 1);
	usage(argv[0]);
	return -1;
      }
    }
    if (!strcmp(argv[i], "-m")) {
      size_in_bytes = atol(argv[i+1]);
    }
    if (!strcmp(argv[i], "-n")) {
      node = atoi(argv[i+1]);
      if (node >= nb_numa_nodes) {
	printf("Invalid numa node %d, must be between 0 and %d\n", node, nb_numa_nodes - 1);
	usage(argv[0]);
	return -1;
      }
    }
    if (!strcmp(argv[i], "-i")) {
      nb_iter = atoi(argv[i+1]);
    }
  }
  if (node == -1) {
    node = numa_node_of_cpu(core);
  }
  if (access_mode == access_undef) {
    usage(argv[0]);
    return -1;
  }
  if (core == -1) {
    usage(argv[0]);
    return -1;
  }

  fprintf(stderr, "Benchmark parameters:\n"
	  "  - access mode = %s\n"
	  "  - core = %d\n"
	  "  - memory size = %zu bytes\n"
	  "  - node = %d\n"
	  "  - iterations = %d\n",
	  access_mode == access_rand ? "rand" : "seq",
	  core,
	  size_in_bytes,
          node,
          nb_iter);

  // Allocate and fill memory
  register uint64_t *memory = numa_alloc_onnode(size_in_bytes, node);
  assert(memory);
  fill_memory(memory, size_in_bytes, access_mode);
  uint64_t *p = memory;
/* asm("movq %0, %%rbx;" */
/*     : */
/*     :"r" (p) */
/*     :"%rbx"); */

  /**
   * Pin process on core CPU
   */
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(core, &mask);
  if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
    printf("sched_setaffinity failed: %s\n", strerror(errno));
    return -1;
  }

  if (nb_iter == -1) {
    // Infinite loop
    while (1) {
      SIXTYFOUR
    }
  } else {
    struct timespec start, end;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    int register i = 0;
    while (i < nb_iter) {
      i++;
      SIXTYFOUR
    }
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
    uint64_t ns = (end.tv_sec * 1E9 + end.tv_nsec) - (start.tv_sec * 1E9 + start.tv_nsec);
    uint64_t latency = (uint64_t)((float)ns / (nb_iter * 64));
    fprintf(stderr, "time = %" PRIu64 " ns\n", ns);
    fprintf(stderr, "average latency = %" PRIu64 " ns (%" PRIu64 " cycles for frequency = 2.668 GHz)\n",
	    latency,
	    (uint64_t)(latency * 2.668));
  }

  return 0;
}
