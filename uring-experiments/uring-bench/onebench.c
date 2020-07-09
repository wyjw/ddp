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

#define DEPNO 8

struct sqring {
  uint32_t* head;
  uint32_t* tail;
  uint32_t* mask;
  uint32_t* entries;
  uint32_t* flags;
  uint32_t* dropped;
  uint32_t* array;
  struct io_uring_sqe* sqes;
  struct iovec* vecs;
};

struct cqring {
  uint32_t* head;
  uint32_t* tail;
  uint32_t* mask;
  uint32_t* entries;
  uint32_t* overflow;
  struct io_uring_cqe* cqes;
};

#define read_barrier() __sync_synchronize()
#define write_barrier() __sync_synchronize()
#define likely(e) __builtin_expect(e, 1)
#define unlikely(e) __builtin_expect(e, 0)

// Returns 0 if no CQE was reaped or the start ts of the reaped request.
ts cq_poll(struct cqring* cq) {
  uint32_t head = *cq->head;

  read_barrier();

  if (head == *cq->tail)
    return 0;

  uint32_t index = head & *cq->mask;
  struct io_uring_cqe* cqe = &cq->cqes[index];
  if (cqe->res < 0) {
    fprintf(stderr, "# WARNING - I/O failure: %d [%s] (flags %u); "
                    "cqe index %u user_data %llu\n",
            cqe->res, strerror(cqe->res), cqe->flags, index, cqe->user_data);
  }
  // XXX Do we want some extra check to ensure we don't hit short reads?

  ts r = cqe->user_data;

  head++;
  *cq->head = head;

  write_barrier();

  return r;
}

void init_io(struct iovec* vec, off_t off, struct io_uring_sqe* sqe) {
  sqe->opcode = IORING_OP_READV;
  sqe->flags = IOSQE_FIXED_FILE;
  sqe->ioprio = 0;
  sqe->fd = 0; // See io_uring_register; index into a "registered fd array".
  sqe->off = off;
  sqe->addr = (uintptr_t)vec;
  sqe->len = 1;
  sqe->rw_flags = 0;
  sqe->user_data = rdtsc(); // XXX This isn't right -- should use last deadline?
  sqe->buf_index = 0;
}

void sq_submit_io(struct sqring* sq, void* buf, size_t len, off_t off) {
  uint32_t tail = *sq->tail;
  const uint32_t index = tail & *sq->mask;

  // Fill in SQE with specifics for this IO request.
  struct iovec* vec = &sq->vecs[index];
  vec->iov_base = buf;
  vec->iov_len = len;
  init_io(vec, off, &sq->sqes[index]);

  // Push SQE index into the array and bump tail to kick IO
  sq->array[index] = index;
  tail++;
  write_barrier();
  *sq->tail = tail;
  write_barrier();
}

void sq_setup(int ring_fd, const struct io_uring_params *p, struct sqring* sq) {
  void* ptr = mmap(NULL, p->sq_off.array + p->sq_entries * sizeof(__u32),
                   PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
                   ring_fd, IORING_OFF_SQ_RING);
  assert(ptr != MAP_FAILED);

  sq->head = ptr + p->sq_off.head;
  sq->tail = ptr + p->sq_off.tail;
  sq->mask = ptr + p->sq_off.ring_mask;
  sq->entries = ptr + p->sq_off.ring_entries;
  sq->flags = ptr + p->sq_off.flags;
  sq->dropped = ptr + p->sq_off.dropped;
  sq->array = ptr + p->sq_off.array;

  ptr = mmap(NULL, p->sq_entries * sizeof(struct io_uring_sqe),
             PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
             ring_fd, IORING_OFF_SQES);
  assert(ptr != MAP_FAILED);

  sq->sqes = (struct io_uring_sqe*)ptr;

  sq->vecs = malloc(sizeof(struct iovec) * *sq->entries);
  assert(sq->vecs != NULL);
}

void cq_setup(int ring_fd, const struct io_uring_params *p, struct cqring* cq) {
  void* ptr = mmap(NULL, p->cq_off.cqes + p->cq_entries * sizeof(struct io_uring_cqe),
                   PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
                   ring_fd, IORING_OFF_CQ_RING);
  assert(ptr != MAP_FAILED);

  cq->head = ptr + p->cq_off.head;
  cq->tail = ptr + p->cq_off.tail;
  cq->mask = ptr + p->cq_off.ring_mask;
  cq->entries = ptr + p->cq_off.ring_entries;
  cq->overflow = ptr + p->cq_off.overflow;
  cq->cqes = ptr + p->cq_off.cqes;
}

const size_t MAX_SAMPLES = 1024 * 1024;

uint64_t convert(uint64_t ns)
{
  return ns * 1000000000 / cycles_per_second;
}

void io_bench(
    const char* filepath,
    uint64_t reqs_per_second,
    size_t buf_size,
    size_t max_off,
    uint64_t seconds,
    uint64_t warmup_seconds,
    bool use_polling,
    int dep)
{
  uint64_t* samples = mmap(NULL, MAX_SAMPLES * sizeof(uint64_t),
                           PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_POPULATE | MAP_ANONYMOUS,
                           -1, 0);
  assert(samples != MAP_FAILED);
  assert(((uintptr_t)samples & 0xfff) == 0);
  // Write-fault all pages just to ensure things are smooth on sampling.
  for (char* p = (char*)samples;
       p < ((char*)samples + MAX_SAMPLES * sizeof(uint64_t));
       p += 4096)
       p[0] = 1;

  size_t sample_count = 0;

  struct cqring cq;
  struct sqring sq;

  char* buf = mmap(NULL, buf_size,
                   PROT_READ | PROT_WRITE,
                   MAP_SHARED | MAP_POPULATE | MAP_ANONYMOUS,
                   -1, 0);
  assert(buf != MAP_FAILED);
  assert(((uintptr_t)buf & 0xfff) == 0);

  const uint32_t ENTRIES = 4096;

  struct io_uring_params params;
  memset(&params, 0, sizeof(params));
  if (use_polling) {
    params.flags = IORING_SETUP_IOPOLL | IORING_SETUP_SQPOLL;
    //params->sq_idle_thread = 1000; // kthread poller idle time in ms
  }

  int ring_fd = io_uring_setup(ENTRIES, &params);
  if (ring_fd == -1) {
    fprintf(stderr, "# io_uring_setup failed: %s\n", strerror(errno));
    exit(-1);
  }

  sq_setup(ring_fd, &params, &sq);
  cq_setup(ring_fd, &params, &cq);

  fprintf(stderr, "# sq ring size: %u\n", *sq.entries);
  fprintf(stderr, "# cq ring size: %u\n", *cq.entries);

  int fd = open(filepath, O_RDONLY | O_DIRECT);
  assert(fd > 0);

  // Registering the file avoids the need for some refcount
  // maintanance on the kernel-side that can slow things down under
  // high IOPS.
  int r = io_uring_register(ring_fd, IORING_REGISTER_FILES, &fd, 1);
  assert(r != -1);

  
  /*
  ts start = begin;
  ts deadline = begin + ns_between_reqs;
  ts last_dump = begin;

  ts warmup_done_ts = (warmup_seconds * cycles_per_second) + begin;
  ts all_done_ts = (seconds * cycles_per_second) + warmup_done_ts;
  bool warmup = true;
  */
  size_t off_mask = max_off - 1;
  ts all_done_ts = (seconds * cycles_per_second);
  ts cqe_ts;

  size_t off = xorshift64() & off_mask & (~0xfff); // O_DIRECT needs block-aligned reads.
  int i = 0;
  ts begin = rdtsc();
  sq_submit_io(&sq, buf, buf_size, off);
  while (begin < begin + all_done_ts)
  {
    r = io_uring_enter(ring_fd, 1, 0, IORING_ENTER_GETEVENTS, NULL);
    assert(r != -1);
    cqe_ts = cq_poll(&cq);
    ts end = rdtsc();
    if (cqe_ts != 0)
    {
      i++;
      if (i < dep)
      {
        off = xorshift64() & off_mask & (~0xfff); // O_DIRECT needs block-aligned reads.
        sq_submit_io(&sq, buf, buf_size, off);
      }
      else{
        break;
      }
    }
  }
  ts end = rdtsc();
  ts total_time = end - begin;
  printf("Total time: %lu\n", convert(total_time));

  /*
  while (start < all_done_ts) {
    // Reap ready CQEs eagerly because to attach end timestamps to them.
    ts cqe_ts;
    while (start < deadline) {
      if ((cqe_ts = cq_poll(&cq)) != 0) { // XXXXX Need to do wait until logic here too....
        ts end = rdtsc();
        completed++;

        warmup = warmup && (end < warmup_done_ts);
        if (!warmup) {
          samples[sample_count++] = (end - cqe_ts);
          if (sample_count == MAX_SAMPLES) {
            fprintf(stderr, "# WARNING - lapping sample array\n");
            sample_count = 0;
          }
        }

        if ((end - last_dump) > dump_period_ns) {
          fprintf(stderr, "%scompletions/s: %.2f\n",
                  warmup ? "# [WARMUP] " : "# ",
                  (completed - last_completed) /
                    ((double)(end - last_dump) / cycles_per_second));
          last_dump = end;
          last_completed = completed;
        }
      }

      start = rdtsc();
    }

    if ((start - deadline) > warn_margin_ns)
      fprintf(stderr, "# WARNING - missed deadline by %lu ns\n", start - deadline);

    size_t off = xorshift64() & off_mask & (~0xfff); // O_DIRECT needs block-aligned reads.
    sq_submit_io(&sq, buf, buf_size, off);
    if (likely(use_polling)) {
      if ((*sq.flags) & IORING_SQ_NEED_WAKEUP) {
        int r = io_uring_enter(ring_fd, 1, 0, IORING_ENTER_SQ_WAKEUP, NULL);
        assert(r != -1);
      }
    } else {
      int r = io_uring_enter(ring_fd, 1, 0, IORING_ENTER_GETEVENTS, NULL);
      assert(r != -1);
    }

    submitted++;

    deadline += (ns_between_reqs * cycles_per_second) / 1000000000;
  }

  fprintf(stderr, "# Total samples: %lu\n", sample_count);
  for (size_t i; i < sample_count; ++i) {
    uint64_t cycles = samples[i];
    uint64_t ns = cycles * 1000000000 / cycles_per_second;
    printf("%lu\n", ns);
  }
  */
  close(fd);
  close(ring_fd);
}

int main(int argc, char* argv[]) {
  const char* filepath = "/dev/nvme0n1";
  bool use_polling = false;
  uint64_t reqs_per_second = 1000;
  uint64_t warmup_seconds = 30;
  uint64_t seconds = 30;
  size_t max_off = 128lu * (1 << 30);
  size_t buf_size = 512;
  uint64_t dep_no = 4;

  int opt;
  while ((opt = getopt(argc, argv, "pr:f:w:s:o:b:d:")) != -1) {
    switch(opt) {
      case 'f':
        filepath = optarg;
        break;
      case 'p':
        use_polling = true;
        break;
      case 'r':
        reqs_per_second = atol(optarg);
        break;
      case 's':
        seconds = atol(optarg);
        break;
      case 'w':
        warmup_seconds = atol(optarg);
        break;
      case 'b':
        buf_size = atol(optarg);
        break;
      case 'o':
        max_off = atol(optarg);
        break;
      case 'd':
        dep_no = atol(optarg);
        break;
      case ':':
        fprintf(stderr, "option needs a value\n");
        break;
      case '?':
        fprintf(stderr, "unknown option: %c\n", optopt);
        break;
    }
  }

  assert(reqs_per_second > 0 && reqs_per_second < 100000);
  assert(seconds > 0 && seconds < 100000);
  assert(warmup_seconds > 0 && warmup_seconds < 100000);

  fprintf(stderr, "# filepath %s\n", filepath);
  fprintf(stderr, "# use_polling %d\n", use_polling);
  fprintf(stderr, "# reqs_per_second %lu\n", reqs_per_second);
  fprintf(stderr, "# buf_size %lu\n", buf_size);
  fprintf(stderr, "# max_off %lu\n", max_off);
  fprintf(stderr, "# seconds %lu\n", seconds);
  fprintf(stderr, "# warmup_seconds %lu\n", warmup_seconds);
  fprintf(stderr, "# depno %lu\n", dep_no);
  bool r = cycles_init();
  assert(r);

  io_bench(filepath, reqs_per_second, buf_size, max_off, seconds, warmup_seconds, use_polling, dep_no);
  return 0;
}
