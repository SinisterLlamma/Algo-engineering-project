#!/usr/bin/env python3
import os
import subprocess
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from pathlib import Path

def run_experiment(binary_path, graph_path, output_dir):
    """Run experiment for a single graph and move results to output directory"""
    # Create output directory for this graph
    graph_name = Path(graph_path).stem
    graph_output_dir = Path(output_dir) / graph_name
    graph_output_dir.mkdir(parents=True, exist_ok=True)
    
    # Run the experiment
    cmd = ["python", "generate_master_csv.py", "--binary", binary_path, "--graph", graph_path]
    subprocess.run(cmd, check=True)
    
    # Move results to graph-specific directory
    os.rename("master_summary.csv", graph_output_dir / "master_summary.csv")
    os.rename("master_iters.csv", graph_output_dir / "master_iters.csv")
    
    # Generate plots for this graph with correct input directory
    plot_cmd = ["python", "plot_master_results.py", "--input-dir", str(graph_output_dir)]
    subprocess.run(plot_cmd, check=True)
    
    # No need to move plots as they're already in the correct directory
    return pd.read_csv(graph_output_dir / "master_summary.csv")

def create_comparative_table(all_results, output_dir):
    """Create a comparative table across all datasets"""
    # Group by Dataset and Strategy, then compute means
    summary = all_results.groupby(['Dataset', 'Strategy']).agg({
        '|V|': 'first',
        '|E|': 'first',
        'AvgDeg': 'first',
        'EccCalls': 'mean',
        'PrunedNodes': 'mean',
        'TotalTime(s)': 'mean',
        'Memory(KB)': 'mean'
    }).round(2)
    
    # Save the summary
    summary.to_csv(Path(output_dir) / "comparative_summary.csv")
    return summary

def create_comparative_plots(all_results, output_dir):
    """Create comparative plots across all datasets"""
    metrics = ['EccCalls', 'PrunedNodes', 'TotalTime(s)', 'Memory(KB)']
    
    for metric in metrics:
        plt.figure(figsize=(12, 6))
        sns.barplot(data=all_results, x='Dataset', y=metric, hue='Strategy')
        plt.xticks(rotation=45)
        plt.title(f'Comparison of {metric} across Datasets')
        plt.tight_layout()
        plt.savefig(Path(output_dir) / f'comparative_{metric}.png')
        plt.close()

def main():
    # Configuration
    binary_path = "./bounding"
    dataset_dir = "../Dataset"
    output_dir = "./results"
    
    # Create output directory
    Path(output_dir).mkdir(exist_ok=True)
    
    # Find all .mtx files
    graph_files = list(Path(dataset_dir).glob("*.mtx"))
    
    # Run experiments for each graph
    all_results = []
    for graph_file in graph_files:
        print(f"Processing {graph_file.name}...")
        results = run_experiment(binary_path, str(graph_file), output_dir)
        all_results.append(results)
    
    # Combine all results
    all_results = pd.concat(all_results, ignore_index=True)
    
    # Create comparative analysis
    summary = create_comparative_table(all_results, output_dir)
    create_comparative_plots(all_results, output_dir)
    
    print("\nExperiments completed. Results are in the 'results' directory:")
    print("- Individual graph results in subdirectories")
    print("- Comparative summary in comparative_summary.csv")
    print("- Comparative plots as comparative_*.png")

if __name__ == "__main__":
    main()
