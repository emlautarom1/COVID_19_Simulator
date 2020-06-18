WARNS=-Wextra -Wall -Wundef  -Wfloat-equal -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wstrict-overflow=5 -Wwrite-strings -Waggregate-return -Wcast-qual -Wswitch-default -Wswitch-enum -Wconversion -Wunreachable-code
# OMP_NUM_THREADS=12
NP=6
GUI=f # t
ROWS=60
COLS=60
DEBUG=-DDEBUG=true # -DNDEBUG
CFLAGS=$(WARNS) --std=c99 -O0 -lSDL2 # -fopenmp

info:
	@ echo "Info: Covid-19 Simulator"

build: src/main.c src/main-mpi.c src/simulation.h src/utils.h
	gcc src/main.c -o build/main $(CFLAGS) $(DEBUG)
	mpicc src/main-mpi.c -o build/main-mpi $(CFLAGS)

run: build/main
	./build/main $(ROWS) $(COLS) $(GUI)

run-mpi: build/main-mpi
	mpirun -np $(NP) ./build/main-mpi $(ROWS) $(COLS)

bench: src/main.c src/simulation.h src/utils.h
	$(CC) src/main.c -o build/main -pg $(CFLAGS)
	make run
	@gprof build/main gmon.out > benchmark/$$(date +'%F_%k-%M')_bench.txt
	rm gmon.out

test: test/test.c
	mpicc test/test.c -o build/test $(CFLAGS)
	mpirun -np $(NP) build/test

.PHONY: clean
clean:
	@ -rm -f build/*