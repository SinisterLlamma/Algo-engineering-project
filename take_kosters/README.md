# Algo-engineering-project

## Installation

```bash
# Compile the C++ code
g++ -O2 -std=c++17 takes_kosters.cpp -o bounding

# Install Python dependencies
pip install pandas matplotlib seaborn
```

## Running Single Experiments

To run a single experiment:
```bash
./bounding --strategy 2 path/to/graph.mtx > results.csv
```

## Running Full Analysis

To run experiments for all strategies and generate plots:

```bash
# Run all strategies and generate master CSVs
python generate_master_csv.py --binary ./bounding --graph path/to/graph.mtx

# Generate plots for the results
python plot_master_results.py --input-dir .
```

## Running Batch Analysis

To analyze multiple graphs and generate comparative results:

```bash
# Run experiments for all graphs in Dataset folder
python run_all_experiments.py

# Results will be organized in the results/ directory:
# results/
# ├── comparative_summary.csv      # Summary table comparing all datasets
# ├── comparative_*.png           # Comparative plots
# └── {graph_name}/              # Per-graph results
#     ├── master_summary.csv
#     ├── master_iters.csv
#     └── *.png                  # Individual plots
```

## Output Files

- `master_summary.csv`: Summary statistics for each strategy
- `master_iters.csv`: Iteration-level data for each strategy
- Various `.png` files showing different metrics and comparisons

## Generated Plots

1. `EccCalls.png` - Number of eccentricity calls by strategy
2. `PrunedNodes.png` - Total pruned nodes by strategy
3. `TotalTime(s).png` - Total time (s) by strategy
4. `W_vs_iter.png` - Candidate-set size vs iteration
5. `bounds_vs_iter.png` - ΔL & ΔU vs iteration
6. `gap_vs_iter.png` - ΔU – ΔL vs iteration
7. `pruned_vs_iter.png` - Cumulative pruned nodes vs iteration

For batch analysis, additional comparative plots are generated showing trends across all datasets.