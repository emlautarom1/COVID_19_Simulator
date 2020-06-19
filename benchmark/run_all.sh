targets="run run-mpi run-omp run-hyb"
sizes="200 800 1500"
resamples=10
np=4

echo "Running benchmarks with $resamples samples each"
echo "This may take a while..."
for t in $targets; do
    for s in $sizes; do
        bench "make $t NP=$np ROWS=$s COLS=$s GUI=f" --resamples $resamples --output benchmark/${t}_${s}.html
    done
    echo "[INFO] Done with target '$t'"
done
