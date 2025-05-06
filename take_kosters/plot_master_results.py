#!/usr/bin/env python3
"""
plot_master_results.py

Reads in:
  - master_summary.csv (columns: Dataset,|V|,|E|,AvgDeg,Strategy,EccCalls,PrunedNodes,TotalTime(s))
  - master_iters.csv   (columns: iter,|W|,DeltaL,DeltaU,Strategy)

Generates these PNG plots:
  1. EccCalls.png        - Number of eccentricity calls by strategy
  2. PrunedNodes.png     - Total pruned nodes by strategy
  3. TotalTime(s).png    - Total time (s) by strategy
  4. W_vs_iter.png       - Candidate-set size vs iteration
  5. bounds_vs_iter.png  - ΔL & ΔU vs iteration (twin-axis)
  6. gap_vs_iter.png     - ΔU – ΔL vs iteration
  7. pruned_vs_iter.png  - Cumulative pruned nodes vs iteration
"""

import pandas as pd
import matplotlib.pyplot as plt

def plot_summary(df):
    metrics = {
        'EccCalls': 'Number of Eccentricity Calls',
        'PrunedNodes': 'Total Pruned Nodes',
        'TotalTime(s)': 'Total Time (s)',
        'Memory(KB)': 'Peak Memory (KB)'
    }
    for col, title in metrics.items():
        plt.figure()
        plt.bar(df['Strategy'].astype(str), df[col])
        plt.xlabel('Strategy')
        plt.ylabel(title)
        plt.title(f'{title} by Strategy')
        plt.tight_layout()
        plt.savefig(f'{col}.png')

def plot_iterations(df):
    # Plot |W| vs iteration
    plt.figure()
    for strat, group in df.groupby('Strategy'):
        plt.plot(group['iter'], group['|W|'], label=f'S{strat}')
    plt.xlabel('Iteration')
    plt.ylabel('Remaining |W|')
    plt.title('Candidate‐set Size vs Iteration')
    plt.legend()
    plt.tight_layout()
    plt.savefig('W_vs_iter.png')

    # Plot ΔL and ΔU vs iteration
    plt.figure()
    ax1 = plt.gca()
    for strat, group in df.groupby('Strategy'):
        ax1.plot(group['iter'], group['DeltaL'], label=f'ΔL (S{strat})')
    ax1.set_xlabel('Iteration')
    ax1.set_ylabel('ΔL')
    ax2 = ax1.twinx()
    for strat, group in df.groupby('Strategy'):
        ax2.plot(group['iter'], group['DeltaU'], linestyle='--', label=f'ΔU (S{strat})')
    ax2.set_ylabel('ΔU')
    lines1, labels1 = ax1.get_legend_handles_labels()
    lines2, labels2 = ax2.get_legend_handles_labels()
    ax1.legend(lines1 + lines2, labels1 + labels2, loc='upper right')
    plt.title('Bounds vs Iteration')
    plt.tight_layout()
    plt.savefig('bounds_vs_iter.png')

    # Plot gap vs iteration
    plt.figure()
    for strat, group in df.groupby('Strategy'):
        gap = group['DeltaU'] - group['DeltaL']
        plt.plot(group['iter'], gap, label=f'S{strat}')
    plt.xlabel('Iteration')
    plt.ylabel('ΔU – ΔL')
    plt.title('Gap vs Iteration')
    plt.legend()
    plt.tight_layout()
    plt.savefig('gap_vs_iter.png')

    # Plot cumulative pruned nodes vs iteration
    plt.figure()
    for strat, group in df.groupby('Strategy'):
        pruned = group['|W|'].shift(1).fillna(group['|W|'].iloc[0]) - group['|W|']
        cum_pruned = pruned.cumsum()
        plt.plot(group['iter'], cum_pruned, label=f'S{strat}')
    plt.xlabel('Iteration')
    plt.ylabel('Cumulative Pruned Nodes')
    plt.title('Pruned Nodes vs Iteration')
    plt.legend()
    plt.tight_layout()
    plt.savefig('pruned_vs_iter.png')

def main():
    # Accept input directory as optional argument
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--input-dir', default='.', help='Directory containing the CSV files')
    args = parser.parse_args()

    # Load the master CSVs from the specified directory
    import os
    df_sum = pd.read_csv(os.path.join(args.input_dir, 'master_summary.csv'))
    df_it  = pd.read_csv(os.path.join(args.input_dir, 'master_iters.csv'))

    # Change to input directory for saving plots
    os.chdir(args.input_dir)
    plot_summary(df_sum)
    plot_iterations(df_it)

    print(f"All plots generated in {args.input_dir}:")
    print("  EccCalls.png, PrunedNodes.png, TotalTime(s).png")
    print("  W_vs_iter.png, bounds_vs_iter.png, gap_vs_iter.png, pruned_vs_iter.png")

if __name__ == '__main__':
    main()
