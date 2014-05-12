#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mem_alloc.h"

/**
 * Fills the pointer's array of the given size with each element points
 * to the next element.
 */
static void fill_memory_seq_npad(uint64_t *memory, size_t size, unsigned char npad) {

  size_t pointer_size = sizeof(char *);
  if (size % pointer_size != 0) {
    fprintf(stderr, "size = %zu must be a multiple of pointer_size = %zu\n", size, pointer_size);
    exit(-1);
  }
  size_t nb_elems = size / pointer_size;
  int i;
  for(i = 0; i < nb_elems - 1; i++) {
    memory[i] = (uint64_t)&memory[i+1];
  }
  memory[i] = (uint64_t)&memory[0];
}

/**
 * Structure used along with the following compar function to shuffle
 * an array of N elements.
 */
struct rand_struct {
  int index;
  int rand;
};

static int compar(const void* a1, const void* a2) {
  struct rand_struct *a = (struct rand_struct*) a1;
  struct rand_struct *b = (struct rand_struct*) a2;
  return a->rand - b->rand;
}

/**
 * Fills the pointer's array of the given size with each element
 * pointing to another pseudo-random element in the array. All the
 * elements of the array are pointed to exactly by one other
 * element. The last element always points to the first one.
 */
static void fill_memory_rand_npad(uint64_t *memory, size_t size, unsigned char npad) {

  size_t pointer_size = sizeof(void *);
  if (size % pointer_size != 0) {
    fprintf(stderr, "size = %zu must be a multiple of pointer_size = %zu\n", size, pointer_size);
    exit(0);
  }
  size_t nb_elems = size / pointer_size;

  /**
   * Creates another array and shuffle it.
   */
  size_t rand_memory_size = nb_elems * sizeof(struct rand_struct);
  struct rand_struct *rand_memory = malloc(rand_memory_size);
  memset(rand_memory, 0, rand_memory_size);
  assert(rand_memory);
  unsigned int seed = 1;
  for (int i = 0; i < nb_elems; i++) {
    rand_memory[i].index = i;
    rand_memory[i].rand = rand_r(&seed);
  }
  qsort(&rand_memory[1], nb_elems - 1, sizeof(*rand_memory), compar);

  /**
   * Fills the array with pointers to a next random element using the
   * shuffled memory array.
   */
  int i;
  for(i = 0; i < nb_elems - 1; i++) {
    memory[i] = (uint64_t)&memory[rand_memory[i + 1].index];
  }
  memory[i] = (uint64_t)&memory[0];
  free(rand_memory);
}

/**
 * Fills the given memory region either sequentially or pseudo
 * randomly.
 *
 * If sequential, fills the pointer's array of the given size with
 * each element points to the next element.
 *
 * If random, fills the pointer's array of the given size with each
 * element pointing to another pseudo-random element in the array. All
 * the elements of the array are pointed to exactly by one other
 * element. The last element always points to the first one.
 *
 * Calling this function is the same as calling fill_memory_npad with
 * npad=0.
 */
void fill_memory(uint64_t *memory, size_t size, enum access_mode_t access_mode) {
  fill_memory_npad(memory, size, access_mode, 0);
}

/**
 * Same as above, but with additional npad parameter allowing to
 * specify the size of each element in the filled memory
 * region. Usefull to play with when one wants to understand
 * prefecthing effects.
 */
void fill_memory_npad(uint64_t *memory, size_t size, enum access_mode_t access_mode, unsigned char npad) {

  switch (access_mode) {
  case access_seq:
    fill_memory_seq_npad(memory, size, npad);
    break;
  case access_rand:
    fill_memory_rand_npad(memory, size, npad);
    break;
  default:
    assert(NULL);
  }
}
