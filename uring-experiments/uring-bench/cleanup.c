#define _GNU_SOURCE

#include <stddef.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include <sys/mman.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

//#include <linux/io_uring.h>

//#include <liburing/io_uring.h>
//#include <liburing.h>
#include "liburing.h"

#include "cycles.h"
#include "rand.h"

int main(int argc, char* argv[]) {
  int r = io_uring_enter(0, 1, 0, IORING_ENTER_CLEANUP, NULL);
  return 0;
}
