#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>

#include <stdio.h>
#include <sys/time.h>

#include "cycles.h"

bool cycles_init() {
  if (cycles_per_second != 0)
    return true;

  // Compute the frequency of the fine-grained CPU timer: to do this,
  // take parallel time readings using both rdtsc and gettimeofday.
  // After 10ms have elapsed, take the ratio between these readings.

  struct timeval start_time, stop_time;
  ts start_cycles, stop_cycles, micros;
  double old_cycles;

  // There is one tricky aspect, which is that we could get interrupted
  // between calling gettimeofday and reading the cycle counter, in which
  // case we won't have corresponding readings.  To handle this (unlikely)
  // case, compute the overall result repeatedly, and wait until we get
  // two successive calculations that are within 0.1% of each other.
  old_cycles = 0;
  while (1) {
    if (gettimeofday(&start_time, NULL) != 0) {
      fprintf(stderr, "cycles_init couldn't read clock: %s",
          strerror(errno));
      return false;
    }
    start_cycles = rdtsc();
    while (1) {
      if (gettimeofday(&stop_time, NULL) != 0) {
        fprintf(stderr, "cycles_init couldn't read clock: %s",
                strerror(errno));
      }
      stop_cycles = rdtsc();
      micros = (stop_time.tv_usec - start_time.tv_usec) +
              (stop_time.tv_sec - start_time.tv_sec)*1000000;
      if (micros > 10000) {
        cycles_per_second = (double)(stop_cycles - start_cycles);
        cycles_per_second = 1000000.0 * cycles_per_second / (double)(micros);
        break;
      }
    }
    double delta = cycles_per_second / 1000.0;
    if ((old_cycles > (cycles_per_second - delta)) &&
            (old_cycles < (cycles_per_second + delta))) {
      return true;
    }
    old_cycles = cycles_per_second;
  }
}

