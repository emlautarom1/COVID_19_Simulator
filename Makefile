WARNS=-Wextra -Wall -Wundef  -Wfloat-equal -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wstrict-overflow=5 -Wwrite-strings -Waggregate-return -Wcast-qual -Wswitch-default -Wswitch-enum -Wconversion -Wunreachable-code
CC=mpicc
# OMP_NUM_THREADS=12
CFLAGS=$(WARNS) --std=c99 -O0 -lSDL2 -lm # -fopenmp

info:
	@ echo "Info: Covid-19 Simulator"

build: src/main.c
	$(CC) src/main.c -o build/main $(CFLAGS) 

run: build/main
	./build/main

bench: src/main.c
	$(CC) src/main.c -o build/main -pg $(CFLAGS)
	make run
	@gprof build/main gmon.out > benchmark/$$(date +'%F_%k-%M')_bench.txt
	rm gmon.out

.PHONY: clean
clean:
	@ -rm -f build/*