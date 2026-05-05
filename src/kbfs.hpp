#pragma once

#include "graph.hpp"
#include <random>
#include <numeric>
#include <algorithm>

/**
 * k-BFS Eccentricity Estimation Algorithm
 * As described in: "Parallel Algorithms for Eccentricity Computation" (Shun et al.)
 *
 * Phase 1:
 *   - Sample k random source vertices S.
 *   - Run BFS from each s in S.
 *   - d(v, S) = max_{s in S} d(v, s)  (the highest BFS level that visits v).
 *
 * Phase 2:
 *   - S' = k vertices with the largest d(v, S) values.
 *   - Run BFS from each vertex in S'.
 *   - d(v, S') = max_{s' in S'} d(v, s')
 *
 * Final estimate:
 *   ê(v) = max(d(v, S), d(v, S'))
 *
 * The diameter estimate is max_v ê(v).
 *
 * Total work: O(2km)   (k BFS's each phase)
 */

struct KBFSResult {
    int diameter_estimate;   // max_v ê(v)
    int num_bfs;             // total number of BFS calls (= 2k)
    std::vector<int> ecc_hat; // ê(v) for each vertex v
    std::vector<int> dS;      // d(v, S): max distance to Phase-1 sources (after phase 1)
};

/**
 * @param g   Connected undirected unweighted graph (use largest_component() first)
 * @param k   Number of BFS sources per phase
 * @param seed Random seed for source selection
 */
inline KBFSResult kbfs_diameter(const Graph& g, int k, unsigned seed = 42) {
    KBFSResult res;
    int n = g.n;
    res.dS.assign(n, 0);
    res.ecc_hat.assign(n, 0);
    res.num_bfs = 0;

    std::mt19937 rng(seed);

    // ── Phase 1: Sample k random sources S ──────────────────────────
    int k1 = std::min(k, n);
    std::vector<int> perm(n);
    std::iota(perm.begin(), perm.end(), 0);
    std::shuffle(perm.begin(), perm.end(), rng);
    std::vector<int> S(perm.begin(), perm.begin() + k1);

    // BFS from each s in S; d(v, S) = max level that visits v
    for (int s : S) {
        auto dist = g.bfs(s);
        res.num_bfs++;
        for (int v = 0; v < n; v++) {
            if (dist[v] > res.dS[v])
                res.dS[v] = dist[v];
        }
    }

    // ── Phase 2: S' = k vertices with largest d(v, S) ───────────────
    // Sort by d(v,S) descending, take top k
    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::partial_sort(order.begin(), order.begin() + k1, order.end(),
                      [&](int a, int b) { return res.dS[a] > res.dS[b]; });
    std::vector<int> Sp(order.begin(), order.begin() + k1);

    // BFS from each s' in S'; d(v, S') = max level
    std::vector<int> dSp(n, 0);
    for (int sp : Sp) {
        auto dist = g.bfs(sp);
        res.num_bfs++;
        for (int v = 0; v < n; v++) {
            if (dist[v] > dSp[v])
                dSp[v] = dist[v];
        }
    }

    // ── Final estimate: ê(v) = max(d(v,S), d(v,S')) ─────────────────
    res.diameter_estimate = 0;
    for (int v = 0; v < n; v++) {
        res.ecc_hat[v] = std::max(res.dS[v], dSp[v]);
        if (res.ecc_hat[v] > res.diameter_estimate)
            res.diameter_estimate = res.ecc_hat[v];
    }

    return res;
}
