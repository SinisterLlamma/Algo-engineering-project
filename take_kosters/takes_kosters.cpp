// main.cpp
#include <bits/stdc++.h>
#include <sys/resource.h>
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
    string header; getline(in, header);
    if(header.rfind("%%MatrixMarket",0)!=0)
        throw runtime_error("Not a MatrixMarket file");
    bool is_pattern  = header.find("pattern")!=string::npos;
    bool is_weighted = !is_pattern
                    && (header.find("real")   != string::npos
                     || header.find("integer")!= string::npos);
    // skip comments
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

// ——— Single‐source distances & eccentricity ———
int computeEccentricity(const Graph& G, int src, vector<int>& dist){
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
            else                dist[i]=int(ceil(d[i]));
        }
    }
    int ecc=0;
    for(int x: dist) if(x<INT_MAX) ecc = max(ecc,x);
    return ecc;
}

// ——— Selection strategies (§4.4) ———
enum Strategy { BOUND_DIFF=1, INTERCHANGE=2, REPEATED=3 };

int selectFrom(const vector<bool>& inW,
               const vector<int>& epsL,
               const vector<int>& epsU,
               const vector<int>& lastDist,
               const Graph& G,
               Strategy strat,
               int prevSelected)
{
    int best = -1;
    switch(strat){
      case BOUND_DIFF: {
         int64_t mx = -1;
         for(int w = 0; w < G.n; w++) if(inW[w]){
             int64_t diff = (int64_t)epsU[w] - (int64_t)epsL[w];
             if(diff > mx){
                 mx = diff;
                 best = w;
             }
         }
         break;
      }
      case INTERCHANGE: {
        static bool pickHigh = true;
        if(pickHigh){
          // largest epsU
          int mx = INT_MIN;
          for(int w=0; w<G.n; w++) if(inW[w]){
            if(epsU[w] > mx){
              mx = epsU[w]; best = w;
            }
          }
        } else {
          // smallest epsL
          int mn = INT_MAX;
          for(int w=0; w<G.n; w++) if(inW[w]){
            if(epsL[w] < mn){
              mn = epsL[w]; best = w;
            }
          }
        }
        pickHigh = !pickHigh;
        break;
      }
      case REPEATED: {
        if(prevSelected < 0){
          // initial: highest degree
          int mxdeg = -1;
          for(int w=0; w<G.n; w++) if(inW[w]){
            int deg = G.adj[w].size();
            if(deg > mxdeg){
              mxdeg = deg; best = w;
            }
          }
        } else {
          // farthest in lastDist
          int mx = -1;
          for(int w=0; w<G.n; w++) if(inW[w]){
            if(lastDist[w] > mx){
              mx = lastDist[w]; best = w;
            }
          }
        }
        break;
      }
    }
    return best;
}

// ——— Instrumented BoundingDiameters (Alg. 1 + §4.4) ———
struct RunStats {
    int    totalEcc       = 0;
    int    totalPruned    = 0;
    double totalTime      = 0.0;
    vector<array<double,4>> log; 
    // each entry: {iter, |W|, ΔL, ΔU}
};

RunStats boundingDiametersInstr(const Graph& G, Strategy strat){
    int n = G.n, Wsize = n, prev = -1;
    vector<bool> inW(n, true);
    vector<int> epsL(n, INT_MIN), epsU(n, INT_MAX), lastDist(n), dist;
    int64_t deltaL = 0, deltaU = INT_MAX;
    RunStats S;
    int iter = 0;

    while(deltaL < deltaU && Wsize > 0){
        int v = selectFrom(inW, epsL, epsU, lastDist, G, strat, prev);
        prev = v;
        auto t0 = Clock::now();
        int eccv = computeEccentricity(G, v, dist);
        auto t1 = Clock::now();
        double dt = chrono::duration<double>(t1 - t0).count();

        S.totalEcc++;
        S.totalTime += dt;
        lastDist = dist;

        // update global bounds
        deltaL = max<int64_t>(deltaL, eccv);
        deltaU = min<int64_t>(deltaU, 2 * (int64_t)eccv);

        // update per-node bounds and prune
        int before = Wsize;
        for(int w=0; w<n; w++) if(inW[w]){
            int dvw = dist[w];
            int low  = max(eccv - dvw, dvw);
            int high = eccv + dvw;
            epsL[w] = max(epsL[w], low);
            epsU[w] = min(epsU[w], high);
            if((epsU[w] <= deltaL && epsL[w] >= (deltaU+1)/2)
               || epsL[w] == epsU[w])
            {
                inW[w] = false;
                --Wsize;
            }
        }
        S.totalPruned += (before - Wsize);

        // log iteration state
        S.log.push_back({double(iter),
                         double(Wsize),
                         double(deltaL),
                         double(deltaU)});
        ++iter;
    }
    return S;
}

// ——— main() ———
int main(int argc, char* argv[]){
    if(argc < 4){
        cerr<<"Usage: "<<argv[0]
            <<" --strategy [1|2|3] graph.mtx\n";
        return 1;
    }
    Strategy strat = Strategy(stoi(argv[2]));
    Graph G = load_mm_graph(argv[3]);

    // compute |E| as sum of adjacency lengths
    int64_t edgeCount = 0;
    for(auto &nbr : G.adj) edgeCount += nbr.size();
    double avgDeg = double(edgeCount) / G.n;

    // Table 1 header
    cout<<"Dataset,|V|,|E|,AvgDeg,Strategy,"
          "EccCalls,PrunedNodes,TotalTime(s),Memory(KB)\n";

    // run the algorithm
    RunStats R = boundingDiametersInstr(G, strat);
    // get peak memory usage (in KB)
    struct rusage usage;
    getrusage(RUSAGE_SELF, & usage);
    long memKB = usage.ru_maxrss;

    // Table 1 row
    cout<<argv[3]<<","
        <<G.n<<","
        <<edgeCount<<","
        <<avgDeg<<","
        <<int(strat)<<","
        <<R.totalEcc<<","
        <<R.totalPruned<<","
        <<R.totalTime<<","
        <<memKB<<"\n\n";

    // Figure 2 header + data
    cout<<"# iter,|W|,DeltaL,DeltaU\n";
    for(auto &e : R.log){
        cout<<int(e[0])<<","
            <<int(e[1])<<","
            <<int(e[2])<<","
            <<int(e[3])<<"\n";
    }

    return 0;
}
