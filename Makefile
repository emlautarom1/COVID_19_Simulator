WARNS=-Wextra -Wall -Wundef  -Wfloat-equal -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wstrict-overflow=5 -Wwrite-strings -Waggregate-return -Wcast-qual -Wswitch-default -Wswitch-enum -Wconversion -Wunreachable-code
# OMP_NUM_THREADS=12
NP=6
GUI=t # f
ROWS=60
COLS=60
FAST=-O3 -DDEBUG=0 -DNDEBUG
SLOW=-O0 -DDEBUG=1
# Select SLOW or FAST depending on your test case
CFLAGS=$(WARNS) --std=c99 $(SLOW) -lSDL2

info:
	@ echo "Info: Covid-19 Simulator"
	@ echo "See README.md for more info"

build: src/main.c src/main-mpi.c src/main-omp.c src/main-hyb.c src/simulation.h src/utils.h
	gcc src/main.c -o build/main $(CFLAGS)
	mpicc src/main-mpi.c -o build/main-mpi $(CFLAGS)
	gcc src/main-omp.c -o build/main-omp $(CFLAGS) -fopenmp
	mpicc src/main-hyb.c -o build/main-hyb $(CFLAGS) -fopenmp

run: build/main
	./build/main $(ROWS) $(COLS) $(GUI)

run-mpi: build/main-mpi
	mpirun -np $(NP) ./build/main-mpi $(ROWS) $(COLS) $(GUI)

run-omp: build/main-omp
	./build/main-omp $(ROWS) $(COLS) $(GUI)

run-hyb: build/main-hyb
	mpirun -np $(NP) ./build/main-hyb $(ROWS) $(COLS) $(GUI)

bench: build
	@ bash benchmark/run_all.sh

test: test/test.c
	mpicc test/test.c -o build/test $(CFLAGS)
	mpirun -np $(NP) build/test

.PHONY: clean
clean:
	@ -rm -f build/*
	@ -rm -f benchmark/*.html