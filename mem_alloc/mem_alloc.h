#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H

#include <stdlib.h>
#include <inttypes.h>

#define KiB 1024
#define MiB KiB*1024
#define GiB MiB*1024

enum access_mode_t {
  access_undef,
  access_seq,
  access_rand
};

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
void fill_memory(uint64_t *memory, size_t size, enum access_mode_t access_mode);

/**
 * Same as above, but with additional npad parameter allowing to
 * specify the size of each element in the filled memory
 * region. Usefull to play with when one wants to understand
 * prefecthing effects.
 */
void fill_memory_npad(uint64_t *memory, size_t size, enum access_mode_t access_mode, unsigned char npad);

#endif
