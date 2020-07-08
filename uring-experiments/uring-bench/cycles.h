#ifndef __CYCLES_H__
#define __CYCLES_H__

#include <assert.h>

typedef uint64_t ts;

ts cycles_per_second;

bool cycles_init();

static __inline __attribute__((always_inline))
ts rdtsc() {
  uint32_t lo, hi;
  __asm__ __volatile__("rdtsc" : "=a" (lo), "=d" (hi));
  return (((uint64_t)hi << 32) | lo);
}

// Wait until `ts` is in the past and what time that observation occurred.
static __inline __attribute__((always_inline))
ts wait_until(ts deadline) {
    ts now;
    do {
      now = rdtsc();
    } while (now < deadline);
    return now;
}

#endif
