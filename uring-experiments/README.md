# uring-experiments

A set of benchmarks wrapped over SPDK perf and fio for testing and comparing
I/O performance through io_uring, SPDK, and conventional I/O syscalls (AIO,
sync).

## Running

### Setup

The goal is that these tests are fairly automated including setup.

```
$ ./scripts/setup
```

This should fetch and compile fio and SPDK.

### Config

Edit `config.sh`. This builds a list of NVMe devices (both by kernel name and by PCIe name) that will be profiled in the experiments.

### Experiments

For now, there are two experiments `open-loop` and `perf`. Before running
experiments, one needs to setup a results repo. From the top-level directory,
create a `results` directory and initialize a git repo in it. All runs log
results, data, and figures there after each step. After each step the scripts
commit to the repo and perform a push. So, you'll need to set up a valid origin
push target for the `results` repo as well (e.g. on github configured with a
deploy key, which lets you watch/collect results via your browser as things
run).

#### open-loop

`open-loop/run` runs through io_uring, SPDK, and libaio and runs a
pseudo-open-loop (max iodepth=1024, with max iops slowly ramped up in steps of
100,000). For io_ring it always uses IOPOLL (fio hipri=1), but it runs in both
with kernel-size SQ polling (sqthread_poll=1) and without (these are labeled in
the output as Blocking and Polling, respectively, for now). The end result is a
pair of latency-throughput curves: one for median and one for 99th percentile.

This experiment also varies the number of threads making submissions. For
example, the CPU saturation issues that arise from SQ polling threads become
evident in this experiment as the number of submission threads starts to go up.

Because of the large number of configurations this tests (30 s per sample with
30 s more each for warmup) this can take a very long time to run. If run with
10 iterations per configuration it can take 1 to 2 days to generate all of its
data.

### perf

`perf/run` is similar to `open-loop` but it uses SPDK's `perf` tool to do
similar measurements. `perf` claims to have lower overhead and better fidelity
at high-throughput/low-latency.

`perf` doesn't support warmup, so each run takes 30 s. It also doesn't support
fine-grained target IOPS for an open loop load, so the script varies IO depth
instead to control concurrency/throughput.

## Other scripts

There are many other helper scripts used in the experiments and independently
useful as well.

- `./scripts/spdk-enable` and `./scripts/spdk-disable` detach/reattach devices
  from `config.sh` from/to Linux. These are used within scripts when switching
  between configurations that use Linux-based I/O interfaces and bare
  kernel-bypass-based interfaces.
- `./scripts/log-ints` dump out the number of NVMe interrupts the combined
  system handled in the last second once per second. This is useful for
  ensuring e.g. that io_uring IOPOLL mode is, indeed, eliminating interrupts
  (which might not happen if the `nvme` module isn't configured with
  `poll_queues`).
- `./scripts/fio` runs `fio` but through the SPDK wrapper that preloads the SPDK engine.
