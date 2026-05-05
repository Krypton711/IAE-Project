#pragma once

#include "graph.hpp"

/**
 * Naive exact diameter computation via all-pairs BFS.
 * Time: O(n * (n + m))
 * Returns the exact diameter (max eccentricity).
 * Also fills per-vertex eccentricities if pointer is provided.
 */
struct NaiveResult {
    int diameter;
    int num_bfs;   // number of BFS calls (= n)
    std::vector<int> eccentricities;
};

inline NaiveResult naive_diameter(const Graph& g) {
    NaiveResult res;
    res.eccentricities.resize(g.n, 0);
    res.diameter = 0;
    res.num_bfs = g.n;

    for (int s = 0; s < g.n; s++) {
        auto dist = g.bfs(s);
        int ecc = 0;
        for (int i = 0; i < g.n; i++) {
            ecc = std::max(ecc, dist[i]);
        }
        res.eccentricities[s] = ecc;
        res.diameter = std::max(res.diameter, ecc);
    }
    return res;
}
