// crescenzi_ifub.cpp
#include <bits/stdc++.h>
using namespace std;
using Clock = chrono::high_resolution_clock;

// ——— Graph definition and loader ———
using Edge = tuple<int,int,double>;
struct Graph {
    int n;
    vector<vector<Edge>> adj;
    bool weighted = false;
    Graph(int _n): n(_n), adj(n) {}
    void add_edge(int u,int v,double w=1.0){
        adj[u].emplace_back(u,v,w);
        adj[v].emplace_back(v,u,w);
        if(w!=1.0) weighted = true;
    }
};

Graph load_mm_graph(const string& path){
    ifstream in(path);
    if(!in) throw runtime_error("Cannot open "+path);
    string header; 
    getline(in, header);
    if(header.rfind("%%MatrixMarket",0)!=0)
        throw runtime_error("Not a MatrixMarket file");
    

    bool is_complex  = header.find("complex") != string::npos;

    bool is_weighted = header.find("real")!=string::npos
                    || header.find("integer")!=string::npos;
    string line;
    while(getline(in,line) && line.size() && line[0]=='%');
    istringstream iss(line);
    int M,N,L; iss>>M>>N>>L;
    if(M!=N) throw runtime_error("Only square graphs supported");
    Graph G(M);
    for(int i=0;i<L;i++){
        int u,v; double w=1.0;
        in>>u>>v;
        if(is_weighted) in>>w;

        else if(is_complex){
            double wr, wi;
            in >> wr >> wi;
            w = wr;
        }        
        G.add_edge(u-1,v-1,w);
    }
    return G;
}

// ——— BFS eccentricity computation ———
int eccentricity(const Graph& G, int src){
    vector<int> dist(G.n, INT_MAX);
    queue<int> q;
    dist[src] = 0; q.push(src);
    int ecc = 0;
    while(!q.empty()){
        int u = q.front(); q.pop();
        for(auto [_,v,_w]: G.adj[u]){
            if(dist[v] == INT_MAX){
                dist[v] = dist[u] + 1;
                ecc = max(ecc, dist[v]);
                q.push(v);
            }
        }
    }
    return ecc;
}

// ——— Root selection strategies ———
int random_root(int n) {
    return rand() % n;
}

int highest_degree(const Graph& G) {
    int max_deg = -1, best = 0;
    for(int i = 0; i < G.n; ++i){
        if((int)G.adj[i].size() > max_deg){
            max_deg = G.adj[i].size();
            best = i;
        }
    }
    return best;
}

int four_sweep(const Graph& G, int initial){
    auto bfs_far = [&](int src) {
        vector<int> dist(G.n, INT_MAX);
        queue<int> q; dist[src] = 0; q.push(src);
        int far = src;
        while(!q.empty()){
            int u = q.front(); q.pop();
            for(auto [_,v,_w]: G.adj[u]){
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
    int b2 = bfs_far(a2);
    return a2; // midpoint not needed for iFUB start
}

// ——— iFUB ———
int iFUB(const Graph& G, int root, int& bfsCount){
    vector<int> dist(G.n, INT_MAX);
    queue<int> q; dist[root] = 0; q.push(root);
    vector<vector<int>> levels(G.n);
    levels[0].push_back(root);
    int D = 0;
    while(!q.empty()){
        int u = q.front(); q.pop();
        for(auto [_,v,_w]: G.adj[u]){
            if(dist[v] == INT_MAX){
                dist[v] = dist[u] + 1;
                D = max(D, dist[v]);
                levels[dist[v]].push_back(v);
                q.push(v);
            }
        }
    }

    int lb = D, ub = 2*D; // initial bounds
    bfsCount = 1;

    for(int d = D; d >= (ub+1)/2; --d){
        for(int v : levels[d]){
            int ecc = eccentricity(G, v);
            lb = max(lb, ecc);
            ub = min(ub, 2 * ecc);
            bfsCount++;
            if(lb == ub) return lb;
        }
    }
    return lb; // exact if lb==ub
}

// ——— main() ———
int main(int argc, char* argv[]){
    if(argc < 3){
        cerr << "Usage: " << argv[0] << " [strategy: 0=random, 1=hd, 2=4s-rand, 3=4s-hd] graph.mtx\n";
        return 1;
    }
    int strategy = stoi(argv[1]);
    printf("start");
    Graph G = load_mm_graph(argv[2]);
    printf("DOne");
    srand(time(0));

    int start_node;
    if(strategy == 0) start_node = random_root(G.n);
    else if(strategy == 1) start_node = highest_degree(G);
    else if(strategy == 2) start_node = four_sweep(G, random_root(G.n));
    else if(strategy == 3) start_node = four_sweep(G, highest_degree(G));
    else { cerr << "Invalid strategy code\n"; return 1; }

    int bfsCalls = 0;
    auto t0 = Clock::now();
    int diam = iFUB(G, start_node, bfsCalls);
    auto t1 = Clock::now();

    double seconds = chrono::duration<double>(t1 - t0).count();

    // grab memory usage
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    // ru_maxrss is in kilobytes on Linux, bytes on macOS
    long peak_rss = usage.ru_maxrss;

    cout << "Strategy:" << strategy  << "\n"
         << "Diameter:" << diam  << "\n"
         << "BFS_Calls:" << bfsCalls  << "\n"
         << "Time(s):" << seconds << "\n"
         << "Peak_RSS:    " << peak_rss
                  << (sizeof(peak_rss)==sizeof(long)? " KB":" bytes")
         << "\n";

    return 0;
}
