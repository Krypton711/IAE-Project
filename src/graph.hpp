#pragma once

#include <vector>
#include <string>
#include <queue>
#include <limits>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <cassert>

/**
 * Simple undirected, unweighted graph using adjacency list.
 * Supports loading from edge-list files (SNAP format).
 * Node IDs are remapped to contiguous [0, n) range internally.
 */
class Graph {
public:
    int n = 0;                              // number of nodes
    long long m = 0;                        // number of edges
    std::vector<std::vector<int>> adj;      // adjacency list

    Graph() = default;

    /**
     * Load from an edge-list file.
     * Lines starting with '#' or '%' are comments.
     * Each data line: "u v" (whitespace-separated).
     * Self-loops are ignored. Duplicate edges are ignored.
     * Node IDs are remapped to contiguous 0-indexed range.
     */
    static Graph load(const std::string& path) {
        Graph g;
        std::ifstream fin(path);
        if (!fin.is_open()) {
            std::cerr << "Error: cannot open file " << path << "\n";
            return g;
        }

        std::unordered_map<int, int> id_map;
        std::vector<std::pair<int,int>> edges;
        std::string line;

        while (std::getline(fin, line)) {
            if (line.empty() || line[0] == '#' || line[0] == '%') continue;
            std::istringstream iss(line);
            int u, v;
            if (!(iss >> u >> v)) continue;
            if (u == v) continue; // skip self-loops

            // Remap IDs
            if (id_map.find(u) == id_map.end()) {
                int id = (int)id_map.size();
                id_map[u] = id;
            }
            if (id_map.find(v) == id_map.end()) {
                int id = (int)id_map.size();
                id_map[v] = id;
            }
            edges.push_back({id_map[u], id_map[v]});
        }

        g.n = (int)id_map.size();
        g.adj.resize(g.n);

        for (auto& [u, v] : edges) {
            g.adj[u].push_back(v);
            g.adj[v].push_back(u);
        }

        // Remove duplicate edges in adjacency lists
        for (int i = 0; i < g.n; i++) {
            std::sort(g.adj[i].begin(), g.adj[i].end());
            g.adj[i].erase(std::unique(g.adj[i].begin(), g.adj[i].end()), g.adj[i].end());
        }

        // Count edges
        g.m = 0;
        for (int i = 0; i < g.n; i++) g.m += g.adj[i].size();
        g.m /= 2;

        return g;
    }

    /**
     * Build a graph programmatically from an edge list.
     * Nodes must already be 0-indexed and contiguous.
     */
    static Graph from_edges(int num_nodes, const std::vector<std::pair<int,int>>& edges) {
        Graph g;
        g.n = num_nodes;
        g.adj.resize(num_nodes);
        for (auto& [u, v] : edges) {
            g.adj[u].push_back(v);
            g.adj[v].push_back(u);
        }
        g.m = (long long)edges.size();
        return g;
    }

    /**
     * BFS from source. Returns distance array.
     * Unreachable nodes have distance = -1.
     */
    std::vector<int> bfs(int source) const {
        std::vector<int> dist(n, -1);
        std::queue<int> q;
        dist[source] = 0;
        q.push(source);
        while (!q.empty()) {
            int u = q.front(); q.pop();
            for (int v : adj[u]) {
                if (dist[v] == -1) {
                    dist[v] = dist[u] + 1;
                    q.push(v);
                }
            }
        }
        return dist;
    }

    /**
     * Compute eccentricity of a single vertex (max distance via BFS).
     * Returns -1 if graph is disconnected from source.
     */
    int eccentricity(int source) const {
        auto dist = bfs(source);
        int ecc = 0;
        for (int i = 0; i < n; i++) {
            if (dist[i] == -1) return -1; // disconnected
            ecc = std::max(ecc, dist[i]);
        }
        return ecc;
    }

    /**
     * Get the largest connected component as a new Graph.
     */
    Graph largest_component() const {
        std::vector<int> comp(n, -1);
        int num_comp = 0;
        std::vector<int> comp_size;

        for (int i = 0; i < n; i++) {
            if (comp[i] != -1) continue;
            int cc = num_comp++;
            comp_size.push_back(0);
            std::queue<int> q;
            q.push(i);
            comp[i] = cc;
            while (!q.empty()) {
                int u = q.front(); q.pop();
                comp_size[cc]++;
                for (int v : adj[u]) {
                    if (comp[v] == -1) {
                        comp[v] = cc;
                        q.push(v);
                    }
                }
            }
        }

        int best_cc = (int)(std::max_element(comp_size.begin(), comp_size.end()) - comp_size.begin());

        // Remap nodes in best_cc to 0-indexed
        std::vector<int> remap(n, -1);
        int new_n = 0;
        for (int i = 0; i < n; i++) {
            if (comp[i] == best_cc) {
                remap[i] = new_n++;
            }
        }

        Graph g;
        g.n = new_n;
        g.adj.resize(new_n);
        for (int u = 0; u < n; u++) {
            if (comp[u] != best_cc) continue;
            for (int v : adj[u]) {
                if (comp[v] == best_cc) {
                    g.adj[remap[u]].push_back(remap[v]);
                }
            }
        }

        g.m = 0;
        for (int i = 0; i < g.n; i++) g.m += g.adj[i].size();
        g.m /= 2;

        return g;
    }
};
