// main.cpp
#include <bits/stdc++.h>
#include <sys/resource.h> // For getrusage
#include <chrono>         // For chrono::high_resolution_clock
#include <iomanip>        // For std::fixed and std::setprecision

using namespace std;
using Clock = chrono::high_resolution_clock;

// --- Graph definition and loader ---
struct Graph {
    int n;
    vector<vector<pair<int, double>>> adj; // Stores {neighbor, weight}
    bool weighted = false;

    Graph(int _n) : n(_n), adj(n) {}

    void add_edge(int u, int v, double w = 1.0) {
        adj[u].emplace_back(v, w);
        adj[v].emplace_back(u, w); // For undirected graph
        if (w != 1.0) weighted = true;
    }
};

Graph load_mm_graph(const string& path) {
    ifstream in(path);
    if (!in) throw runtime_error("Cannot open " + path);
    string header;
    getline(in, header);
    if (header.rfind("%%MatrixMarket", 0) != 0)
        throw runtime_error("Not a MatrixMarket file");
    bool is_pattern = header.find("pattern") != string::npos;
    bool is_weighted_file = !is_pattern &&
                           (header.find("real") != string::npos ||
                            header.find("integer") != string::npos);
    // skip comments
    string line;
    while (getline(in, line) && line.size() && line[0] == '%');
    istringstream iss(line);
    int M, N_nodes, L_links; 
    iss >> M >> N_nodes >> L_links;
    if (M != N_nodes) throw runtime_error("Only square graphs supported (M==N)");
    Graph G(N_nodes); 
    for (int i = 0; i < L_links; i++) {
        int u, v;
        double w = 1.0;
        in >> u >> v;
        if (is_weighted_file) in >> w;
        G.add_edge(u - 1, v - 1, w); 
    }
    return G;
}

// --- Single‐source distances & eccentricity ---
int computeEccentricity(const Graph& G, int src, vector<int>& dist_vector) {
    dist_vector.assign(G.n, INT_MAX);

    if (src < 0 || src >= G.n) { // Basic sanity check
        if (G.n == 0) return 0; // Or handle as error
        // Attempt to find a valid source if the provided one is bad and graph not empty
        // This situation should ideally be prevented by selectFrom
        bool found_valid_src = false;
        for(int i=0; i<G.n; ++i) if (!G.adj[i].empty() || G.n==1 ) {src = i; found_valid_src=true; break;}
        if(!found_valid_src && G.n > 0) src = 0; // Fallback, though graph might be all isolated nodes
        else if (!found_valid_src && G.n == 0) return 0;
    }


    if (!G.weighted) { 
        queue<int> q; 
        if (G.n > 0) { // Ensure src is valid before use
             dist_vector[src] = 0;
             q.push(src);
        } else {
            return 0; // No eccentricity for empty graph
        }
        
        while(!q.empty()){
            int u = q.front();
            q.pop();
            for (const auto& edge_pair : G.adj[u]) {
                int v = edge_pair.first;
                if (dist_vector[v] == INT_MAX) {
                    dist_vector[v] = dist_vector[u] + 1;
                    q.push(v);
                }
            }
        }
    } else { 
        using P = pair<double, int>; 
        vector<double> d_double(G.n, numeric_limits<double>::infinity());
        priority_queue<P, vector<P>, greater<P>> pq;

        if (G.n > 0) {
            d_double[src] = 0;
            pq.emplace(0.0, src);
        } else {
            return 0;
        }

        while (!pq.empty()) {
            auto [du, u] = pq.top();
            pq.pop();

            if (du > d_double[u] + 1e-9) continue; 

            for (const auto& edge_pair : G.adj[u]) {
                int v = edge_pair.first;
                double w = edge_pair.second;
                if (d_double[u] != numeric_limits<double>::infinity() && d_double[u] + w < d_double[v] - 1e-9) {
                    d_double[v] = d_double[u] + w;
                    pq.emplace(d_double[v], v);
                }
            }
        }
        for (int i = 0; i < G.n; i++) {
            if (d_double[i] >= numeric_limits<double>::infinity() / 2.0) { 
                dist_vector[i] = INT_MAX;
            } else {
                dist_vector[i] = int(ceil(d_double[i]));
            }
        }
    }

    int ecc = 0;
    for (int x : dist_vector) {
        if (x != INT_MAX) { 
            ecc = max(ecc, x);
        }
    }
    return ecc;
}

// --- Selection strategies (§4.4) ---
enum Strategy { BOUND_DIFF = 1, INTERCHANGE = 2, REPEATED = 3 };

int selectFrom(const vector<bool>& inW,
               const vector<long long>& epsL, 
               const vector<long long>& epsU, 
               const vector<int>& lastDist, 
               const Graph& G,
               Strategy strat,
               int prevSelectedNode) { 
    int best_node = -1;
    
    if (strat == Strategy::BOUND_DIFF) {
        long long max_diff = -1; // Using -1 to ensure any valid diff is greater
        for (int w = 0; w < G.n; w++) {
            if (inW[w]) {
                long long current_diff;
                // Prioritize nodes whose bounds haven't been updated much from INF
                if (epsU[w] == LLONG_MAX && epsL[w] == LLONG_MIN) current_diff = LLONG_MAX;
                else if (epsU[w] == LLONG_MAX) current_diff = LLONG_MAX -1; // Slightly less than fully uninitialized
                else if (epsL[w] == LLONG_MIN) current_diff = LLONG_MAX -2;
                else current_diff = epsU[w] - epsL[w];


                if (current_diff > max_diff) {
                    max_diff = current_diff;
                    best_node = w;
                } else if (current_diff == max_diff && current_diff != -1) { // Check current_diff != -1 to avoid issues if all diffs are <0
                    if (best_node == -1 || G.adj[w].size() > G.adj[best_node].size()) {
                        best_node = w; 
                    }
                }
            }
        }
    } else if (strat == Strategy::INTERCHANGE) {
        static bool pick_high_epsU = true; 
        if (pick_high_epsU) { 
            long long current_max_epsU = LLONG_MIN;
            for (int w = 0; w < G.n; w++) {
                if (inW[w]) {
                    if (epsU[w] > current_max_epsU) {
                        current_max_epsU = epsU[w];
                        best_node = w;
                    } else if (epsU[w] == current_max_epsU) {
                         if (best_node == -1 || G.adj[w].size() > G.adj[best_node].size()) {
                            best_node = w; 
                        }
                    }
                }
            }
        } else { 
            long long current_min_epsL = LLONG_MAX;
            for (int w = 0; w < G.n; w++) {
                if (inW[w]) {
                    if (epsL[w] < current_min_epsL) {
                        current_min_epsL = epsL[w];
                        best_node = w;
                    } else if (epsL[w] == current_min_epsL) {
                        if (best_node == -1 || G.adj[w].size() > G.adj[best_node].size()) {
                            best_node = w; 
                        }
                    }
                }
            }
        }
        pick_high_epsU = !pick_high_epsU;
    } else if (strat == Strategy::REPEATED) {
        if (prevSelectedNode < 0) { 
            int max_degree = -1;
            for (int w = 0; w < G.n; w++) {
                if (inW[w]) {
                    if ((int)G.adj[w].size() > max_degree) {
                        max_degree = G.adj[w].size();
                        best_node = w;
                    }
                }
            }
        } else { 
            int max_dist_val = -1; 
            for (int w = 0; w < G.n; w++) {
                if (inW[w]) {
                    if (lastDist[w] != INT_MAX && lastDist[w] > max_dist_val) {
                        max_dist_val = lastDist[w];
                        best_node = w;
                    } else if (lastDist[w] != INT_MAX && lastDist[w] == max_dist_val) {
                         if (best_node == -1 || G.adj[w].size() > G.adj[best_node].size()) { 
                            best_node = w; 
                        }
                    }
                }
            }
        }
    }
    
    if (best_node == -1) { 
        for (int w = 0; w < G.n; ++w) {
            if (inW[w]) {
                best_node = w; 
                break;
            }
        }
    }
    return best_node;
}


// --- Instrumented BoundingDiameters (Alg. 1 + §4.4) ---
struct RunStats {
    int totalEcc = 0;
    long long totalPruned = 0; 
    double totalTime = 0.0;
    vector<array<double, 4>> log;
};

RunStats boundingDiametersInstr(const Graph& G, Strategy strat) {
    int n = G.n;
    RunStats S; // Initialize S here so it can be returned even if n=0

    if (n == 0) return S; 

    long long current_Wsize = n; 
    int prev_selected_node = -1;
    vector<bool> inW(n, true);
    vector<long long> epsL(n, LLONG_MIN); 
    vector<long long> epsU(n, LLONG_MAX); 
    vector<int> distances_from_v(n); 

    long long deltaL = 0; 
    long long deltaU = LLONG_MAX;
    
    int iter = 0;

    while (deltaL < deltaU && current_Wsize > 0) {
        int v_selected = selectFrom(inW, epsL, epsU, distances_from_v, G, strat, prev_selected_node);
        
        if (v_selected == -1) { // No selectable node found in W (W might be empty or all nodes filtered unexpectedly)
            break; 
        }

        prev_selected_node = v_selected;

        auto t0 = Clock::now();
        int ecc_v = computeEccentricity(G, v_selected, distances_from_v);
        auto t1 = Clock::now();
        double dt = chrono::duration<double>(t1 - t0).count();

        S.totalEcc++;
        S.totalTime += dt;
        
        deltaL = max(deltaL, (long long)ecc_v);
        // Ensure 2LL * ecc_v does not cause issues if ecc_v is negative (not expected for graph eccentricity)
        if (ecc_v >= 0) { // Standard update
           deltaU = min(deltaU, 2LL * ecc_v);
        } else { // Should not happen with graph eccentricity
           // Potentially log an error or handle ecc_v < 0 case if it's possible by computeEccentricity
        }

        
        long long pruned_this_iteration = 0;
        for (int w = 0; w < n; w++) {
            if (inW[w]) { 
                long long d_v_w = distances_from_v[w]; 

                if (d_v_w != INT_MAX) { 
                    long long ecc_v_ll = ecc_v;
                    epsL[w] = max(epsL[w], max(ecc_v_ll - d_v_w, d_v_w));
                    epsU[w] = min(epsU[w], ecc_v_ll + d_v_w);
                }
                
                bool condition1 = (epsU[w] <= deltaL && epsL[w] >= deltaU / 2); 
                bool condition2 = (epsL[w] == epsU[w]);

                if (condition1 || condition2) {
                   inW[w] = false; 
                   pruned_this_iteration++;
                }
            }
        }
        current_Wsize -= pruned_this_iteration;
        S.totalPruned += pruned_this_iteration;
        
        S.log.push_back({(double)iter, (double)current_Wsize, (double)deltaL, (double)deltaU});
        iter++;
    }
    
    // Ensure log has at least one entry if graph is not empty and loop didn't run (e.g. n=1)
    // This captures the state after initial setup or the first (and only) processing step for n=1
    if (n > 0 && S.log.empty()) { 
        S.log.push_back({(double)0, (double)current_Wsize, (double)deltaL, (double)deltaU});
    }
    return S;
}


// --- main() ---
int main(int argc, char* argv[]) {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    if (argc < 4) {
        cerr << "Usage: " << argv[0]
             << " --strategy [1|2|3] graph.mtx\n";
        return 1;
    }
    Strategy strat = Strategy(stoi(argv[2]));
    string graph_path = argv[3];
    Graph G = load_mm_graph(graph_path);

    long long sum_degrees = 0;
    for (int i = 0; i < G.n; ++i) {
        sum_degrees += G.adj[i].size();
    }
    long long num_links = sum_degrees / 2; 
    double avg_deg = (G.n > 0) ? (double)sum_degrees / G.n : 0.0;

    cout << "Dataset,|V|,|E|,AvgDeg,Strategy,"
         << "EccCalls,PrunedNodes,TotalTime(s),Memory(KB)\n";

    RunStats R = boundingDiametersInstr(G, strat);

    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    long memKB = usage.ru_maxrss; 
    #if defined(__APPLE__) || defined(__MACH__)
        memKB /= 1024; 
    #endif


    cout << graph_path << ","
         << G.n << ","
         << num_links << "," 
         << fixed << setprecision(2) << avg_deg << "," 
         << int(strat) << ","
         << R.totalEcc << ","
         << R.totalPruned << ","
         << fixed << setprecision(3) << R.totalTime << "," 
         << memKB << "\n\n";

    cout << "# iter,|W|,DeltaL,DeltaU\n";
    cout << fixed << setprecision(0); 
    for (const auto& entry : R.log) {
        cout << (long long)entry[0] << ","
             << (long long)entry[1] << "," 
             << (long long)entry[2] << "," 
             << (long long)entry[3] << "\n"; 
    }

    // Corrected final output logic:
    if (G.n == 0) {
        cout << "\nGraph is empty. No diameter.\n";
    } else { // G.n > 0
        // R.log is guaranteed by boundingDiametersInstr to be non-empty if G.n > 0.
        if (R.log.empty()) {
            // This block should ideally not be reached.
            cout << "\nError: Log is empty for a non-empty graph. State is indeterminate.\n";
            if (G.n == 1) { // Specific hint for n=1, diameter is 0
                 cout << "For a single node graph, the diameter is 0.\n";
            }
        } else {
            long long finalDeltaL_from_log = static_cast<long long>(R.log.back()[2]);
            long long finalDeltaU_from_log = static_cast<long long>(R.log.back()[3]);
            if (finalDeltaL_from_log == finalDeltaU_from_log) {
                cout << "\nFinal Diameter: " << finalDeltaL_from_log << "\n";
            } else {
                cout << "\nDiameter bounds: [" << finalDeltaL_from_log << ", " << finalDeltaU_from_log << "]\n";
            }
        }
    }

    return 0;
}