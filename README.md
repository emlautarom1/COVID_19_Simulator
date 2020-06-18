# Simulador COVID 19

## Requirements:
```
- OpenMP
- MPI (mpicc, mpirun)
- SDL2
```

## Build All
```
make build
```

## Run

- **Sequential**: `make run`
- **MPI**: `make run-mpi`
- **OpenMP**: `make run-omp`
- **Hybrid (MPI + OpenMP)**: `make run-hyb`

## Make Flags
- `ROWS :: Int`: Matrix number of rows (200, 800, 1500, ...)
- `COLS :: Int`: Matrix number of columns (200, 800, 1500, ...)
- `GUI :: 't' | 'f'`: Enable or disable SDL2 GUI. Quit with `Q`, decrease and increase the simulation speed with `[` and `]`, respectively.

### Example:

Run Hybrid with a 1500x1500 Matrix, no GUI.
```
make run-hyb ROWS=1500 COLS=1500 GUI=f
```

### Notes:

You can enable optimizations and disable assert checks for improved performance, see `Makefile`