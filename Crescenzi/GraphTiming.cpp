#include <bits/stdc++.h>
#include <filesystem>
using namespace std;
namespace fs = std::filesystem;
using Clock = chrono::high_resolution_clock;

using Edge = tuple<int,int,double>;

struct Graph {
    int n;
    vector<vector<Edge>> adj;
    bool weighted = false;
    Graph(int _n): n(_n), adj(n) {}
    void add_edge(int u,int v,double w=1.0){
        adj[u].emplace_back(u,v,w);
        adj[v].emplace_back(v,u,w);
        if(w != 1.0) weighted = true;
    }
};

Graph load_mm_graph(const string& path){
    ifstream in(path);
    if(!in) throw runtime_error("Cannot open " + path);
    string header; getline(in, header);
    if(header.rfind("%%MatrixMarket", 0) != 0)
        throw runtime_error("Not a MatrixMarket file");
    
    bool is_complex  = header.find("complex") != string::npos;

    bool is_weighted = header.find("real") != string::npos
                    || header.find("integer") != string::npos;

    string line;
    while(getline(in,line) && line.size() && line[0] == '%');
    istringstream iss(line);
    int M, N, L; iss >> M >> N >> L;
    if(M != N) throw runtime_error("Only square graphs supported");
    Graph G(M);
    for(int i = 0; i < L; i++){
        int u, v; double w = 1.0;
        in >> u >> v;
        if(is_weighted) in >> w;
        else if(is_complex){
            double wr, wi;
            in >> wr >> wi;
            w = wr;
        } 
        G.add_edge(u-1, v-1, w);
    }
    return G;
}

int eccentricity(const Graph& G, int src){
    vector<int> dist(G.n, INT_MAX);
    queue<int> q;
    dist[src] = 0; q.push(src);
    int ecc = 0;
    while(!q.empty()){
        int u = q.front(); q.pop();
        for(auto [_, v, _w]: G.adj[u]){
            if(dist[v] == INT_MAX){
                dist[v] = dist[u] + 1;
                ecc = max(ecc, dist[v]);
                q.push(v);
            }
        }
    }
    return ecc;
}

int random_root(int n) {
    return rand() % n;
}

int highest_degree(const Graph& G) {
    int max_deg = -1, best = 0;
    for(int i = 0; i < G.n; ++i){
        int deg = G.adj[i].size();
        if(deg > max_deg){
            max_deg = deg;
            best = i;
        }
    }
    return best;
}

int four_sweep(const Graph& G, int initial){
    auto bfs_far = [&](int src){
        vector<int> dist(G.n, INT_MAX);
        queue<int> q; dist[src] = 0; q.push(src);
        int far = src;
        while(!q.empty()){
            int u = q.front(); q.pop();
            for(auto [_, v, _w]: G.adj[u]){
                if(dist[v] == INT_MAX){
                    dist[v] = dist[u] + 1;
                    if(dist[v] > dist[far]) far = v;
                    q.push(v);
                }
            }
        }
        return far;
    };
    int a1 = bfs_far(initial);
    int b1 = bfs_far(a1);
    int a2 = bfs_far(b1);
    return a2;
}

int iFUB(const Graph& G, int root, int& bfsCount){
    vector<int> dist(G.n, INT_MAX);
    queue<int> q; dist[root] = 0; q.push(root);
    vector<vector<int>> levels(G.n);
    levels[0].push_back(root);
    int D = 0;
    while(!q.empty()){
        int u = q.front(); q.pop();
        for(auto [_, v, _w]: G.adj[u]){
            if(dist[v] == INT_MAX){
                dist[v] = dist[u] + 1;
                D = max(D, dist[v]);
                if (dist[v] < G.n)
                    levels[dist[v]].push_back(v);
                q.push(v);
            }
        }
    }

    int lb = D, ub = 2 * D;
    bfsCount = 1;

    for(int d = D; d >= (ub+1)/2; --d){
        if (d >= (int)levels.size()) continue;
        for(int v : levels[d]){
            int ecc = eccentricity(G, v);
            lb = max(lb, ecc);
            ub = min(ub, 2 * ecc);
            bfsCount++;
            if(lb == ub) return lb;
        }
    }
    return lb;

}

int main(int argc, char* argv[]){
    srand(42);
    if(argc!=2){
        cerr<<"Usage: "<<argv[0]<<" folder_path\n";
        return 1;
    }
    string folder = argv[1];
    cout<<"strategy,avg_time_s\n";

    for(int strategy=0; strategy<4; ++strategy){
        double totalTime = 0;
        int fileCount = 0;

        for(const auto& entry: fs::directory_iterator(folder)){
            if(entry.path().extension()==".mtx"){
                try{
                    Graph G = load_mm_graph(entry.path());
                    int start;
                    switch(strategy){
                      case 0: start = random_root(G.n); break;
                      case 1: start = highest_degree(G); break;
                      case 2: start = four_sweep(G, random_root(G.n)); break;
                      case 3: start = four_sweep(G, highest_degree(G)); break;
                    }
                    int bfsCalls;
                    auto t0 = Clock::now();
                    iFUB(G, start, bfsCalls);
                    auto t1 = Clock::now();
                    totalTime += chrono::duration<double>(t1-t0).count();
                    ++fileCount;
                } catch(...){ /* skip bad */ }
            }
        }

        double avg = fileCount ? totalTime/fileCount : 0;
        cout<<strategy<<","<<avg<<"\n";
    }
    return 0;
}