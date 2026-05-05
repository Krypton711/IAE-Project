#pragma once

#include "graph.hpp"
#include <random>
#include <numeric>
#include <functional>

/**
 * k-BFS Eccentricity Bounding Algorithm
 *
 * Performs k BFS traversals from selected source vertices to compute
 * lower and upper bounds on eccentricity for every vertex.
 *
 * For each vertex v:
 *   Lower bound: eL(v) = max_{s in S} d(s, v)
 *   Upper bound: eU(v) = min_{s in S} (ecc(s) + d(s, v))
 *
 * Diameter estimate = max_v eL(v)
 *
 * Source selection strategies:
 *   RANDOM       - pick k random vertices
 *   HIGH_DEGREE  - pick k highest-degree vertices
 *   ITERATIVE    - greedily pick vertex with largest bound gap
 */

enum class KBFSStrategy {
    RANDOM,
    HIGH_DEGREE,
    ITERATIVE
};

struct KBFSResult {
    int diameter_lower;      // max lower bound = estimated diameter
    int diameter_upper;      // max upper bound
    int num_bfs;             // total BFS calls performed
    std::vector<int> ecc_lower;
    std::vector<int> ecc_upper;
};

inline KBFSResult kbfs_diameter(const Graph& g, int k, KBFSStrategy strategy = KBFSStrategy::RANDOM, unsigned seed = 42) {
    KBFSResult res;
    res.ecc_lower.assign(g.n, 0);
    res.ecc_upper.assign(g.n, std::numeric_limits<int>::max());
    res.num_bfs = 0;

    std::mt19937 rng(seed);

    // Helper: perform BFS from source and update bounds
    auto do_bfs = [&](int source) {
        auto dist = g.bfs(source);
        res.num_bfs++;

        // Eccentricity of the source = max distance from source
        int ecc_s = *std::max_element(dist.begin(), dist.end());

        for (int v = 0; v < g.n; v++) {
            if (dist[v] < 0) continue; // unreachable
            res.ecc_lower[v] = std::max(res.ecc_lower[v], dist[v]);
            res.ecc_upper[v] = std::min(res.ecc_upper[v], ecc_s + dist[v]);
        }
    };

    if (strategy == KBFSStrategy::RANDOM) {
        // Pick k random distinct vertices
        std::vector<int> perm(g.n);
        std::iota(perm.begin(), perm.end(), 0);
        std::shuffle(perm.begin(), perm.end(), rng);
        int actual_k = std::min(k, g.n);
        for (int i = 0; i < actual_k; i++) {
            do_bfs(perm[i]);
        }
    }
    else if (strategy == KBFSStrategy::HIGH_DEGREE) {
        // Pick k highest-degree vertices
        std::vector<int> order(g.n);
        std::iota(order.begin(), order.end(), 0);
        std::sort(order.begin(), order.end(), [&](int a, int b) {
            return g.adj[a].size() > g.adj[b].size();
        });
        int actual_k = std::min(k, g.n);
        for (int i = 0; i < actual_k; i++) {
            do_bfs(order[i]);
        }
    }
    else if (strategy == KBFSStrategy::ITERATIVE) {
        // First source: random
        {
            int s = rng() % g.n;
            do_bfs(s);
        }
        // Remaining k-1 sources: pick vertex with largest gap (eU - eL)
        for (int step = 1; step < k && step < g.n; step++) {
            int best = -1, best_gap = -1;
            for (int v = 0; v < g.n; v++) {
                int gap = res.ecc_upper[v] - res.ecc_lower[v];
                if (gap > best_gap) {
                    best_gap = gap;
                    best = v;
                }
            }
            if (best < 0 || best_gap == 0) break; // all exact
            do_bfs(best);
        }
    }

    // Compute diameter bounds
    res.diameter_lower = *std::max_element(res.ecc_lower.begin(), res.ecc_lower.end());
    res.diameter_upper = *std::max_element(res.ecc_upper.begin(), res.ecc_upper.end());

    return res;
}
