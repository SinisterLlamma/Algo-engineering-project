# Crescenzi iFUB Project

This project implements the iFUB algorithm from Crescenzi et al. for computing the exact diameter of undirected graphs. It includes multiple C++ programs for timing, memory analysis, and BFS metrics, along with Python scripts for plotting results.

---

## 📄 C++ Programs and Usage

### 1. `Crescenzi.cpp`
- **Purpose:** Runs the iFUB algorithm on a single `.mtx` graph file using a specified root-selection strategy.
- **Usage:**
  ```bash
  g++ -std=c++17 Crescenzi.cpp -o cresc
  ./cresc <strategy_number> <graph_file.mtx>
  ```
- **Example:**
  ```bash
  ./cresc 3 graph.mtx
  ```
- **Notes:** Strategy is one of:
  - `0`: Random root
  - `1`: Highest-degree root
  - `2`: 4-sweep random
  - `3`: 4-sweep highest-degree

### 2. `CrescenziAverageTime.cpp`
- **Purpose:** Applies a single strategy across all `.mtx` files in a directory and prints timing for each file and the final average.
- **Usage:**
  ```bash
  g++ -std=c++17 CrescenziAverageTime.cpp -o cresc_avg
  ./cresc_avg <strategy_number> <folder_path>
  ```
- **Example:**
  ```bash
  ./cresc_avg 2 all_mtx_files/
  ```

### 3. `GraphTiming.cpp`
- **Purpose:** Runs **all four strategies** on every `.mtx` file in a folder and outputs only the average time for each strategy.
- **Output File:** `results.csv`
- **Usage:**
  ```bash
  g++ -std=c++17 GraphTiming.cpp -o strat_times
  ./strat_times <folder_path> > results.csv
  ```

### 4. `mem_stats.cpp`
- **Purpose:** Records **peak memory usage** (RSS) of each strategy for a single graph.
- **Output File:** `mem.csv`
- **Usage:**
  ```bash
  g++ -std=c++17 mem_stats.cpp -o mem
  ./mem <strategy_number> <graph_file.mtx> > mem.csv
  ```

### 5. `bfs_called.cpp`
- **Purpose:** Tracks **number of BFS calls** made during iFUB for each strategy on a single graph.
- **Output File:** `bfs_called.csv`
- **Usage:**
  ```bash
  g++ -std=c++17 bfs_called.cpp -o bfs
  ./bfs <strategy_number> <graph_file.mtx> > bfs_called.csv
  ```

---

## 📊 Python Plotting Scripts

### Requirements
- Python 3
- `pandas`
- `matplotlib`

### Scripts & Usage
- **Plot strategy average runtimes (from `results.csv`):**
  ```bash
  python3 Plot.py
  ```
- **Plot memory usage (from `mem.csv`):**
  ```bash
  python3 plot_mem.py mem.csv
  ```
- **Plot BFS calls (from `bfs_called.csv`):**
  ```bash
  python3 plot_bfs.py bfs_called.csv
  ```

---

## 🔁 Randomness Note
This implementation uses `srand(time(0))` to initialize randomness. Runs for strategies `0` and `2` will yield different results unless the seed is fixed manually in code.

---

## 📌 Additional Notes
- All input graphs must be square `.mtx` Matrix Market format.
- This implementation assumes undirected and unweighted graphs.
- Only the following programs **require saving output to a file** for Python plotting:
  - `GraphTiming.cpp`
  - `mem_stats.cpp`
  - `bfs_called.cpp`
