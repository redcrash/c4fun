#define _GNU_SOURCE

#include <math.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <time.h> // For clock_gettime()
#include <numa.h>
#include <numaif.h>
#include <sys/ioctl.h> // For ioctl
#include <sys/sysinfo.h> // For get_nprocs()
#include <sys/mman.h> // For madvise
#include <sched.h> // For sched_setaffinity
#include <linux/perf_event.h> // For perf_event_open
#include <unistd.h> // For syscall
#include <sys/syscall.h> // For syscall

#include "mem_alloc.h"

#define ONE      asm("movq (%%rbx), %%rbx;"	\
		     :				\
		     :				\
		     :"%rbx");
//#define ONE       p = (uint64_t *)*p;
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

#define PROTECTION (PROT_READ | PROT_WRITE)
#define FLAGS (MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB)

static long perf_event_open(struct perf_event_attr *hw_event,
			    pid_t pid,
			    int cpu,
			    int group_fd,
			    unsigned long flags) {
  int ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
		    group_fd, flags);
  return ret;
}

static size_t get_hugepage_size() {
  const char *key = "Hugepagesize:";

  FILE *f = fopen("/proc/meminfo", "r");
  assert(f);

  char *linep = NULL;
  size_t n = 0;
  size_t size = 0;
  while (getline(&linep, &n, f) > 0) {
    if (strstr(linep, key) != linep)
      continue;
    size = atol(linep + strlen(key)) * 1024;
    break;
  }
  fclose(f);
  assert(size);
  return size;
}

static unsigned char check_THP_never() {

  const char key = '[';

  FILE *f = fopen("/sys/kernel/mm/transparent_hugepage/enabled", "r");
  assert(f);

  char *linep = NULL;
  size_t n = 0;
  getline(&linep, &n, f);
  int i = 0;
  int spaces = 0;
  char current = linep[0];
  while (current != key) {
    if (current == ' ') {
      spaces++;
    }
    i++;
    current = linep[i];
  }
  fclose(f);
  return spaces == 2;
}

void usage(const char *prog_name) {
  printf ("Usage: %s -a <access mode> -c <core> [-m <size>] [-n <node>] [-i <nb_iter>] [-r <nb_run>] [-s]\n"
	  "\t -a: access mode is either seq or rand for sequential or random accesses\n"
	  "\t -c: the core where the thread loading memory is pinned\n"
	  "\t -m: memory size in bytes of allocated and accessed memory\n"
	  "\t -n: the NUMA node where memory must be explicitely allocated (-1 for local allocation)\n"
	  "\t -i: the number of time the iteration reading over 64 elements is done (-1 for infinite loop)\n"
	  "\t -r: the number of time we repeat the bench to compute average and standard deviation (default is 1)\n"
	  "\t -s: to remove the usage of huge pages\n",
	  prog_name);
}

int main(int argc, char **argv) {

  // Assert THP is disabled
  if (!check_THP_never()) {
    fprintf(stderr, "THP must be disabled to run this bench:\n  write \"never\" to /sys/kernel/mm/transparent_hugepage/enabled\n");
    return  -1;
  }

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
  unsigned char huge_pages = 1;
  register int nb_iter = -1;
  unsigned int nb_runs = 1;
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
    if (!strcmp(argv[i], "-r")) {
      nb_runs = atoi(argv[i+1]);
    }
    if (!strcmp(argv[i], "-s")) {
      huge_pages = 0;
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
	  "  - iterations = %d\n"
	  "  - runs = %u\n"
	  "  - huge pages (%" PRIu64 " Kb) = %s\n",
	  access_mode == access_rand ? "rand" : "seq",
	  core,
	  size_in_bytes,
          node,
          nb_iter,
	  nb_runs,
	  get_hugepage_size() / 1024,
          huge_pages == 1 ? "yes" : "no");

  // Allocate and fill memory
  fprintf(stderr, "Allocating and filling memory ... ");
  long unsigned int nodes = 0;
  nodes += 1 << node;
  assert(set_mempolicy(MPOL_BIND, &nodes, 8*sizeof(long unsigned int)) == 0);
  uint64_t *memory;
  if (huge_pages) {

    /* assert(posix_memalign((void**)&memory, get_hugepage_size(), size_in_bytes) == 0); */
    /* if(madvise(memory, size_in_bytes, MADV_HUGEPAGE)) { */
    /*   fprintf(stderr, "Cannot use large pages.\n"); */
    /* } */

    memory = mmap(0, size_in_bytes, PROTECTION, FLAGS, 0, 0);
    if (memory == MAP_FAILED) {
      perror("mmap");
      exit(1);
    }


    /* int fd = open("/mnt/huge/hugepagefile", O_CREAT | O_RDWR, 0755); */
    /* if (fd < 0) { */
    /*   perror("Open failed"); */
    /*   exit(1); */
    /* } */
    /* memory = mmap(0, size_in_bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); */
    /* if (memory == MAP_FAILED) { */
    /*   perror("mmap"); */
    /*   exit(1); */
    /* } */
  } else {
    assert(posix_memalign((void**)&memory, get_hugepage_size(), size_in_bytes) == 0);
    /* if(madvise(memory, size_in_bytes, MADV_NOHUGEPAGE)) { */
    /*   fprintf(stderr, "Cannot use large pages.\n"); */
    /* } */
  }
  fill_memory(memory, size_in_bytes, access_mode);
  fprintf(stderr, "done\n");

  //sleep(50);

  register uint64_t *p = memory;
  asm("movq %0, %%rbx;"
      :
      :"r" (p)
      :"%rbx");

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

  /**
   * Stuff for counting the number of DTLB misses
   */
  struct perf_event_attr pe_attr_cache;
  memset(&pe_attr_cache, 0, sizeof(pe_attr_cache));
  pe_attr_cache.size = sizeof(pe_attr_cache);
  pe_attr_cache.type = PERF_TYPE_HW_CACHE;
  pe_attr_cache.config = PERF_COUNT_HW_CACHE_DTLB | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
  pe_attr_cache.disabled = 1;
  pe_attr_cache.exclude_kernel = 1;
  pe_attr_cache.exclude_hv = 1;
  pe_attr_cache.exclude_idle = 1;
  int dtlb_misses_fd = perf_event_open(&pe_attr_cache, 0, core, -1, 0);
  if (dtlb_misses_fd == -1) {
    fprintf(stderr, "perf_event_open failed for dtlb misses: %s\n", strerror(errno));
    return -1;
  }

  /**
   * Stuff for counting the number of cache misses 2
   */
  struct perf_event_attr pe_attr_cache2;
  memset(&pe_attr_cache2, 0, sizeof(pe_attr_cache2));
  pe_attr_cache2.size = sizeof(pe_attr_cache2);
  pe_attr_cache2.type = PERF_TYPE_HARDWARE;
  pe_attr_cache2.config = PERF_COUNT_HW_CACHE_MISSES;
  pe_attr_cache2.disabled = 1;
  pe_attr_cache2.exclude_kernel = 1;
  pe_attr_cache2.exclude_hv = 1;
  pe_attr_cache2.exclude_idle = 1;
  int cache_misses_fd = perf_event_open(&pe_attr_cache2, 0, core, -1, 0);
  if (cache_misses_fd == -1) {
    fprintf(stderr, "perf_event_open failed for cache misses: %s\n", strerror(errno));
    return -1;
  }

  /**
   * Loop over memory either infinite or not
   */
  if (nb_iter == -1) {
    // Infinite loop
    while (1)
      SIXTYFOUR
	}


  uint64_t *times = malloc(nb_runs * sizeof(uint64_t));
  float *latencies = malloc(nb_runs * sizeof(float));
  uint64_t *dtlb_misses = malloc(nb_runs * sizeof(uint64_t));
  uint64_t *cache_misses = malloc(nb_runs * sizeof(uint64_t));
  assert(times);
  assert(latencies);
  assert(dtlb_misses);
  assert(cache_misses);

  for (int run = 0; run < nb_runs; run++) {

    fprintf(stderr, "\rRun %d", run + 1);

    /**
     * Loop the specified number of time
     */
    struct timespec start, end;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

    ioctl(cache_misses_fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(dtlb_misses_fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(cache_misses_fd, PERF_EVENT_IOC_ENABLE, 0);
    ioctl(dtlb_misses_fd, PERF_EVENT_IOC_ENABLE, 0);

    int register i = 0;
    while (i < nb_iter) {
      i++;
      SIXTYFOUR
	}

    ioctl(dtlb_misses_fd, PERF_EVENT_IOC_DISABLE, 0);
    ioctl(cache_misses_fd, PERF_EVENT_IOC_DISABLE, 0);

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
    times[run] = (end.tv_sec * 1E9 + end.tv_nsec) - (start.tv_sec * 1E9 + start.tv_nsec);
    latencies[run] = times[run] / (nb_iter * 64.0);
    read(dtlb_misses_fd, &(dtlb_misses[run]), sizeof(dtlb_misses[run]));
    read(cache_misses_fd, &(cache_misses[run]), sizeof(cache_misses[run]));
  }

  uint64_t time_sum = 0;
  float latency_sum = 0;
  uint64_t dtlb_misses_sum = 0;
  uint64_t cache_misses_sum = 0;
  for (int i = 0; i < nb_runs; i++) {
    time_sum += times[i];
    latency_sum += latencies[i];
    dtlb_misses_sum += dtlb_misses[i];
    cache_misses_sum += cache_misses[i];
  }
  float time_avg = time_sum / (float)nb_runs;
  float latency_avg = latency_sum / (float)nb_runs;
  float dtlb_misses_avg = dtlb_misses_sum / (float)nb_runs;
  float cache_misses_avg = cache_misses_sum / (float)nb_runs;

  uint64_t time_deviation_sum = 0;
  float latency_deviation_sum = 0;
  uint64_t dtlb_misses_deviation_sum = 0;
  uint64_t cache_misses_deviation_sum = 0;
  for (int i = 0; i < nb_runs; i++) {
    time_deviation_sum += (times[i] - time_avg) * (times[i] - time_avg);
    latency_deviation_sum += (latencies[i] - latency_avg) * (latencies[i] - latency_avg);
    dtlb_misses_deviation_sum += (dtlb_misses[i] - dtlb_misses_avg) * (dtlb_misses[i] - dtlb_misses_avg);
    cache_misses_deviation_sum += (cache_misses[i] - cache_misses_avg) * (cache_misses[i] - cache_misses_avg);
  }
  float time_deviation = sqrt(time_deviation_sum / (float)nb_runs);
  float latency_deviation = sqrt(latency_deviation_sum / (float)nb_runs);
  float dtlb_misses_deviation = sqrt(dtlb_misses_deviation_sum / (float)nb_runs);
  float cache_misses_deviation = sqrt(cache_misses_deviation_sum / (float)nb_runs);

  fprintf(stderr, "\nBench time:                   average = %.3f ms, standard deviation = %.3f%%\n", time_avg / 1E6, (time_deviation / time_avg) * 100);
  fprintf(stderr, "Single memory access latency: average = %.3f ns, standard deviation = %.3f%%\n", latency_avg, (latency_deviation / latency_avg) * 100);
  fprintf(stderr, "Data tlb misses %% (among all reads): average = %.3f, standard deviation = %.3f%%\n", (dtlb_misses_avg * 100) / (64.0 * nb_iter), (dtlb_misses_deviation / dtlb_misses_avg) * 100);
  fprintf(stderr, "Cache misses %% (among all reads)   : average = %.3f, standard deviation = %.3f%%\n", (cache_misses_avg * 100) / (64.0 * nb_iter), (cache_misses_deviation / cache_misses_avg) * 100);

  free(times);
  free(latencies);
  free(dtlb_misses);
  free(cache_misses);

  return 0;
}
