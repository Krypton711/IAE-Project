#include "graph.hpp"
#include "naive.hpp"
#include "kbfs.hpp"
#include "rv.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <string>
#include <vector>
#include <cstring>

namespace fs = std::filesystem;

// ─── Timing helper ──────────────────────────────────────────────────
struct Timer {
    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point start;
    void begin() { start = Clock::now(); }
    double elapsed_ms() const {
        return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
    }
};

// ─── CSV writer ─────────────────────────────────────────────────────
struct CSVWriter {
    std::ofstream out;
    bool header_written = false;

    CSVWriter(const std::string& path) : out(path) {}

    void write_header() {
        out << "graph,nodes,edges,algorithm,parameter,strategy,"
            << "estimated_diameter,true_diameter,approx_ratio,"
            << "num_bfs,time_ms\n";
        header_written = true;
    }

    void write_row(const std::string& graph_name, int n, long long m,
                   const std::string& algo, const std::string& param,
                   const std::string& strategy,
                   int est_diam, int true_diam, int num_bfs, double time_ms) {
        double ratio = (true_diam > 0) ? (double)est_diam / true_diam : -1.0;
        out << graph_name << "," << n << "," << m << ","
            << algo << "," << param << "," << strategy << ","
            << est_diam << "," << true_diam << ","
            << std::fixed << std::setprecision(4) << ratio << ","
            << num_bfs << ","
            << std::fixed << std::setprecision(2) << time_ms << "\n";
        out.flush();
    }
};

// ─── Run all algorithms on a single graph ───────────────────────────
void benchmark_graph(const std::string& path, CSVWriter& csv, bool run_naive, bool verbose) {
    std::string name = fs::path(path).stem().string();

    if (verbose) std::cout << "\n=== Loading graph: " << name << " ===\n";

    Graph g_full = Graph::load(path);
    if (g_full.n == 0) {
        std::cerr << "  Skipping (empty or invalid)\n";
        return;
    }

    // Use largest connected component
    Graph g = g_full.largest_component();

    if (verbose) {
        std::cout << "  Nodes: " << g.n << "  Edges: " << g.m;
        if (g.n != g_full.n)
            std::cout << "  (LCC of " << g_full.n << " nodes)";
        std::cout << "\n";
    }

    Timer timer;
    int true_diam = -1;

    // ── Naive (exact) ───────────────────────────────────────────────
    if (run_naive) {
        if (verbose) std::cout << "  Running Naive (all-pairs BFS)...\n";
        timer.begin();
        auto naive_res = naive_diameter(g);
        double t = timer.elapsed_ms();
        true_diam = naive_res.diameter;
        if (verbose) std::cout << "    Diameter = " << true_diam
                               << "  BFS = " << naive_res.num_bfs
                               << "  Time = " << t << " ms\n";
        csv.write_row(name, g.n, g.m, "Naive", "n", "-",
                      true_diam, true_diam, naive_res.num_bfs, t);
    }

    // ── k-BFS (two-phase: S random → S' = k farthest from S) ──────────
    // As per paper: Phase 1 picks k random sources S, Phase 2 picks S' = k vertices
    // with largest d(v,S). ê(v) = max(d(v,S), d(v,S')). No strategy enum needed.
    std::vector<int> k_values = {1, 3, 5, 10, 20};

    for (int k : k_values) {
        if (k > g.n) continue;
        if (verbose) std::cout << "  Running k-BFS (k=" << k << ")...\n";
        timer.begin();
        auto kbfs_res = kbfs_diameter(g, k);
        double t = timer.elapsed_ms();
        if (verbose) std::cout << "    Diameter estimate = " << kbfs_res.diameter_estimate
                               << "  BFS = " << kbfs_res.num_bfs
                               << "  Time = " << t << " ms\n";
        csv.write_row(name, g.n, g.m, "k-BFS",
                      std::to_string(k), "-",
                      kbfs_res.diameter_estimate, true_diam,
                      kbfs_res.num_bfs, t);
    }

    // ── RV algorithm ────────────────────────────────────────────────
    {
        if (verbose) std::cout << "  Running RV (sqrt(n) sample)...\n";
        timer.begin();
        auto rv_res = rv_diameter(g);
        double t = timer.elapsed_ms();
        if (verbose) std::cout << "    Diameter estimate = " << rv_res.diameter_estimate
                               << "  BFS = " << rv_res.num_bfs
                               << "  Time = " << t << " ms\n";
        csv.write_row(name, g.n, g.m, "RV",
                      std::to_string((int)std::ceil(std::sqrt(g.n))), "-",
                      rv_res.diameter_estimate, true_diam,
                      rv_res.num_bfs, t);
    }
}

// ─── Verification mode: test on known small graphs ──────────────────
bool run_verification() {
    std::cout << "=== VERIFICATION MODE ===\n\n";
    bool all_ok = true;

    auto check = [&](const std::string& name, const Graph& g, int expected_diam) {
        std::cout << "Testing " << name << " (n=" << g.n << ", m=" << g.m << ")...\n";

        auto naive_res = naive_diameter(g);
        if (naive_res.diameter != expected_diam) {
            std::cout << "  FAIL: Naive diameter = " << naive_res.diameter
                      << " expected " << expected_diam << "\n";
            all_ok = false;
            return;
        }
        std::cout << "  Naive: OK (diameter = " << naive_res.diameter << ")\n";

        // k-BFS: estimate should be <= true diameter (it's a lower-bound estimator)
        // and >= 1 (or > 0 for any non-trivial graph)
        for (int k : {1, 3, 5}) {
            if (k > g.n) continue;
            auto kbfs_res = kbfs_diameter(g, k);
            if (kbfs_res.diameter_estimate > expected_diam) {
                std::cout << "  FAIL: k-BFS(k=" << k << ") estimate "
                          << kbfs_res.diameter_estimate << " > true " << expected_diam << "\n";
                all_ok = false;
            }
            std::cout << "  k-BFS(k=" << k << "): estimate=" << kbfs_res.diameter_estimate
                      << " (true=" << expected_diam << ") OK\n";
        }

        // RV: estimate >= ceil(2D/3)
        auto rv_res = rv_diameter(g);
        int min_expected = (2 * expected_diam + 2) / 3; // ceil(2D/3)
        if (rv_res.diameter_estimate < min_expected) {
            std::cout << "  FAIL: RV estimate " << rv_res.diameter_estimate
                      << " < ceil(2*" << expected_diam << "/3) = " << min_expected << "\n";
            all_ok = false;
        }
        if (rv_res.diameter_estimate > expected_diam) {
            std::cout << "  FAIL: RV estimate " << rv_res.diameter_estimate
                      << " > true diameter " << expected_diam << "\n";
            all_ok = false;
        }
        std::cout << "  RV: estimate=" << rv_res.diameter_estimate
                  << " (expected in [" << min_expected << ", " << expected_diam << "]) OK\n";
    };

    // Path graph: 0-1-2-3-4  (diameter = 4)
    check("Path(5)", Graph::from_edges(5, {{0,1},{1,2},{2,3},{3,4}}), 4);

    // Cycle: 0-1-2-3-4-0  (diameter = 2)
    check("Cycle(5)", Graph::from_edges(5, {{0,1},{1,2},{2,3},{3,4},{4,0}}), 2);

    // Star: center=0, leaves=1,2,3,4 (diameter = 2)
    check("Star(5)", Graph::from_edges(5, {{0,1},{0,2},{0,3},{0,4}}), 2);

    // Complete K5 (diameter = 1)
    check("K5", Graph::from_edges(5, {{0,1},{0,2},{0,3},{0,4},{1,2},{1,3},{1,4},{2,3},{2,4},{3,4}}), 1);

    // Binary tree depth 3: 7 nodes, diameter = 4
    //       0
    //      / \
    //     1   2
    //    / \ / \
    //   3  4 5  6
    check("BinTree(7)", Graph::from_edges(7, {{0,1},{0,2},{1,3},{1,4},{2,5},{2,6}}), 4);

    // Larger path (10 nodes, diameter = 9)
    {
        std::vector<std::pair<int,int>> edges;
        for (int i = 0; i < 9; i++) edges.push_back({i, i+1});
        check("Path(10)", Graph::from_edges(10, edges), 9);
    }

    std::cout << "\n" << (all_ok ? "ALL TESTS PASSED" : "SOME TESTS FAILED") << "\n";
    return all_ok;
}

// ─── Main ───────────────────────────────────────────────────────────
void print_usage(const char* prog) {
    std::cout << "Usage:\n"
              << "  " << prog << " --verify                 Run correctness tests\n"
              << "  " << prog << " [options] <files/dirs>   Benchmark on graph files\n"
              << "\nOptions:\n"
              << "  --no-naive         Skip exact (naive) computation\n"
              << "  --naive-limit N    Only run naive if nodes <= N (default: 50000)\n"
              << "  --output FILE      Output CSV file (default: results/benchmark.csv)\n"
              << "  --quiet            Suppress verbose output\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    // Parse arguments
    bool verify = false;
    bool verbose = true;
    bool force_no_naive = false;
    int naive_limit = 50000;
    std::string output_path = "results/benchmark.csv";
    std::vector<std::string> inputs;

    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--verify") == 0) {
            verify = true;
        } else if (std::strcmp(argv[i], "--no-naive") == 0) {
            force_no_naive = true;
        } else if (std::strcmp(argv[i], "--naive-limit") == 0 && i + 1 < argc) {
            naive_limit = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output_path = argv[++i];
        } else if (std::strcmp(argv[i], "--quiet") == 0) {
            verbose = false;
        } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            inputs.push_back(argv[i]);
        }
    }

    if (verify) {
        return run_verification() ? 0 : 1;
    }

    if (inputs.empty()) {
        std::cerr << "Error: no input files or directories specified.\n";
        print_usage(argv[0]);
        return 1;
    }

    // Collect all graph files
    std::vector<std::string> graph_files;
    for (auto& input : inputs) {
        fs::path p(input);
        if (fs::is_directory(p)) {
            for (auto& entry : fs::directory_iterator(p)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    if (ext == ".txt" || ext == ".edges" || ext == ".csv" || ext == ".tsv" || ext == ".el") {
                        graph_files.push_back(entry.path().string());
                    }
                }
            }
        } else if (fs::is_regular_file(p)) {
            graph_files.push_back(p.string());
        } else {
            std::cerr << "Warning: skipping " << input << " (not a file or directory)\n";
        }
    }

    std::sort(graph_files.begin(), graph_files.end());

    if (graph_files.empty()) {
        std::cerr << "Error: no graph files found.\n";
        return 1;
    }

    // Ensure output directory exists
    fs::create_directories(fs::path(output_path).parent_path());

    CSVWriter csv(output_path);
    csv.write_header();

    if (verbose) {
        std::cout << "Found " << graph_files.size() << " graph file(s)\n"
                  << "Output: " << output_path << "\n"
                  << "Naive limit: " << (force_no_naive ? "disabled" : std::to_string(naive_limit)) << " nodes\n";
    }

    for (auto& gf : graph_files) {
        // Determine if we should run naive
        // Quick peek at graph size
        Graph peek = Graph::load(gf);
        Graph lcc = peek.largest_component();
        bool do_naive = !force_no_naive && (lcc.n <= naive_limit);

        benchmark_graph(gf, csv, do_naive, verbose);
    }

    if (verbose) std::cout << "\nDone. Results written to " << output_path << "\n";
    return 0;
}
