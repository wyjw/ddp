all: bench benchtry onebench

%.o: %.c
	gcc -O3 -std=gnu11 $< -o $@ -g -c

bench: bench.o cycles.o syscall.o 
	gcc -O3 -std=gnu11 $^ -o $@ -luring -g

benchtry: benchtry.o cycles.o syscall.o
	gcc -O3 -std=gnu11 $^ -o $@ -luring -g

onebench: onebench.o cycles.o syscall.o
	gcc -O3 -std=gnu11 $^ -o $@ -luring -g

cleanup: cleanup.o syscall.o
	gcc -O3 -std=gnu11 $^ -o $@ -luring -g

.PHONY: clean

clean:
	-rm bench *.o
	-rm cleanup
	-rm benchtry
	-rm onebench
