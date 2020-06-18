WARNS=-Wextra -Wall -Wundef  -Wfloat-equal -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wstrict-overflow=5 -Wwrite-strings -Waggregate-return -Wcast-qual -Wswitch-default -Wswitch-enum -Wconversion -Wunreachable-code
# OMP_NUM_THREADS=12
NP=6
GUI=t # f
ROWS=60
COLS=60
FAST=-O3 -DDEBUG=0 -DNDEBUG
SLOW=-O0 -DDEBUG=1
CFLAGS=$(WARNS) --std=c99 $(SLOW) -lSDL2

info:
	@ echo "Info: Covid-19 Simulator"

build: src/main.c src/main-mpi.c src/main-omp.c src/simulation.h src/utils.h
	gcc src/main.c -o build/main $(CFLAGS)
	mpicc src/main-mpi.c -o build/main-mpi $(CFLAGS)
	gcc src/main-omp.c -o build/main-omp $(CFLAGS) -fopenmp

run: build/main
	./build/main $(ROWS) $(COLS) $(GUI)

run-mpi: build/main-mpi
	mpirun -np $(NP) ./build/main-mpi $(ROWS) $(COLS) $(GUI)

run-omp: build/main-omp
	./build/main-omp $(ROWS) $(COLS) $(GUI)

bench: src/main.c src/simulation.h src/utils.h
	gcc src/main.c -o build/main -pg $(CFLAGS)
	./build/main 1500 1500 f
	@ gprof build/main gmon.out > benchmark/$$(date +'%F_%k-%M')_bench.txt
	@ rm gmon.out

test: test/test.c
	mpicc test/test.c -o build/test $(CFLAGS)
	mpirun -np $(NP) build/test

.PHONY: clean
clean:
	@ -rm -f build/*
	@ -rm -f benchmark/*