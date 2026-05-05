#pragma once

#include "graph.hpp"
#include <random>
#include <cmath>
#include <set>
#include <algorithm>

/**
 * RV Algorithm (Roditty-Vassilevska Williams, STOC 2013)
 *
 * 3/2-approximation of the diameter for undirected unweighted graphs.
 *
 * Algorithm:
 * 1. Sample a random set S of ceil(sqrt(n)) vertices.
 * 2. Run BFS from every vertex in S. Compute ecc(s) for each s in S.
 * 3. For each vertex v, find w(v) = argmax_{s in S} d(v, s).
 * 4. Collect the set W = { w(v) : v in V } of distinct "witness" vertices.
 * 5. Run BFS from each vertex in W. Compute ecc(w) for each w in W.
 * 6. For each vertex v:
 *      ecc_hat(v) = max( max_{s in S} d(v, s),  ecc(w(v)) )
 * 7. Diameter estimate = max_v ecc_hat(v).
 *
 * Guarantee: ceil(2D/3) <= D_hat <= D
 *
 * Time: O(|S| * (n + m) + |W| * (n + m))  ~  O(sqrt(n) * m) expected
 */

struct RVResult {
    int diameter_estimate;   // estimated diameter
    int num_bfs;             // total BFS calls performed
    std::vector<int> ecc_hat; // per-vertex eccentricity estimate
};

inline RVResult rv_diameter(const Graph& g, unsigned seed = 42) {
    RVResult res;
    res.ecc_hat.assign(g.n, 0);
    res.num_bfs = 0;

    std::mt19937 rng(seed);

    int sample_size = std::max(1, (int)std::ceil(std::sqrt(g.n)));

    // Step 1: Sample S
    std::vector<int> perm(g.n);
    std::iota(perm.begin(), perm.end(), 0);
    std::shuffle(perm.begin(), perm.end(), rng);

    std::vector<int> S(perm.begin(), perm.begin() + sample_size);

    // Step 2: BFS from each s in S, compute ecc(s) and distances
    // Store all distance arrays for S
    std::vector<std::vector<int>> dist_S(sample_size);
    std::vector<int> ecc_S(sample_size, 0);

    for (int i = 0; i < sample_size; i++) {
        dist_S[i] = g.bfs(S[i]);
        res.num_bfs++;
        for (int v = 0; v < g.n; v++) {
            ecc_S[i] = std::max(ecc_S[i], dist_S[i][v]);
        }
    }

    // Step 3: For each vertex v, find w(v) = argmax_{s in S} d(v, s)
    // Also compute max_{s in S} d(v, s) for the estimate
    std::vector<int> w(g.n);       // w[v] = index in S (not S[index])
    std::vector<int> max_d_S(g.n, 0); // max distance from v to any s in S

    for (int v = 0; v < g.n; v++) {
        int best_idx = 0;
        int best_dist = dist_S[0][v];
        for (int i = 1; i < sample_size; i++) {
            if (dist_S[i][v] > best_dist) {
                best_dist = dist_S[i][v];
                best_idx = i;
            }
        }
        w[v] = S[best_idx]; // actual vertex ID of w(v)
        max_d_S[v] = best_dist;
    }

    // Step 4: Collect distinct witness vertices W = { w(v) : v in V }
    std::set<int> W_set(w.begin(), w.end());
    std::vector<int> W(W_set.begin(), W_set.end());

    // Step 5: BFS from each vertex in W, compute ecc(w)
    std::unordered_map<int, int> ecc_W; // vertex -> eccentricity
    for (int ww : W) {
        auto dist = g.bfs(ww);
        res.num_bfs++;
        int ecc = 0;
        for (int v = 0; v < g.n; v++) {
            ecc = std::max(ecc, dist[v]);
        }
        ecc_W[ww] = ecc;
    }

    // Step 6: Compute ecc_hat(v) for each v
    for (int v = 0; v < g.n; v++) {
        res.ecc_hat[v] = std::max(max_d_S[v], ecc_W[w[v]]);
    }

    // Step 7: Diameter estimate
    res.diameter_estimate = *std::max_element(res.ecc_hat.begin(), res.ecc_hat.end());

    return res;
}
