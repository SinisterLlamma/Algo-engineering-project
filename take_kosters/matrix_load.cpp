#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <tuple>
#include <stdexcept>

using Edge = std::tuple<int,int,double>;  // (u, v, weight)

struct Graph {
    int n;                              // number of vertices
    std::vector<std::vector<Edge>> adj; // adjacency list

    Graph(int _n): n(_n), adj(n) {}
    void add_edge(int u, int v, double w=1.0) {
        adj[u].emplace_back(u, v, w);
        adj[v].emplace_back(v, u, w);   // omit for directed graphs
    }
};

// Loads a symmetric or general graph in Matrix Market coordinate format.
// Assumes 1-based indices. Skips comment lines ('%').
Graph load_mm_graph(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Cannot open file: " + path);

    std::string line;
    // Read header
    std::getline(in, line);
    if (line.substr(0,14) != "%%MatrixMarket")
        throw std::runtime_error("Not a MatrixMarket file");

    bool is_weighted = false;
    if (line.find("real") != std::string::npos ||
        line.find("integer") != std::string::npos)
        is_weighted = true;

    // skip comments
    do { std::getline(in, line); } 
    while (line.size() && line[0]=='%');

    // read dimensions: M rows, N cols, L entries
    std::istringstream header(line);
    int M, N, L;
    header >> M >> N >> L;
    if (M != N) 
        throw std::runtime_error("Only square graphs supported");

    Graph G(M);

    // read edges
    for (int i = 0; i < L; i++) {
        int u, v;
        double w = 1.0;
        in >> u >> v;
        if (is_weighted) in >> w;
        // convert to 0-based
        G.add_edge(u-1, v-1, w);
    }
    return G;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " graph.mtx\n";
        return 1;
    }
    try {
        Graph G = load_mm_graph(argv[1]);
        std::cout << "Loaded graph with " << G.n << " vertices\n";
        size_t m = 0;
        for (auto &nbrs : G.adj) m += nbrs.size();
        std::cout << "Number of edges (directed count): " << m << "\n";

        // TODO: run your algorithms on G.adj
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}