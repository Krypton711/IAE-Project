# Walkthrough: k-BFS vs RV Diameter Comparison

## What Was Built

A C++ benchmarking framework to compare **k-BFS** and **RV** algorithms for graph diameter computation.

### Files Created

| File | Purpose |
|------|---------|
| [graph.hpp](file:///home/karthik-sundram/Documents/IAE-Project/src/graph.hpp) | Graph data structure (adjacency list, BFS, SNAP loader, LCC extraction) |
| [naive.hpp](file:///home/karthik-sundram/Documents/IAE-Project/src/naive.hpp) | Exact all-pairs BFS for ground truth |
| [kbfs.hpp](file:///home/karthik-sundram/Documents/IAE-Project/src/kbfs.hpp) | k-BFS with 3 strategies (random, high-degree, iterative) |
| [rv.hpp](file:///home/karthik-sundram/Documents/IAE-Project/src/rv.hpp) | Roditty-Vassilevska Williams 3/2-approximation |
| [benchmark.cpp](file:///home/karthik-sundram/Documents/IAE-Project/src/benchmark.cpp) | Main benchmarker with CSV output and verification mode |
| [Makefile](file:///home/karthik-sundram/Documents/IAE-Project/Makefile) | Build system |
| [datasets/README.md](file:///home/karthik-sundram/Documents/IAE-Project/datasets/README.md) | Dataset sources (SNAP, NetworkRepository, KONECT, DIMACS) with download commands |

## Verification Results

All 6 test cases **PASSED**:

```
Testing Path(5)   → Naive=4, k-BFS bounds bracket ✓, RV=4 ✓
Testing Cycle(5)  → Naive=2, k-BFS bounds bracket ✓, RV=2 ✓
Testing Star(5)   → Naive=2, k-BFS bounds bracket ✓, RV=2 ✓
Testing K5        → Naive=1, k-BFS bounds bracket ✓, RV=1 ✓
Testing BinTree(7)→ Naive=4, k-BFS bounds bracket ✓, RV=4 ✓
Testing Path(10)  → Naive=9, k-BFS bounds bracket ✓, RV=9 ✓
```

## Quick Start

```bash
# Build
make

# Verify correctness
./build/benchmark --verify

# Benchmark on datasets
./build/benchmark datasets/          # all files in directory
./build/benchmark graph.txt          # single file
./build/benchmark --no-naive graph.txt  # skip exact computation

# Output goes to results/benchmark.csv
```

## CSV Output Columns

`graph, nodes, edges, algorithm, parameter, strategy, estimated_diameter, true_diameter, approx_ratio, num_bfs, time_ms`
