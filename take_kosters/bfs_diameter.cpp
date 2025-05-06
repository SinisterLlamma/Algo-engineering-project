#include <bits/stdc++.h>
#include <sys/resource.h>
using namespace std;
using Clock = chrono::high_resolution_clock;

// Reuse Graph definition from takes_kosters.cpp
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

// Reuse graph loader from takes_kosters.cpp
Graph load_mm_graph(const string& path){
    ifstream in(path);
    if(!in) throw runtime_error("Cannot open "+path);
    string header; getline(in, header);
    if(header.rfind("%%MatrixMarket",0)!=0)
        throw runtime_error("Not a MatrixMarket file");
    bool is_pattern  = header.find("pattern")!=string::npos;
    bool is_weighted = !is_pattern
                    && (header.find("real")   != string::npos
                     || header.find("integer")!= string::npos);
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
        G.add_edge(u-1,v-1,w);
    }
    return G;
}

// Computes single-source distances and returns eccentricity
int computeEccentricity(const Graph& G, int src, vector<int>& dist) {
    dist.assign(G.n, INT_MAX);
    if(!G.weighted){
        queue<int> q;
        dist[src]=0; q.push(src);
        while(!q.empty()){
            int u=q.front(); q.pop();
            for(auto [_,v,w]: G.adj[u]){
                if(dist[v]==INT_MAX){
                    dist[v] = dist[u]+1;
                    q.push(v);
                }
            }
        }
    } else {
        using P = pair<double,int>;
        vector<double> d(G.n,1e30);
        priority_queue<P,vector<P>,greater<P>> pq;
        d[src]=0; pq.emplace(0,src);
        while(!pq.empty()){
            auto [du,u] = pq.top(); pq.pop();
            if(du> d[u]) continue;
            for(auto [_,v,w]: G.adj[u]){
                if(du + w < d[v]){
                    d[v] = du + w;
                    pq.emplace(d[v],v);
                }
            }
        }
        for(int i=0;i<G.n;i++){
            if(d[i]>=1e30) dist[i]=INT_MAX;
            else dist[i]=int(ceil(d[i]));
        }
    }
    int ecc = 0;
    for(int x: dist) if(x<INT_MAX) ecc = max(ecc,x);
    return ecc;
}

// Compute diameter by running BFS/Dijkstra from every vertex
int computeDiameter(const Graph& G) {
    vector<int> dist;
    int diameter = 0;
    auto start = Clock::now();
    
    for(int i = 0; i < G.n; i++) {
        int ecc = computeEccentricity(G, i, dist);
        diameter = max(diameter, ecc);
    }
    
    auto end = Clock::now();
    double time = chrono::duration<double>(end - start).count();
    
    cout << "Time taken: " << time << " seconds\n";
    return diameter;
}

int main(int argc, char* argv[]) {
    if(argc != 2) {
        cerr << "Usage: " << argv[0] << " graph.mtx\n";
        return 1;
    }

    Graph G = load_mm_graph(argv[1]);
    cout << "Computing diameter for graph with " << G.n << " vertices...\n";
    
    int diameter = computeDiameter(G);
    cout << "Diameter: " << diameter << "\n";

    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    cout << "Peak memory usage: " << usage.ru_maxrss << " KB\n";
    
    return 0;
}
