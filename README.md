# Cache-Benchmarking

The aim of the project was to study the cache hierarchy of a processor and estimate the capacities of its L1, L2 and L3 caches using C++.

The benchmarking was performed on an Intel Core i5-12500H processor by varying the amount of memory accessed and measuring the time taken for memory accesses.

## Files

- `core_check.cpp` – Identifies the P-cores and E-cores of the processor.
- `cache.cpp` – Main program used for cache benchmarking through pointer chasing.
- `plot_cache.py` – Python script used to generate the benchmark graph.
- `cache_results.csv` – Contains the measurements collected during the benchmark.
- `cache.png` – Graph showing memory access latency against working-set size.

## Hardware and Software

- Processor: Intel Core i5-12500H (12th Gen)
- Programming Language: C++
- Compiler: GCC (MinGW)
- Plotting: Python and Matplotlib
- Operating System: Windows

## Approach

The benchmark uses a pointer-chasing technique to study memory access latency for different working-set sizes.

A circular pointer chain is created in memory, with the nodes randomised within 64 KB regions. The working-set size is gradually increased to observe changes in access latency.

The benchmark is executed on logical processor 0, which was identified as a P-core. Each working-set size is tested over 30 trials, with 600,000 pointer accesses per trial.

Mean latency, median latency and standard deviation are calculated for the measurements.

## Results

The experiments gave the following approximate cache capacity estimates:

| Cache Level | Experimental Estimate |
|---|---:|
| L1 Data Cache | 48 KiB |
| L2 Cache | 1.25 MiB |
| L3 Cache | 17–18 MiB |

These estimates were obtained by observing changes in memory access latency and comparing them with the processor's documented cache specifications.

The results are plotted in `cache.png`, which shows the variation in median memory access latency with working-set size.

## Running the Programs

Compile and run the core identification program:

```powershell
g++ -O2 -std=c++17 core_check.cpp -o core_check.exe
.\core_check.exe
```

Compile and run the cache benchmark:

```powershell
g++ -O2 -std=c++17 cache.cpp -o cache.exe
.\cache.exe
```

Install Matplotlib:

```powershell
py -m pip install matplotlib
```

Generate the graph:

```powershell
py plot_cache.py
```

The benchmark measurements are stored in a CSV file, which is used by the Python script to generate the latency graph.
