#pragma once

#include "graph.hpp"
#include <random>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <limits>
#include <unordered_map>
#include <unordered_set>

/**
 * RV Eccentricity Estimation Algorithm
 * Roditty and Vassilevska Williams, STOC 2013.
 * As described (and implemented) in: "Parallel Algorithms for Eccentricity Computation" (Shun et al.)
 *
 * Guarantee: max(R, (2/3)*e(v)) <= ê(v) <= min(D, (3/2)*e(v))  w.h.p.
 *
 * Algorithm:
 * 1. Set s = ceil(sqrt(n * ln(n))) (the "ball size").
 *    Sample |S| = ceil(n/s * ln(n)) = Θ(sqrt(n/ln(n))) random vertices → set S.
 *
 * 2. BFS from each vertex in S.
 *    This gives d(v, q) for all v, q∈S.
 *    Compute:
 *      pS(v)  = argmin_{q∈S} d(v, q)   (closest sample to v)
 *      e(q)   for each q ∈ S           (exact eccentricities via BFS)
 *      max_{q∈S} d(v,q)                for all v
 *
 * 3. Find w = argmax_v d(v, pS(v))  (vertex farthest from its closest sample).
 *
 * 4. BFS from w.
 *    During this BFS:
 *      - Collect Ns(w) = s closest vertices to w (by BFS order = distance from w).
 *      - For each vertex v outside Ns(w), record vt = the first vertex of Ns(w)
 *        encountered on the BFS path from w to v (i.e., the ancestor of v in the
 *        BFS tree that is in Ns(w)).
 *      - Record d(v, w) for all v.
 *
 * 5. BFS from each vertex in Ns(w). Compute e(u) for each u ∈ Ns(w).
 *
 * 6. For each vertex v:
 *    - If v ∈ S ∪ Ns(w): eccentricity already known exactly.
 *    - Else:
 *        e'(v) = max(max_{q∈S} d(v,q), d(v,w))
 *        if d(v, vt) <= d(vt, w):
 *            ê(v) = max(e'(v), e(vt))
 *        else:
 *            ê(v) = max(e'(v), min_{q∈S} e(q))
 *
 * 7. Diameter estimate = max_v ê(v).
 */

struct RVResult {
    int diameter_estimate;      // estimated diameter
    int num_bfs;                // total BFS calls
    std::vector<int> ecc_hat;  // ê(v) for each vertex
};

inline RVResult rv_diameter(const Graph& g, unsigned seed = 42) {
    RVResult res;
    int n = g.n;
    res.ecc_hat.assign(n, 0);
    res.num_bfs = 0;

    std::mt19937 rng(seed);

    // ── Step 1: Compute parameters and sample S ──────────────────────
    double log_n = std::log(std::max(2, n));
    int s = std::max(1, (int)std::ceil(std::sqrt((double)n * log_n)));
    int S_size = std::max(1, (int)std::ceil((double)n / s * log_n));
    S_size = std::min(S_size, n);

    std::vector<int> perm(n);
    std::iota(perm.begin(), perm.end(), 0);
    std::shuffle(perm.begin(), perm.end(), rng);

    std::vector<int> S(perm.begin(), perm.begin() + S_size);
    std::unordered_set<int> S_set(S.begin(), S.end());

    // ── Step 2: BFS from each vertex in S ────────────────────────────
    // maxDistToS[v] = max_{q∈S} d(v, q)
    // closestInS[v] = index of pS(v) in S (closest sample to v)
    // closestDistToS[v] = d(v, pS(v))
    // ecc_S[i] = eccentricity of S[i]
    std::vector<int> maxDistToS(n, 0);
    std::vector<int> closestInS(n, 0);
    std::vector<int> closestDistToS(n, std::numeric_limits<int>::max());
    std::vector<int> ecc_S(S_size, 0);

    // Store all dist arrays for S (needed for ê(v) computation)
    std::vector<std::vector<int>> dist_S(S_size);

    for (int i = 0; i < S_size; i++) {
        dist_S[i] = g.bfs(S[i]);
        res.num_bfs++;
        for (int v = 0; v < n; v++) {
            int d = dist_S[i][v];
            if (d < 0) continue;
            ecc_S[i] = std::max(ecc_S[i], d);
            if (d > maxDistToS[v]) maxDistToS[v] = d;
            if (d < closestDistToS[v]) {
                closestDistToS[v] = d;
                closestInS[v] = i;
            }
        }
    }

    int minEccS = *std::min_element(ecc_S.begin(), ecc_S.end());

    // Mark eccentricities for vertices in S
    for (int i = 0; i < S_size; i++) {
        res.ecc_hat[S[i]] = ecc_S[i];
    }

    // ── Step 3: Find w = argmax_v d(v, pS(v)) ───────────────────────
    int w = 0;
    int w_dist = -1;
    for (int v = 0; v < n; v++) {
        if (closestDistToS[v] > w_dist) {
            w_dist = closestDistToS[v];
            w = v;
        }
    }

    // ── Step 4: Modified BFS from w ──────────────────────────────────
    // Collect Ns(w) = first s vertices visited (in BFS order from w).
    // For each v outside Ns(w), record vt = ancestor in Ns(w) on path from w to v.
    std::vector<int> dist_w(n, -1);
    std::vector<int> parent_w(n, -1);  // BFS parent
    std::vector<int> Ns_w;             // s closest vertices to w (BFS order)
    std::unordered_set<int> Ns_w_set;
    // vt[v] = closest ancestor of v (in BFS tree) that lies in Ns(w)
    std::vector<int> vt(n, -1);

    {
        std::queue<int> q;
        dist_w[w] = 0;
        parent_w[w] = w;
        q.push(w);
        Ns_w.push_back(w);
        Ns_w_set.insert(w);
        vt[w] = w;  // w is its own vt

        while (!q.empty()) {
            int u = q.front(); q.pop();
            for (int nb : g.adj[u]) {
                if (dist_w[nb] == -1) {
                    dist_w[nb] = dist_w[u] + 1;
                    parent_w[nb] = u;
                    // vt[nb] = vt[u] if u has a vt already in Ns(w)
                    if ((int)Ns_w.size() < s) {
                        // nb itself is being added to Ns(w)
                        Ns_w.push_back(nb);
                        Ns_w_set.insert(nb);
                        vt[nb] = nb;
                    } else {
                        vt[nb] = vt[u];
                    }
                    q.push(nb);
                }
            }
        }
        res.num_bfs++;
    }

    // Eccentricity of w itself
    {
        int ecc_w = 0;
        for (int v = 0; v < n; v++)
            if (dist_w[v] >= 0) ecc_w = std::max(ecc_w, dist_w[v]);
        res.ecc_hat[w] = ecc_w;
        if (S_set.count(w)) {
            // already set from S BFS; keep whichever
        }
    }

    // ── Step 5: BFS from each vertex in Ns(w) ────────────────────────
    std::unordered_map<int, int> ecc_Ns_w;   // vertex -> eccentricity
    std::unordered_map<int, std::vector<int>> dist_Ns_w; // dist arrays for Ns(w)
    for (int u : Ns_w) {
        auto dist_u = g.bfs(u);
        res.num_bfs++;
        int ecc_u = 0;
        for (int v = 0; v < n; v++)
            if (dist_u[v] >= 0) ecc_u = std::max(ecc_u, dist_u[v]);
        ecc_Ns_w[u] = ecc_u;
        res.ecc_hat[u] = ecc_u; // exact eccentricity for vertices in Ns(w)
        dist_Ns_w[u] = std::move(dist_u);
    }

    // Also exact for vertices in S (already computed above)

    // ── Step 6: Compute ê(v) for v ∉ S ∪ Ns(w) ──────────────────────
    for (int v = 0; v < n; v++) {
        if (S_set.count(v) || Ns_w_set.count(v)) continue;  // already have exact ecc

        int e_prime = std::max(maxDistToS[v], dist_w[v] >= 0 ? dist_w[v] : 0);

        int vt_v = vt[v];              // closest vertex in Ns(w) on path from w to v
        int d_v_vt = (vt_v >= 0 && dist_Ns_w.count(vt_v)) ? dist_Ns_w.at(vt_v)[v] : -1;
        int d_vt_w = (vt_v >= 0) ? dist_w[vt_v] : -1;

        int ecc_hat_v;
        if (vt_v >= 0 && d_v_vt >= 0 && d_vt_w >= 0 && d_v_vt <= d_vt_w) {
            ecc_hat_v = std::max(e_prime, ecc_Ns_w.at(vt_v));
        } else {
            ecc_hat_v = std::max(e_prime, minEccS);
        }
        res.ecc_hat[v] = ecc_hat_v;
    }

    // ── Step 7: Diameter estimate ─────────────────────────────────────
    res.diameter_estimate = *std::max_element(res.ecc_hat.begin(), res.ecc_hat.end());

    return res;
}
