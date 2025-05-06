# Report

Eshaan Sharma, Aditya Daseri 

---

## 1. Problem Statement

In this project, I address the challenge of computing the **diameter** ∆(G) of an undirected graph G(V,E)—that is, the longest shortest‐path between any two vertices.  A naïve all‐pairs shortest‐paths approach requires O(n·m) time on sparse graphs (or O(n³) in dense cases), which quickly becomes infeasible for **small‐world networks** with millions of nodes and hundreds of millions of edges .

Small‐world graphs, such as social networks or web graphs, typically exhibit:

* **Sparsity**: m ≪ n(n−1)/2
* **Low average distance**: average shortest‐path length ∼ O(log n)
* **Skewed degree distribution**: a few high‐degree hubs and many low‐degree vertices

Yet the exact diameter is crucial for understanding worst‐case communication delay, network resilience, and extremal path structure.

---

## 2. The BoundingDiameters Algorithm

Rather than computing every node’s eccentricity via full APSP, I implemented **BoundingDiameters**, which maintains **lower** and **upper** bounds on both per-vertex eccentricities ε(v) and the global diameter ∆:

* For each v:

  $$
    ε_L[v] \le ε(v) \le ε_U[v]
  $$
* For the graph:

  $$
    ∆_L = \max_v ε_L[v], 
    \quad ∆_U = \min\bigl(2\min_v ε_U[v],\,\max_v ε_U[v]\bigr)
  $$

Key observations from the paper allow bound tightening after each single‐source run:

1. **Per-vertex bounds**

   $$
     \max\bigl(ε(v)-d(v,w),\,d(v,w)\bigr)\;\le\;ε(w)\;\le\;ε(v)+d(v,w)
   $$
2. **Global diameter bounds**

   $$
     ∆_L \le ∆ \le ∆_U
   $$



### 2.1 High-Level Pseudocode

```text
Initialize W ← all vertices, ∆L ← -∞, ∆U ← +∞
For all w∈W: εL[w]← -∞, εU[w]← +∞

while ∆L < ∆U and W ≠ ∅:
  v ← selectFrom(W)               # choose a “critical” node
  ε[v] ← computeEccentricity(G,v) # one BFS or Dijkstra
  ∆L  ← max(∆L, ε[v])
  ∆U  ← min(∆U, 2·ε[v])
  for each w∈W:
    # update per-node bounds via Observation 1
    εL[w] ← max(εL[w], max(ε[v]−d(v,w), d(v,w)))
    εU[w] ← min(εU[w], ε[v]+d(v,w))
    # prune vertices whose interval can no longer affect the diameter
    if (εU[w] ≤ ∆L and εL[w] ≥ ⌈∆U/2⌉) or (εL[w] == εU[w]):
      remove w from W
return ∆L
```

---

## 3. Running Example

To illustrate convergence:

1. **Iteration 1**: I pick node F (highest degree in the example), compute ε(F)=5 → update ∆L=5, ∆U=10.  All εL/εU tighten but W remains large.
2. **Iteration 2**: I switch to node T (highest εU) and find ε(T)=7 → now ∆L=7, ∆U=10.  Many nodes’ intervals collapse or fall entirely within \[⌈∆U/2⌉, ∆L], so I prune dozens at once.
3. **Iteration 3**: A final pick suffices to achieve ∆L=∆U=7.

Only **3** expensive distance‐computations replace the 23 runs required by naïve APSP .

---

## 4. Implementation Details

### 4.1 Graph Loader

* I wrote `load_mm_graph` to parse **MatrixMarket** files, detecting weighted vs. pattern format and converting 1-based indices to 0-based.
* The adjacency list stores edges as `(u,v,w)` and flags `G.weighted` if any weight ≠1.

### 4.2 Eccentricity Computation

```cpp
int computeEccentricity(const Graph& G, int src, vector<int>& dist);
```

* **Unweighted**: simple BFS in O(m).
* **Weighted**: Dijkstra with a min‐heap in O(m log n).
* I store unreachable distances as `INT_MAX` and take `ceil(d)` for all non-integral shortest paths to maintain integer bounds.

### 4.3 Instrumented Loop & Logging

In `boundingDiametersInstr`, I:

* Track **RunStats**:

  * `totalEcc` (# of calls to computeEccentricity)
  * `totalPruned` (cumulative vertices removed)
  * `totalTime` (wall‐clock using `chrono`)
  * `log`: vector of `(iter, |W|, ∆L, ∆U)` for easy CSV/plot export.
* After the main loop I use `getrusage` to record peak memory (RSS in KB).

---

## 5. Selection Strategies

Choosing the right next vertex is crucial for rapid bound tightening.  I implemented three strategies in `selectFrom(...)`:

### 5.1 BOUND\_DIFF

* **Idea**: pick the vertex with the largest gap between its current upper and lower eccentricity bounds:

  $$
    w = \arg\max_{v∈W}\bigl(ε_U[v] - ε_L[v]\bigr)
  $$
* **Rationale**: a large gap indicates maximal uncertainty—probing it should yield maximal global bound reduction.
* **Implementation**: a single pass over W computing `epsU[w] - epsL[w]`.

### 5.2 INTERCHANGE

* **Idea**: alternate between “explorer” picks (maximizing εU) and “tightener” picks (minimizing εL).

  * On even iterations, choose

    $$
      w = \arg\max_{v∈W} ε_U[v]
    $$
  * On odd iterations, choose

    $$
      w = \arg\min_{v∈W} ε_L[v]
    $$
* **Rationale**: Explorer steps push down ∆U aggressively; tightener steps push up ∆L—together they close the gap from both ends more quickly than focusing on one side.
* **Implementation**: I track a `bool pickHigh` flag and flip it each call.

### 5.3 REPEATED

* **Idea**: start at a structural hub, then repeatedly jump to the node farthest (in last computed distances) from the previous pick.

  1. **Initial**: pick the vertex of maximum degree.
  2. **Subsequent**: use the last `dist[]` array to select

     $$
       w = \arg\max_{v∈W} d_{\text{last}}(prev, v)
     $$
* **Rationale**: hubs often have large eccentricities; afterward, exploring the periphery (farthest nodes) uncovers other extremal vertices quickly.
* **Implementation**: I carry `prevSelected` and use the stored `lastDist` vector.

Empirically on real-world small-world datasets, **INTERCHANGE** gave the fastest convergence in terms of calls versus pruning rate, while **BOUND\_DIFF** was simpler but sometimes oscillated, and **REPEATED** worked well on graphs with a clear hub-and-spoke structure.

---

## 6. Experimental Results

### 6.1 Dataset Characteristics

The algorithm was tested on various real-world and synthetic networks:


| Dataset | V | E | Description |
|---------|-------|-------------|-------------|
| smallworld | 100,000 | 999,996 | Synthetic small-world network |
| delaunay_n15 | 32,768 | 196,548 | Delaunay triangulation |
| delaunay_n14 | 16,384 | 98,244 | Delaunay triangulation |
| cs4 | 22,499 | 87,716 | Computer science network |
| fe_4elt2 | 11,143 | 65,636 | Finite element mesh |



### 6.2 Performance Analysis

#### 6.2.1 Eccentricity Computations
![Eccentricity Calls Comparison](take_kosters/results/comparative_EccCalls.png)

Our experimental results show significant variation in the number of eccentricity computations required:

- **Small-world Network**: Required the most computations
  - BOUND_DIFF (S1): 11,431 calls
  - INTERCHANGE (S2): 20,625 calls
  - REPEATED (S3): 14,146 calls
- **Delaunay Networks**: Moderate computation requirements
  - n15: 631-988 calls across strategies
  - n14: 523-628 calls across strategies
- **Sparse Networks**: Most efficient
  - cs4: 153-349 calls
  - fe_4elt2: 143-202 calls

#### 6.2.2 Time Performance
![Total Time Performance](take_kosters/results/comparative_TotalTime(s).png)

Runtime analysis reveals interesting patterns:

- **Small-world Network** (most computationally intensive):
  - BOUND_DIFF: 50.63s
  - INTERCHANGE: 89.10s
  - REPEATED: 60.60s
- **Other Networks**: All completed under 1 second
  - delaunay_n15: 0.46-0.72s
  - delaunay_n14: 0.19-0.23s
  - cs4: 0.06-0.15s
  - fe_4elt2: 0.03-0.04s

#### 6.2.3 Memory Usage
![Peak Memory Usage](take_kosters/results/comparative_Memory(KB).png)

Peak memory consumption shows scaling with graph size:

- smallworld: ~34-35 MB
- delaunay_n15: ~7.4-7.5 MB
- delaunay_n14: ~4.3-4.5 MB
- cs4: ~3.8-4.0 MB
- fe_4elt2: ~3.3-3.4 MB

### 6.3 Strategy Comparison

![Pruned Nodes Comparison](take_kosters/results/comparative_PrunedNodes.png)

1. **BOUND_DIFF (Strategy 1)**
   - Most consistent performance across different graph types
   - Best performer on small-world network in terms of eccentricity calls
   - Memory efficient with minimal variance

2. **INTERCHANGE (Strategy 2)**
   - Best performance on structured networks (delaunay, fe_4elt2)
   - Higher computation cost on small-world network
   - Slightly higher memory usage in most cases

3. **REPEATED (Strategy 3)**
   - Moderate performance across all metrics
   - Higher variance in number of eccentricity calls
   - Consistent memory usage profile

### 6.4 Key Findings

1. **Graph Structure Impact**
   - Small-world networks require significantly more computations (10-100x)
   - Regular structures (meshes, triangulations) show more predictable performance
   - Sparse networks (cs4, fe_4elt2) compute fastest across all strategies

2. **Strategy Effectiveness**
   - BOUND_DIFF shows best overall balance of performance metrics
   - INTERCHANGE excels on structured graphs but struggles with small-world
   - REPEATED provides a reliable middle-ground approach

3. **Scaling Behavior**
   - Memory usage scales linearly with graph size
   - Computation time heavily dependent on graph structure
   - All strategies complete in reasonable time even for 100K+ vertex graphs

---

## 7.  Reflections

* Supporting all three strategies let me benchmark and compare convergence curves directly from the logged data.
* Alternating “exploration” and “tightening” (INTERCHANGE) balanced the tradeoff between quickly lowering the upper bound and quickly raising the lower bound.
* Ceiling weighted distances simplified bound comparisons while preserving correctness guarantees.
* The modular design—separating graph I/O, eccentricity routines, selection logic, and logging—made it easy to extend or swap strategies.

---


# The iFUB Algorithm for Diameter Computation

*(Based on Crescenzi et al., TCS 514 (2013) 84–95)*

**By Aditya**

---

## Abstract

Computing the diameter of a large undirected unweighted graph naively requires $O(nm)$ time via all-pairs BFS. Crescenzi et al. introduce the **iFUB** (iterative Fringe Upper Bound) algorithm, which in practice runs in $O(m)$ time on real-world graphs by carefully interleaving BFS traversals to refine lower and upper bounds until they meet. This note summarizes the key ideas, algorithmic structure, and root-selection strategies.

---

## 1. Problem Statement

Let $G = (V,E)$ be an undirected unweighted graph with $n = |V|$, $m = |E|$. The distance $d(u, v)$ is the length of a shortest path between $u, v$, and the **diameter** is

$$
D \;=\; \max_{u,v\in V} d(u, v).
$$

A textbook approach runs a BFS from every node in $O(nm)$ time, which is infeasible on million-edge networks.

---

## 2. Overview of iFUB

iFUB maintains two values:

$$
\mathit{lb} \;\le\; D \;\le\; \mathit{ub}
$$

and iteratively refines them using BFSs from carefully chosen “fringe” vertices:

1. **Initial BFS**
   Perform a BFS from a root $u$ to compute its eccentricity $\epsilon(u) = \max_v d(u, v)$, which yields

   $$
   \mathit{lb} \leftarrow \epsilon(u),\quad \mathit{ub} \leftarrow 2\,\epsilon(u).
   $$

2. **Record Fringes**
   Record the levels (fringes)

   $$
   F_i(u) = \{\,v \mid d(u, v) = i\},\quad 0 \le i \le \epsilon(u).
   $$

3. **Bottom–Up Sweep**
   For $i = \epsilon(u),\, \epsilon(u)-1,\,\dots$ until $\mathit{ub}-\mathit{lb}$ converges:

   * For each $v \in F_i(u)$, run BFS to get $\epsilon(v)$, then

     $$
     \mathit{lb} \leftarrow \max(\mathit{lb},\,\epsilon(v)),\quad
     \mathit{ub} \leftarrow \min(\mathit{ub},\,2\,\epsilon(v)).
     $$
   * Stop when $\mathit{lb} = \mathit{ub}$ (exact diameter) or when $\mathit{ub}-\mathit{lb} \le k$ for a tolerance $k$.

### Pseudocode for Exact Diameter ($k = 0$)

```plaintext
Algorithm 1 iFUB(G, u) for exact diameter

1:  BFS(G, u)      // compute levels Fi(u) and ε(u)
2:  lb ← ε(u); ub ← 2ε(u)
3:  for i = ε(u) to 1 by −1 do
4:    for each v ∈ Fi(u) do
5:      BFS(G, v)  // compute ε(v)
6:      lb ← max(lb, ε(v))
7:      ub ← min(ub, 2ε(v))
8:      if lb = ub then
9:        return lb
10:     end if
11:   end for
12: end for
13: return lb
```

---

## 3. Root-Selection Strategies

The choice of the initial root $u$ greatly affects practical performance. Crescenzi et al. evaluate:

1. **Random**: pick $u$ uniformly at random.
2. **Highest-degree**: choose a node of maximum degree.
3. **4-Sweep-rand**: run the “4-Sweep” heuristic starting from a random node:

   * BFS from $r_1$ → find farthest $a_1$ → farthest $b_1$; set $r_2$ to the midpoint of $a_1$–$b_1$.
   * BFS from $r_2$ → find farthest $a_2$ → farthest $b_2$; set $u$ as midpoint of $a_2$–$b_2$.
4. **4-Sweep-hd**: same as above but start 4-Sweep from the highest-degree node.

Each 4-Sweep uses exactly four BFSes to approximate a “center” of $G$, yielding a root with small eccentricity and small fringe sizes.

---

## 4. Theoretical Analysis

* **Worst-case time**: still $O(nm)$, since in the worst case almost all vertices may be visited as fringe roots.
* **Amortized bound**: if $u$ has eccentricity $R$, then iFUB performs at most $N_{\ge R/2}(u)$ BFSes, where $N_{\ge h}(u)$ is the number of nodes at distance $\ge h$ from $u$.
* **Termination correctness**: the bottom-up sweep is justified by the observation that once a fringe vertex $v$ yields $\epsilon(v) \le 2(i-1)$, no vertex at distance $< i$ can exceed that bound.

---

## 5. Negative Examples

The authors exhibit graph families where:

* 4-Sweep can fail to find a tight lower bound (approximation ratio ≈ 2).
* iFUB degenerates to $n$ BFSes (e.g., odd cycles), achieving $\Theta(nm)$ time.

These pathological cases rely on highly regular structures **not** found in most real-world networks.

---

## Reflections

iFUB combines simple BFS routines with a clever fringe-based bound-refinement to compute exact diameters. With appropriate root-selection (notably 4-Sweep variants), it performs only a handful of BFSes in practice, achieving near-linear $O(m)$ behavior on complex networks.

---
## 6 Experimental Results

### Runtime by Strategy

![Time Performance Comparison](Crescenzi/iFUB_Runtime.png)


| Matrix               | Strat 0 | Strat 1 | Strat 2 | Strat 3 |
|----------------------|--------:|--------:|--------:|--------:|
| **smallworld.mtx**   | 1.4506 s | 1.8449 s | 0.0998 s | 1.4083 s |
| **fe_4elt2.mtx**     | 0.000641 s | 0.000633 s | 0.000619 s | 0.000613 s |
| **delaunay_n14.mtx** | 0.003888 s | 0.001116 s | 0.001020 s | 0.001022 s |
| **delaunay_n15.mtx** | 0.001929 s | 0.002089 s | 0.006806 s | 0.002039 s |
| **cs4.mtx**          | 0.005547 s | 0.001835 s | 0.004087 s | 0.003105 s |

### BFS Calls by Strategy

![BFS Calls Comparison](Crescenzi/iFUB_BFS_Calls.png)


| Matrix               | Strat 0 | Strat 1 | Strat 2 | Strat 3 |
|----------------------|--------:|--------:|--------:|--------:|
| **smallworld.mtx**   | 587     | 737     | 39      | 573     |
| **fe_4elt2.mtx**     | 2       | 2       | 2       | 2       |
| **delaunay_n14.mtx** | 9       | 2       | 2       | 2       |
| **delaunay_n15.mtx** | 2       | 2       | 8       | 2       |
| **cs4.mtx**          | 11      | 3       | 8       | 6       |

### Peak Memory (RSS) by Strategy

![Memory Usage Comparison](Crescenzi/iFUB_Peak_Memory.png)


| Matrix               | Strat 0    | Strat 1    | Strat 2    | Strat 3    |
|----------------------|-----------:|-----------:|-----------:|-----------:|
| **smallworld.mtx**   | 45 318 144 KB | 44 810 240 KB | 42 516 480 KB | 44 810 240 KB |
| **fe_4elt2.mtx**     | 12 206 080 KB | 12 206 080 KB | 12 222 464 KB | 12 206 080 KB |
| **delaunay_n14.mtx** | 13 336 576 KB | 13 320 192 KB | 13 320 192 KB | 13 320 192 KB |
| **delaunay_n15.mtx** | 16 580 608 KB | 16 580 608 KB | 16 580 608 KB | 16 580 608 KB |
| **cs4.mtx**          | 13 156 352 KB | 13 041 664 KB | 13 058 048 KB | 13 058 048 KB |

### Analysis

#### 1. `smallworld.mtx`
**Runtime**  
- **Strategy 2 (4-sweep random):** 0.10 s (≈10× faster than the rest)  
- **Strategies 0 & 3:** ~1.40–1.45 s  
- **Strategy 1 (highest-degree):** ~1.84 s  

**BFS Calls**  
- **Strat 2:** 39  
- **Strats 0, 1, 3:** 570–740  

**Memory (Peak RSS)**  
- **Strat 2:** ~42.5 M KB  
- **Strats 0 & 3:** ~44.8 M KB  
- **Strat 1:** ~44.8 M KB  

> **Takeaway:** On small-world graphs, **4-sweep random** dominates in speed, BFS count, and memory.  
> _Note: Strat 0 and Strat 2 measurements can vary drastically if the `srand` seed changes._

---

#### 2. `fe_4elt2.mtx` (Finite-element mesh)
**Runtime**  
- All strategies finish in < 0.00065 s  

**BFS Calls**  
- 2 (initial tree + one refinement)  

**Memory**  
- ~12.2 M KB  

> **Takeaway:** Very low-diameter, well-connected mesh—any root converges immediately.

---

#### 3. `delaunay_n14.mtx` (Planar Delaunay graph)
**Runtime**  
- **Strat 1 (highest-degree):** ~0.00112 s (fastest)  
- **Strats 2 & 3:** ~0.00102 s  
- **Strat 0:** ~0.00389 s  

**BFS Calls**  
- **Strat 0:** 9  
- **Strats 1, 2, 3:** 2  

**Memory**  
- ~13.32 M KB  

> **Takeaway:** Highest-degree root (Strat 1) is best; random roots can land in “long arms,” costing extra BFS.

---

#### 4. `delaunay_n15.mtx` (Larger Delaunay graph)
**Runtime**  
- **Strat 0:** ~0.00193 s  
- **Strats 1 & 3:** ~0.002 s  
- **Strat 2:** ~0.00681 s  

**BFS Calls**  
- **Strat 2:** 8  
- **Strats 0, 1, 3:** 2  

**Memory**  
- ~16.58 M KB  

> **Takeaway:** On this larger graph, 4-sweep random (Strat 2) may pick a less-central root and pay for it. Simpler roots suffice.

---

#### 5. `cs4.mtx` (Circuit-simulation graph)
**Runtime**  
- **Strat 1:** ~0.00184 s (fastest)  
- **Strat 3:** ~0.00311 s  
- **Strat 2:** ~0.00409 s  
- **Strat 0:** ~0.00555 s  

**BFS Calls**  
- **Strat 1:** 3  
- **Strat 3:** 6  
- **Strat 2:** 8  
- **Strat 0:** 11  

**Memory**  
- ~13.04–13.15 M KB  

> **Takeaway:** Highest-degree root (Strat 1) is most efficient here; blind 4-sweep random (Strat 2) can misfire, costing extra passes.  
> _Note: Strat 0 and Strat 2 can vary if the random seed is changed._

---

## Comparative Analysis: BoundingDiameters vs. iFUB

This section provides a comparative analysis of the BoundingDiameters algorithm (referred to as BD) and the iFUB algorithm based on their experimental results on a common set of graph datasets. The comparison focuses on the number of core computations (eccentricity/BFS calls), overall runtime, and peak memory usage. For BoundingDiameters, results generally refer to its best or representative strategies (S1: BOUND_DIFF, S2: INTERCHANGE). For iFUB, results refer to its best performing strategies (often Strat 2: 4-Sweep-rand for `smallworld`, Strat 1: highest-degree, or ranges).

### 1. BFS/Eccentricity Calls

Both algorithms rely on graph traversals (BFS for unweighted graphs, Dijkstra for weighted in BD) to compute eccentricities. The number of such calls is a key indicator of computational work.

*   **`smallworld.mtx` (100k V, 1M E):**
    *   **BD:** 11,431 (S1) to 20,625 (S2) calls.
    *   **iFUB:** 39 (Strat 2) to 737 calls.
    *   *iFUB is vastly superior, requiring significantly fewer calls.*
*   **`delaunay_n15.mtx` (32k V, 196k E):**
    *   **BD:** 631-988 calls.
    *   **iFUB:** 2 (best strategies) to 8 calls.
    *   *iFUB is vastly superior.*
*   **`delaunay_n14.mtx` (16k V, 98k E):**
    *   **BD:** 523-628 calls.
    *   **iFUB:** 2 (best strategies) to 9 calls.
    *   *iFUB is vastly superior.*
*   **`cs4.mtx` (22k V, 87k E):**
    *   **BD:** 153-349 calls.
    *   **iFUB:** 3 (Strat 1) to 11 calls.
    *   *iFUB is vastly superior.*
*   **`fe_4elt2.mtx` (11k V, 65k E):**
    *   **BD:** 143-202 calls.
    *   **iFUB:** 2 calls for all strategies.
    *   *iFUB is vastly superior.*

**Summary (BFS/Eccentricity Calls):** iFUB consistently outperforms BoundingDiameters by a significant margin, often requiring orders of magnitude fewer BFS/eccentricity computations across all tested datasets. This suggests iFUB's strategy for selecting nodes and updating bounds is much more efficient at converging to the exact diameter.

### 2. Time Performance

Execution time reflects the practical efficiency of the algorithms.

*   **`smallworld.mtx`:**
    *   **BD:** 50.63s (S1) to 89.10s (S2).
    *   **iFUB:** 0.0998s (Strat 2) to ~1.8s.
    *   *iFUB is dramatically faster.*
*   **`delaunay_n15.mtx`:**
    *   **BD:** 0.46s to 0.72s.
    *   **iFUB:** ~0.0019s to ~0.0068s.
    *   *iFUB is significantly faster.*
*   **`delaunay_n14.mtx`:**
    *   **BD:** 0.19s to 0.23s.
    *   **iFUB:** ~0.0010s to ~0.0039s.
    *   *iFUB is significantly faster.*
*   **`cs4.mtx`:**
    *   **BD:** 0.06s to 0.15s.
    *   **iFUB:** ~0.0018s to ~0.0055s.
    *   *iFUB is significantly faster.*
*   **`fe_4elt2.mtx`:**
    *   **BD:** 0.03s to 0.04s.
    *   **iFUB:** < 0.00065s.
    *   *iFUB is significantly faster.*

**Summary (Time Performance):** Consistent with the fewer BFS calls, iFUB demonstrates substantially better time performance across all datasets. The differences are particularly stark on the larger `smallworld` graph, where iFUB is orders of magnitude faster.

### 3. Memory Usage

Peak memory usage (RSS) indicates the memory footprint of each algorithm. (Assuming iFUB's "KB" values with spaces are byte counts, converted to MB for comparison).

*   **`smallworld.mtx`:**
    *   **BD:** ~34-35 MB.
    *   **iFUB:** ~42.5 MB (Strat 2) to ~45.3 MB.
    *   *BoundingDiameters is slightly more memory efficient.*
*   **`delaunay_n15.mtx`:**
    *   **BD:** ~7.4-7.5 MB.
    *   **iFUB:** ~16.5 MB.
    *   *BoundingDiameters is significantly more memory efficient (uses less than half).*
*   **`delaunay_n14.mtx`:**
    *   **BD:** ~4.3-4.5 MB.
    *   **iFUB:** ~13.3 MB.
    *   *BoundingDiameters is significantly more memory efficient (uses about one-third).*
*   **`cs4.mtx`:**
    *   **BD:** ~3.8-4.0 MB.
    *   **iFUB:** ~13.0 - 13.1 MB.
    *   *BoundingDiameters is significantly more memory efficient (uses about one-third).*
*   **`fe_4elt2.mtx`:**
    *   **BD:** ~3.3-3.4 MB.
    *   **iFUB:** ~12.2 MB.
    *   *BoundingDiameters is significantly more memory efficient (uses about one-fourth).*

**Summary (Memory Usage):** BoundingDiameters consistently exhibits lower peak memory usage compared to iFUB. For smaller graphs, iFUB's memory footprint can be 2-4 times larger. On the largest graph (`smallworld`), the difference is less pronounced but still favors BoundingDiameters. This suggests that iFUB might maintain more extensive data structures (e.g., for fringe sets) during its execution.

### 4. Overall Conclusion

The comparison reveals a clear trade-off between computational efficiency and memory usage:

*   **iFUB Algorithm:**
    *   **Pros:** Exceptionally fast in terms of execution time and requires drastically fewer BFS/eccentricity computations. Its strategies, particularly 4-Sweep, are highly effective in quickly identifying critical nodes.
    *   **Cons:** Higher memory consumption compared to BoundingDiameters. The presented iFUB is primarily for unweighted graphs.

*   **BoundingDiameters Algorithm:**
    *   **Pros:** More memory-efficient across all datasets. It also has explicit support for weighted graphs (using Dijkstra), making it more versatile.
    *   **Cons:** Significantly slower and performs a much larger number of eccentricity computations.

**Which algorithm performs better?**

*   For **speed and minimizing computational load (CPU time, I/O for graph traversals)**, **iFUB is unequivocally the superior algorithm** for unweighted graphs. Its ability to converge with a minimal number of BFS calls makes it highly suitable for large networks where these computations are expensive.
*   If **memory is an extremely critical constraint**, BoundingDiameters offers a more frugal alternative, albeit at a substantial cost in performance.
*   If the graph is **weighted**, BoundingDiameters provides a direct solution, whereas the iFUB version discussed is for unweighted graphs.

In typical scenarios involving large unweighted networks where performance is key, **iFUB would be the preferred choice** due to its remarkable speed advantage. However, the lower memory footprint and weighted graph capability of BoundingDiameters could be decisive in specific contexts.






