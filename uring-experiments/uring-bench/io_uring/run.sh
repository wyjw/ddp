fallocate -l 10G a.img
./io_uring-bench a.img & ./io_uring-bench c.img & ./io_uring-bench b.img & 
