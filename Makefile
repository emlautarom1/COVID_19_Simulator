WARNS=-Wextra -Wall -Wundef  -Wfloat-equal -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wstrict-overflow=5 -Wwrite-strings -Waggregate-return -Wcast-qual -Wswitch-default -Wswitch-enum -Wconversion -Wunreachable-code
CC=mpicc
# OMP_NUM_THREADS=12
CFLAGS=$(WARNS) -O0 -lm # -fopenmp

info:
	@ echo "Info: Covid-19 Simulator"

build: src/main.c
	$(CC) src/main.c -o build/main $(CFLAGS) 

run: build/main
	./build/main

.PHONY: clean
clean:
	@ -rm -f build/*