#include <inttypes.h>

#ifndef __RAND_H__
#define __RAND_H__

uint64_t rand_state = 0xf0001010e077f3f3;

static __inline __attribute__((always_inline))
uint64_t xorshift64() {
  uint64_t x = rand_state;
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  return rand_state = x;
}

#endif
