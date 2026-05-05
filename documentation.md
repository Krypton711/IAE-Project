# Parallel Graph Eccentricity Estimation Benchmark

This project benchmarks parallel graph eccentricity and diameter estimation algorithms, specifically comparing an optimized k-BFS approach against the theoretical RV algorithm (Roditty and Vassilevska Williams).

## File Structure

The project expects the following directory layout:
\`\`\`
.
├── src/
│   ├── benchmark.cpp      # Main execution script; handles IO, timing, and CSV generation.
│   ├── graph.hpp          # Adjacency list graph representation with SNAP edge-list parsing.
│   ├── kbfs.hpp           # Optimized k-BFS implementation using 64-bit vector parallelism.
│   ├── rv.hpp             # Sequential implementation of the RV eccentricity algorithm.
│   └── naive.hpp          # Exact all-pairs BFS algorithm (used only for small verification graphs).
└── datasets/              # Directory containing the unweighted, undirected SNAP graph files (.txt).
\`\`\`

## How to Compile and Run

### 1. Compilation
The algorithms utilize `std::atomic` and modern C++ features. Compile the source using C++17 and the highest optimization flag (`-O3`):

\`\`\`bash
g++ -std=c++17 -O3 src/benchmark.cpp -o benchmark
\`\`\`

*(Note: On Windows using MSVC, use `cl /EHsc /O2 /std:c++17 src\benchmark.cpp`)*

### 2. Verification Step
Before running large datasets, verify that the core algorithms compute correct bounds on small, known graph topologies (Path, Cycle, Star, K5, Binary Tree):

\`\`\`bash
./benchmark --verify
\`\`\`

### 3. Full Benchmark Execution
Run the benchmark across all graphs in the `datasets/` folder. We use the `--no-naive` flag to skip the exhaustive $O(mn)$ exact calculation, relying instead on a hardcoded ground-truth dictionary for accuracy metrics.

\`\`\`bash
./benchmark --no-naive --output results/final_benchmark.csv datasets/
\`\`\`

Once execution is complete, the results, including approximation ratios and BFS call counts, will be available in `results/final_benchmark.csv`.