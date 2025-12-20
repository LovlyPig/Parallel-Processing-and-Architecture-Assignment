# Requirment
- C++17 compiler (g++)
- tbb
```
sudo apt update && sudo apt install libtbb-dev
```
- python3 with matplotlib and pandas

# build
- compile and run
```
make all
./benchmark
```
Default output CSV: benchmark.csv
- plot
```
   python3 plot_results.py benchmark_results.csv
```
Default output png: plots/result.png
