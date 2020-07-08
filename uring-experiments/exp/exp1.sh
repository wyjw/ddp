fio ../open-loop/run.fio -filename /dev/nvme0n1 --ioengine=io_uring --rate_iops=40000 --timeout 30 --output cn.test
