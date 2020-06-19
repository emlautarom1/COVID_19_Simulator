targets="run run-mpi run-omp run-hyb"
sizes="200 800 1500"
np=4

echo "Running benchmarks. This may take a while..."
for t in $targets; do
    for s in $sizes; do
        bench "make $t NP=$np ROWS=$s COLS=$s GUI=f" --output benchmark/${t}_${s}.html
    done
    echo "[INFO] Done with target '$t'"
done
