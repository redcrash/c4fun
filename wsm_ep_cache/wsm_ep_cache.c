#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <numa.h>
#include <numaif.h>
#include <assert.h>
#include <sys/time.h>
#include <errno.h>
#include <err.h>
#include <sys/mman.h>
#include <pthread.h>
#include <fcntl.h>
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>

// For thread affinity
#include <sched.h>

#include "mem_alloc.h"

/* Used to control what we count */
#define CORE_COUNT_DTLB_LOAD_MISSES_ANY
/* #define CORE_COUNT_LOADS */
/* #define CORE_COUNT_INST */
/* #define CORE_OFFCORE_COUNT_UNCORE_HIT */
/* #define CORE_OFFCORE_COUNT_OTHER_CORE_HIT_SNP */
/* #define CORE_OFFCORE_COUNT_OTHER_CORE_HITM */
#define CORE_OFFCORE_COUNT_REMOTE_CACHE_FWD
/* #define CORE_OFFCORE_COUNT_REMOTE_DRAM */
/* #define CORE_OFFCORE_COUNT_LOCAL_DRAM */

#define ELEM_TYPE uint64_t

#define READ_CPU 8
#define WRITE_CPU 8

#define ONE addr = (uint64_t *)*addr;
#define FOUR ONE ONE ONE ONE
#define SIXTEEN FOUR FOUR FOUR FOUR
#define THIRTY_TWO SIXTEEN SIXTEEN
#define SIXTY_FOUR THIRTY_TWO THIRTY_TWO

static long perf_event_open(struct perf_event_attr *hw_event,
			    pid_t pid,
			    int cpu,
			    int group_fd,
			    unsigned long flags) {
  int ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
		    group_fd, flags);
  return ret;
}

uint64_t *memory;
size_t size_in_bytes = 64*MiB;

void *fill_mem(void * p) {

  /**
   * Allocates and fills memory. Because the memory is filled, all its
   * pages are touched and as a consequence, no page faults will occur
   * during the measurement. Using mmap and MAP_HUGETLB we can
   * allocate huge page of 1 GiB and thus avoid memory accesses caused
   * by TLB misses. Because we have only 1 1GB huge page in the system
   * and because by default Linux allocates huge pages in a round
   * robin fashion on NUMA systems, the first node is always used in
   * our case.
   */
  //memory = malloc(size_in_bytes);
  memory = mmap(NULL, size_in_bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, 0, 0);
  if (memory == MAP_FAILED) {
    fprintf(stderr, "mmap failed: %s\n", strerror(errno));
    return (void *) -1;
  }
  memset(memory, -1, size_in_bytes);
  fill_memory(memory, size_in_bytes, access_rand);

  fprintf(stdout, "Memory filled successfully\n");
  return memory;
}

/**
 * Read the given memory region. Several accesses are done in each
 * loop iteration to limit the number of accesses caused by the loop
 * test.
 */
void *read_memory(void *p) {

  int64_t nb_elems = size_in_bytes / sizeof(ELEM_TYPE);
  printf("Nb 64 bits loads from memory = %" PRId64 "\n", nb_elems);

#ifdef CORE_COUNT_DTLB_LOAD_MISSES_ANY
  struct perf_event_attr pe_attr_tlb_misses;
  memset(&pe_attr_tlb_misses, 0, sizeof(pe_attr_tlb_misses));
  pe_attr_tlb_misses.size = sizeof(pe_attr_tlb_misses);
  pe_attr_tlb_misses.type = PERF_TYPE_RAW;
  pe_attr_tlb_misses.config = 0x0149; // DTLB_LOAD_MISSES.ANY
  pe_attr_tlb_misses.disabled = 1;
  pe_attr_tlb_misses.exclude_kernel = 1;
  pe_attr_tlb_misses.exclude_hv = 1;
  int tlb_misses_fd = perf_event_open(&pe_attr_tlb_misses, 0, READ_CPU, -1, 0);
  if (tlb_misses_fd == -1) {
    fprintf(stderr, "perf_event_open failed for tlb misses: %s\n", strerror(errno));
    return (void *) -1;
  }
#endif

#ifdef CORE_COUNT_INST
  struct perf_event_attr pe_attr_inst;
  memset(&pe_attr_inst, 0, sizeof(pe_attr_inst));
  pe_attr_inst.size = sizeof(pe_attr_inst);
  pe_attr_inst.type = PERF_TYPE_RAW;
  pe_attr_inst.config = 0x00c0; // INST_RETIRED.ANY
  pe_attr_inst.disabled = 1;
  pe_attr_inst.exclude_kernel = 1;
  pe_attr_inst.exclude_hv = 1;
  int inst_fd = perf_event_open(&pe_attr_inst, 0, READ_CPU, -1, 0);
  if (inst_fd == -1) {
    fprintf(stderr, "perf_event_open failed for instructions: %s\n", strerror(errno));
    return (void *) -1;
  }
#endif

#ifdef CORE_COUNT_LOADS
  struct perf_event_attr pe_attr_loads;
  memset(&pe_attr_loads, 0, sizeof(pe_attr_loads));
  pe_attr_loads.size = sizeof(pe_attr_loads);
  pe_attr_loads.type = PERF_TYPE_RAW;
  pe_attr_loads.config = 0x010b; // MEM_INST_RETIRED.LOADS
  pe_attr_loads.disabled = 1;
  pe_attr_loads.exclude_kernel = 1;
  pe_attr_loads.exclude_hv = 1;
  int loads_fd = perf_event_open(&pe_attr_loads, 0, READ_CPU, -1, 0);
  if (loads_fd == -1) {
    fprintf(stderr, "perf_event_open failed for core loads: %s\n", strerror(errno));
    return (void *) -1;
  }
#endif

#ifdef CORE_OFFCORE_COUNT_UNCORE_HIT
  struct perf_event_attr pe_attr_uncore_hit;
  memset(&pe_attr_uncore_hit, 0, sizeof(pe_attr_uncore_hit));
  pe_attr_uncore_hit.size = sizeof(pe_attr_uncore_hit);
  pe_attr_uncore_hit.type = PERF_TYPE_RAW;
  pe_attr_uncore_hit.config = 0x5301b7; // OFF_CORE_RESPONSE_0
  pe_attr_uncore_hit.config1 = 0x133; // UNCORE_HIT
  pe_attr_uncore_hit.disabled = 1;
  pe_attr_uncore_hit.exclude_kernel = 1;
  pe_attr_uncore_hit.exclude_hv = 1;
  int uncore_hit_fd = perf_event_open(&pe_attr_uncore_hit, 0, READ_CPU, -1, 0);
  if (uncore_hit_fd == -1) {
    printf("perf_event_open failed for instructions: %s\n", strerror(errno));
    return (void *) -1;
  }
#endif

#ifdef CORE_OFFCORE_COUNT_OTHER_CORE_HIT_SNP
  struct perf_event_attr pe_attr_other_core_hit_snp;
  memset(&pe_attr_other_core_hit_snp, 0, sizeof(pe_attr_other_core_hit_snp));
  pe_attr_other_core_hit_snp.size = sizeof(pe_attr_other_core_hit_snp);
  pe_attr_other_core_hit_snp.type = PERF_TYPE_RAW;
  pe_attr_other_core_hit_snp.config = 0x5301b7; // OFF_CORE_RESPONSE_0
  pe_attr_other_core_hit_snp.config1 = 0x233; // OTHER_CORE_HIT_SNP
  pe_attr_other_core_hit_snp.disabled = 1;
  pe_attr_other_core_hit_snp.exclude_kernel = 1;
  pe_attr_other_core_hit_snp.exclude_hv = 1;
  int other_core_hit_snp_fd = perf_event_open(&pe_attr_other_core_hit_snp, 0, READ_CPU, -1, 0);
  if (other_core_hit_snp_fd == -1) {
    printf("perf_event_open failed for other core hit snoop: %s\n", strerror(errno));
    return (void *) -1;
  }
#endif

#ifdef CORE_OFFCORE_COUNT_OTHER_CORE_HITM
  struct perf_event_attr pe_attr_other_core_hitm;
  memset(&pe_attr_other_core_hitm, 0, sizeof(pe_attr_other_core_hitm));
  pe_attr_other_core_hitm.size = sizeof(pe_attr_other_core_hitm);
  pe_attr_other_core_hitm.type = PERF_TYPE_RAW;
  pe_attr_other_core_hitm.config = 0x5301b7; // OFF_CORE_RESPONSE_0
  pe_attr_other_core_hitm.config1 = 0x433; // OTHER_CORE_HITM
  pe_attr_other_core_hitm.disabled = 1;
  pe_attr_other_core_hitm.exclude_kernel = 1;
  pe_attr_other_core_hitm.exclude_hv = 1;
  int other_core_hitm_fd = perf_event_open(&pe_attr_other_core_hitm, 0, READ_CPU, -1, 0);
  if (other_core_hitm_fd == -1) {
    printf("perf_event_open failed for other core hitm: %s\n", strerror(errno));
    return (void *) -1;
  }
#endif

#ifdef CORE_OFFCORE_COUNT_REMOTE_CACHE_FWD
  struct perf_event_attr pe_attr_remote_cache_fwd;
  memset(&pe_attr_remote_cache_fwd, 0, sizeof(pe_attr_remote_cache_fwd));
  pe_attr_remote_cache_fwd.size = sizeof(pe_attr_remote_cache_fwd);
  pe_attr_remote_cache_fwd.type = PERF_TYPE_RAW;
  pe_attr_remote_cache_fwd.config = 0x5301b7; // OFF_CORE_RESPONSE_0
  pe_attr_remote_cache_fwd.config1 = 0x1033; // REMOTE_CACHE_FWD
  pe_attr_remote_cache_fwd.disabled = 1;
  pe_attr_remote_cache_fwd.exclude_kernel = 1;
  pe_attr_remote_cache_fwd.exclude_hv = 1;
  int remote_cache_fwd_fd = perf_event_open(&pe_attr_remote_cache_fwd, 0, READ_CPU, -1, 0);
  if (remote_cache_fwd_fd == -1) {
    printf("perf_event_open failed for remote cache forward: %s\n", strerror(errno));
    return (void *) -1;
  }
#endif

#ifdef CORE_OFFCORE_COUNT_REMOTE_DRAM
  struct perf_event_attr pe_attr_remote_ram;
  memset(&pe_attr_remote_ram, 0, sizeof(pe_attr_remote_ram));
  pe_attr_remote_ram.size = sizeof(pe_attr_remote_ram);
  pe_attr_remote_ram.type = PERF_TYPE_RAW;
  pe_attr_remote_ram.config = 0x5301bb; // OFF_CORE_RESPONSE_1
  pe_attr_remote_ram.config1 = 0x2033; // REMOTE_DRAM
  pe_attr_remote_ram.disabled = 1;
  pe_attr_remote_ram.exclude_kernel = 1;
  pe_attr_remote_ram.exclude_hv = 1;
  int remote_ram_fd = perf_event_open(&pe_attr_remote_ram, 0, READ_CPU, -1, 0);
  if (remote_ram_fd == -1) {
    printf("perf_event_open failed for remote dram: %s\n", strerror(errno));
    return (void *)-1;
  }
#endif

#ifdef CORE_OFFCORE_COUNT_LOCAL_DRAM
  struct perf_event_attr pe_attr_local_ram;
  memset(&pe_attr_local_ram, 0, sizeof(pe_attr_local_ram));
  pe_attr_local_ram.size = sizeof(pe_attr_local_ram);
  pe_attr_local_ram.type = PERF_TYPE_RAW;
  pe_attr_local_ram.config = 0x5301bb; // OFF_CORE_RESPONSE_1
  pe_attr_local_ram.config1 = 0x4033; // LOCAL_DRAM
  pe_attr_local_ram.disabled = 1;
  pe_attr_local_ram.exclude_kernel = 1;
  pe_attr_local_ram.exclude_hv = 1;
  int local_ram_fd = perf_event_open(&pe_attr_local_ram, 0, READ_CPU, -1, 0);
  if (local_ram_fd == -1) {
    printf("perf_event_open failed for local dram: %s\n", strerror(errno));
    return (void *)-1;
  }
#endif

#ifdef CORE_OFFCORE_COUNT_UNCORE_HIT
  ioctl(uncore_hit_fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(uncore_hit_fd, PERF_EVENT_IOC_ENABLE, 0);
#endif
#ifdef CORE_OFFCORE_COUNT_OTHER_CORE_HIT_SNP
  ioctl(other_core_hit_snp_fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(other_core_hit_snp_fd, PERF_EVENT_IOC_ENABLE, 0);
#endif
#ifdef CORE_OFFCORE_COUNT_OTHER_CORE_HITM
  ioctl(other_core_hitm_fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(other_core_hitm_fd, PERF_EVENT_IOC_ENABLE, 0);
#endif
#ifdef CORE_OFFCORE_COUNT_REMOTE_CACHE_FWD
  ioctl(remote_cache_fwd_fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(remote_cache_fwd_fd, PERF_EVENT_IOC_ENABLE, 0);
#endif
#ifdef CORE_OFFCORE_COUNT_REMOTE_DRAM
  ioctl(remote_ram_fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(remote_ram_fd, PERF_EVENT_IOC_ENABLE, 0);
#endif
#ifdef CORE_OFFCORE_COUNT_LOCAL_DRAM
  ioctl(local_ram_fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(local_ram_fd, PERF_EVENT_IOC_ENABLE, 0);
#endif
#ifdef CORE_COUNT_INST
  ioctl(inst_fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(inst_fd, PERF_EVENT_IOC_ENABLE, 0);
#endif
#ifdef CORE_COUNT_LOADS
  ioctl(loads_fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(loads_fd, PERF_EVENT_IOC_ENABLE, 0);
#endif
#ifdef CORE_COUNT_DTLB_LOAD_MISSES_ANY
  ioctl(tlb_misses_fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(tlb_misses_fd, PERF_EVENT_IOC_ENABLE, 0);
#endif


  // register allocation to avoid having memory loads generated by
  // stack accesses. If addr is in the stack, the line addr =
  // (uint64_t *)*addr is compiled (in pseudo assembler) to r1 =
  // mem[addr]; r2 = mem[r1]; mem[addr] = r2; When doing the register
  // allocation, the code is compiled to: r2 = mem[r1]; r1 = r2; and
  // we see that in that case we only have one memory load.
  register uint64_t *addr = memory;

  // Also register allocation for the same reason
  register int64_t nb_elem_remaining = nb_elems;

  while (nb_elem_remaining > 0) {
    THIRTY_TWO
      nb_elem_remaining -= 32;
  }

#ifdef CORE_COUNT_DTLB_LOAD_MISSES_ANY
  ioctl(tlb_misses_fd, PERF_EVENT_IOC_DISABLE, 0);
#endif
#ifdef CORE_COUNT_LOADS
  ioctl(loads_fd, PERF_EVENT_IOC_DISABLE, 0);
#endif
#ifdef CORE_COUNT_INST
  ioctl(inst_fd, PERF_EVENT_IOC_DISABLE, 0);
#endif
#ifdef CORE_OFFCORE_COUNT_UNCORE_HIT
  ioctl(uncore_hit_fd, PERF_EVENT_IOC_DISABLE, 0);
#endif
#ifdef CORE_OFFCORE_COUNT_OTHER_CORE_HIT_SNP
  ioctl(other_core_hit_snp_fd, PERF_EVENT_IOC_DISABLE, 0);
#endif
#ifdef CORE_OFFCORE_COUNT_OTHER_CORE_HITM
  ioctl(other_core_hitm_fd, PERF_EVENT_IOC_DISABLE, 0);
#endif
#ifdef CORE_OFFCORE_COUNT_REMOTE_CACHE_FWD
  ioctl(remote_cache_fwd_fd, PERF_EVENT_IOC_DISABLE, 0);
#endif
#ifdef CORE_OFFCORE_COUNT_REMOTE_DRAM
  ioctl(remote_ram_fd, PERF_EVENT_IOC_DISABLE, 0);
#endif
#ifdef CORE_OFFCORE_COUNT_LOCAL_DRAM
  ioctl(local_ram_fd, PERF_EVENT_IOC_DISABLE, 0);
#endif

#ifdef CORE_COUNT_DTLB_LOAD_MISSES_ANY
  uint64_t tlb_count;
  read(tlb_misses_fd, &tlb_count, sizeof(tlb_count));
#endif
#ifdef CORE_COUNT_INST
  uint64_t insts_count;
  read(inst_fd, &insts_count, sizeof(insts_count));
#endif
#ifdef CORE_COUNT_LOADS
  uint64_t loads_count;
  read(loads_fd, &loads_count, sizeof(loads_count));
#endif
#ifdef CORE_OFFCORE_COUNT_UNCORE_HIT
  uint64_t uncore_hit_count;
  read(uncore_hit_fd, &uncore_hit_count, sizeof(uncore_hit_count));
#endif
#ifdef CORE_OFFCORE_COUNT_OTHER_CORE_HIT_SNP
  uint64_t other_core_hit_snp_count;
  read(other_core_hit_snp_fd, &other_core_hit_snp_count, sizeof(other_core_hit_snp_count));
#endif
#ifdef CORE_OFFCORE_COUNT_OTHER_CORE_HITM
  uint64_t other_core_hitm_count;
  read(other_core_hitm_fd, &other_core_hitm_count, sizeof(other_core_hitm_count));
#endif
#ifdef CORE_OFFCORE_COUNT_REMOTE_CACHE_FWD
  uint64_t remote_cache_fwd_count;
  read(remote_cache_fwd_fd, &remote_cache_fwd_count, sizeof(remote_cache_fwd_count));
#endif
#ifdef CORE_OFFCORE_COUNT_REMOTE_DRAM
  uint64_t remote_ram_count;
  read(remote_ram_fd, &remote_ram_count, sizeof(remote_ram_count));
#endif
#ifdef CORE_OFFCORE_COUNT_LOCAL_DRAM
  uint64_t local_ram_count;
  read(local_ram_fd, &local_ram_count, sizeof(local_ram_count));
#endif

#ifdef CORE_COUNT_DTLB_LOAD_MISSES_ANY
  printf("%-80s = %15" PRId64 "\n", "tlb misses count (core event: DTLB_LOAD_MISSES.ANY)", tlb_count);
#endif
#ifdef CORE_COUNT_INST
  printf("%-80s = %15" PRId64 "\n", "instructions count (core event: INST_RETIRED.ANY)", insts_count);
#endif
#ifdef CORE_COUNT_LOADS
  printf("%-80s = %15" PRId64 " (expected = %" PRId64 ")\n", "loads count (core event: MEM_INST_RETIRED.LOADS)", loads_count, nb_elems);
#endif
#ifdef CORE_OFFCORE_COUNT_UNCORE_HIT
  printf("%-80s = %15" PRId64 "\n", "uncore hit (core event: OFF_CORE_RESPONSE_0:UNCORE_HIT)", uncore_hit_count);
#endif
#ifdef CORE_OFFCORE_COUNT_OTHER_CORE_HIT_SNP
  printf("%-80s = %15" PRId64 "\n", "other core hit snoop (core event: OFF_CORE_RESPONSE_0:OTHER_CORE_HIT_SNP)", other_core_hit_snp_count);
#endif
#ifdef CORE_OFFCORE_COUNT_OTHER_CORE_HITM
  printf("%-80s = %15" PRId64 "\n", "other core hitm count (core event: OFF_CORE_RESPONSE_0:OTHER_CORE_HITM)", other_core_hitm_count);
#endif
#ifdef CORE_OFFCORE_COUNT_REMOTE_CACHE_FWD
  printf("%-80s = %15" PRId64 "\n", "remote cache forward count (core event: OFF_CORE_RESPONSE_0:REMOTE_CACHE_FWD)", remote_cache_fwd_count);
#endif
#ifdef CORE_OFFCORE_COUNT_REMOTE_DRAM
  printf("%-80s = %15" PRId64 "\n", "remote memory count (core event: OFF_CORE_RESPONSE_1:REMOTE_DRAM)", remote_ram_count);
#endif
#ifdef CORE_OFFCORE_COUNT_LOCAL_DRAM
  printf("%-80s = %15" PRId64 "\n", "local memory count (core event: OFF_CORE_RESPONSE_1:LOCAL_DRAM)", local_ram_count);
#endif

  return memory;
}

int main(int argc, char **argv) {

  /**
   * Fill the memory in a thread on a specific core.
   */
  int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
  if (WRITE_CPU >= num_cores || READ_CPU >= num_cores) {
    fprintf(stderr, "Can't set affinity to write thread to %d or to read thread to %d\n", WRITE_CPU, READ_CPU);
    return -1;
  }
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(WRITE_CPU, &mask);
  int res = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
  if (res != 0) {
    fprintf(stderr, "Error setting affinity to write thread: %s\n", strerror(res));
    return -1;
  }
  pthread_t thread;
  if ((res = pthread_create(&thread, &attr, fill_mem, (void *)NULL)) < 0) {
    fprintf(stderr, "Error creating thread for core 0: %d\n", res);
  }
  void *status;
  if ((res = pthread_join(thread, &status)) < 0) {
    fprintf(stderr, "Error joining fill memory thread: %d\n", res);
    return -1;
  }
  if ((long)status == -1) {
    return -1;
  }

  /**
   * Read the memory in a thread on a specific core.
   */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  CPU_ZERO(&mask);
  CPU_SET(READ_CPU, &mask);
  res = pthread_attr_setaffinity_np(&attr, sizeof(mask), &mask);
  if (res != 0) {
    printf("Error setting affinity to read thread: %d\n", res);
    return -1;
  }
  if ((res = pthread_create(&thread, &attr, read_memory, (void *)NULL)) < 0) {
    printf("Error creating thread for core 0: %d\n", res);
    return -1;
  }
  if ((res = pthread_join(thread, &status)) < 0) {
    printf("Error joining read memory thread: %d\n", res);
    return -1;
  }

  return 0;
}