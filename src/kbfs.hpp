#pragma once

#include "graph.hpp"
#include <vector>
#include <cstdint>
#include <random>
#include <algorithm>
#include <numeric>
#include <atomic>

/**
 * k-BFS Eccentricity Estimation Algorithm (Optimized with Bit-Vectors)
 * As described in: "Parallel Algorithms for Eccentricity Computation" (Shun et al.)
 *
 * Optimization:
 * Uses 64-bit integers to run up to 64 BFS searches simultaneously. 
 * A bitwise-OR and Compare-and-Swap (CAS) approach reduces cache misses 
 * by checking the state of all k searches across an edge in a single instruction.
 */

struct KBFSResult {
    int diameter_estimate;
    int num_bfs;
    std::vector<int> ecc_hat; 
    std::vector<int> dS;      
};

// Helper function to run a 64-bit parallelized BFS phase
inline std::vector<int> run_kbfs_phase(const Graph& g, const std::vector<int>& sources) {
    int n = g.n;
    std::vector<uint64_t> Visited(n, 0);
    std::vector<std::atomic<uint64_t>> NextVisited(n);
    std::vector<std::atomic<int>> Ecc(n);

    for (int i = 0; i < n; ++i) {
        NextVisited[i].store(0, std::memory_order_relaxed);
        Ecc[i].store(-1, std::memory_order_relaxed); // -1 signifies unvisited
    }

    std::vector<int> frontier;
    
    // Initialize sources: Each source gets a unique bit (1 << i)
    for (size_t i = 0; i < sources.size(); ++i) {
        int s = sources[i];
        uint64_t bit = (1ULL << i);
        Visited[s] |= bit;
        NextVisited[s].fetch_or(bit, std::memory_order_relaxed);
        Ecc[s].store(0, std::memory_order_relaxed);
        frontier.push_back(s);
    }

    int round = 0;
    while (!frontier.empty()) {
        std::vector<int> next_frontier;
        round++;

        // Edge traversal: process the current frontier
        for (int u : frontier) {
            uint64_t u_vis = Visited[u];
            
            for (int v : g.adj[u]) {
                uint64_t v_vis = Visited[v];
                
                // If u has been reached by a BFS that hasn't reached v yet
                if ((v_vis | u_vis) != v_vis) {
                    
                    // Atomically apply the new BFS visits to the neighbor's NextVisited state
                    NextVisited[v].fetch_or(u_vis, std::memory_order_relaxed);

                    // Atomically update the eccentricity to the current round
                    int old_ecc = Ecc[v].load(std::memory_order_relaxed);
                    if (old_ecc != round) {
                        // CAS ensures a vertex is only added to the next_frontier once per round
                        if (Ecc[v].compare_exchange_strong(old_ecc, round, std::memory_order_relaxed)) {
                            // Note: In a fully parallel (e.g., OpenMP) environment, 
                            // this push_back must be locked or use thread-local queues.
                            next_frontier.push_back(v);
                        }
                    }
                }
            }
        }

        // Synchronization: Copy NextVisited into Visited for the new frontier
        for (int v : next_frontier) {
            Visited[v] = NextVisited[v].load(std::memory_order_relaxed);
        }

        frontier = std::move(next_frontier);
    }

    // Convert atomic Ecc values to a standard vector
    std::vector<int> res(n);
    for (int i = 0; i < n; ++i) {
        res[i] = std::max(0, Ecc[i].load(std::memory_order_relaxed));
    }
    return res;
}

inline KBFSResult kbfs_diameter(const Graph& g, int k, unsigned seed = 42) {
    KBFSResult res;
    int n = g.n;
    res.dS.assign(n, 0);
    res.ecc_hat.assign(n, 0);
    
    // Clamp k to 64 to fit perfectly into our uint64_t bit-vector optimization
    int k_actual = std::min({k, n, 64}); 
    res.num_bfs = 2 * k_actual; 

    std::mt19937 rng(seed);

    // ── Phase 1: Sample k random sources S ──────────────────────────
    std::vector<int> perm(n);
    std::iota(perm.begin(), perm.end(), 0);
    std::shuffle(perm.begin(), perm.end(), rng);
    std::vector<int> S(perm.begin(), perm.begin() + k_actual);

    // Execute the parallel bit-vector BFS phase
    res.dS = run_kbfs_phase(g, S);

    // ── Phase 2: S' = k vertices with largest d(v, S) ───────────────
    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    
    // Sort descending by d(v,S) to find the farthest vertices
    std::partial_sort(order.begin(), order.begin() + k_actual, order.end(),
                      [&](int a, int b) { return res.dS[a] > res.dS[b]; });
                      
    std::vector<int> Sp(order.begin(), order.begin() + k_actual);

    // Execute the second parallel bit-vector BFS phase
    std::vector<int> dSp = run_kbfs_phase(g, Sp);

    // ── Final estimate: ê(v) = max(d(v,S), d(v,S')) ─────────────────
    res.diameter_estimate = 0;
    for (int v = 0; v < n; v++) {
        res.ecc_hat[v] = std::max(res.dS[v], dSp[v]);
        if (res.ecc_hat[v] > res.diameter_estimate) {
            res.diameter_estimate = res.ecc_hat[v];
        }
    }

    return res;
}