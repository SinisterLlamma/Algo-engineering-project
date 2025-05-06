#include <bits/stdc++.h>
#include <sys/resource.h>
using namespace std;
using Clock = chrono::high_resolution_clock;
using Edge  = tuple<int,int,double>;

struct Graph {
    int n;
    vector<vector<Edge>> adj;
    Graph(int _n): n(_n), adj(n){}
    void add_edge(int u,int v,double w=1){
        adj[u].emplace_back(u,v,w);
        adj[v].emplace_back(v,u,w);
    }
};

Graph load_mm(const string& p){
    ifstream in(p);
    string h; getline(in,h);
    bool w = h.find("real")!=string::npos||h.find("integer")!=string::npos;
    string line;
    while(getline(in,line) && line[0]=='%');
    int M,N,L; istringstream(line)>>M>>N>>L;
    Graph G(M);
    for(int i=0;i<L;i++){
        int u,v; double ww=1;
        in>>u>>v;
        if(w) in>>ww;
        G.add_edge(u-1,v-1,ww);
    }
    return G;
}

int eccentricity(const Graph& G,int s){
    vector<int>d(G.n,INT_MAX);
    queue<int>q; d[s]=0; q.push(s);
    int ecc=0;
    while(!q.empty()){
        int u=q.front();q.pop();
        for(auto& [_,v,_w]:G.adj[u]) if(d[v]==INT_MAX){
            d[v]=d[u]+1;
            ecc=max(ecc,d[v]);
            q.push(v);
        }
    }
    return ecc;
}

int random_root(int n){ return rand()%n; }
int highest_degree(const Graph& G){
    int b=0,m=-1;
    for(int i=0;i<G.n;i++) if((int)G.adj[i].size()>m){
        m=G.adj[i].size(); b=i;
    }
    return b;
}
int four_sweep(const Graph& G,int r){
    auto far=[&](int src){
        vector<int>d(G.n,INT_MAX);
        queue<int>q; d[src]=0; q.push(src);
        int f=src;
        while(!q.empty()){
            int u=q.front();q.pop();
            for(auto& [_,v,_w]:G.adj[u]) if(d[v]==INT_MAX){
                d[v]=d[u]+1;
                if(d[v]>d[f]) f=v;
                q.push(v);
            }
        }
        return f;
    };
    int a1=far(r), b1=far(a1), a2=far(b1);
    return a2;
}

int iFUB(const Graph& G,int root,int& bfsCount){
    vector<int>d(G.n,INT_MAX);
    queue<int>q; d[root]=0; q.push(root);
    vector<vector<int>>lvl(G.n);
    lvl[0].push_back(root);
    int D=0;
    while(!q.empty()){
        int u=q.front();q.pop();
        for(auto& [_,v,_w]:G.adj[u]) if(d[v]==INT_MAX){
            d[v]=d[u]+1;
            D = max(D,d[v]);
            lvl[d[v]].push_back(v);
            q.push(v);
        }
    }
    int lb=D, ub=2*D; bfsCount=1;
    for(int dd=D; dd>= (ub+1)/2; --dd){
        for(int v:lvl[dd]){
            int e=eccentricity(G,v);
            lb=max(lb,e);
            ub=min(ub,2*e);
            bfsCount++;
            if(lb==ub) return lb;
        }
    }
    return lb;
}

long peak_kb(){
    struct rusage u; getrusage(RUSAGE_SELF,&u);
    return u.ru_maxrss; 
}

int main(int argc,char**argv){
    if(argc!=2){
        cerr<<"Usage: "<<argv[0]<<" graph.mtx\n"; return 1;
    }
    Graph G = load_mm(argv[1]);
    srand(0);
    cout<<"strategy,bfs_calls\n";
    for(int s=0;s<4;s++){
        int root;
        switch(s){
            case 0: root=random_root(G.n); break;
            case 1: root=highest_degree(G); break;
            case 2: root=four_sweep(G,random_root(G.n)); break;
            default:root=four_sweep(G,highest_degree(G));break;
        }
        int bfsC=0;
        iFUB(G,root,bfsC);
        cout<<s<<","<<bfsC<<"\n";
    }
    return 0;
}