#!/usr/bin/env python3
"""
plot_diameter_results.py

Handles both named‐column CSVs and pure data CSVs (headerless)
as output by the C++ code.

Usage:
    python plot_diameter_results.py --summary summary.csv --iters iters.csv
"""

import argparse
import pandas as pd
import matplotlib.pyplot as plt
import io

# Expected column names
SUMMARY_COLS = [
    "Dataset", "|V|", "|E|", "AvgDeg",
    "Strategy", "EccCalls", "PrunedNodes", "TotalTime(s)"
]
ITERS_COLS = ["iter", "|W|", "DeltaL", "DeltaU"]

def load_summary(path):
    # Try reading with header
    df = pd.read_csv(path)
    if "Strategy" in df.columns:
        # named columns present
        if len(df) != 1:
            raise ValueError("summary.csv must have exactly one row")
        return df
    # Fallback: headerless
    df = pd.read_csv(path, header=None)
    if df.shape[1] != len(SUMMARY_COLS):
        raise ValueError(
            f"Expected {len(SUMMARY_COLS)} columns in headerless summary, "
            f"found {df.shape[1]}"
        )
    df.columns = SUMMARY_COLS
    if len(df) != 1:
        raise ValueError("headerless summary.csv must have exactly one row")
    return df

def load_iterations(path, default_strategy):
    # Try reading with header
    df = pd.read_csv(path, comment='#')
    missing = set(ITERS_COLS) - set(df.columns)
    if not missing and "iter" in df.columns:
        # has named columns
        if "Strategy" not in df.columns:
            df["Strategy"] = default_strategy
        return df

    # Fallback: headerless
    df = pd.read_csv(path, header=None)
    if df.shape[1] not in (len(ITERS_COLS), len(ITERS_COLS)+1):
        raise ValueError(
            f"Expected {len(ITERS_COLS)} or {len(ITERS_COLS)+1} columns in headerless iters, "
            f"found {df.shape[1]}"
        )
    # Assign first 4 columns
    df.columns = ITERS_COLS + (
        ["Strategy"] if df.shape[1] == len(ITERS_COLS)+1 else []
    )
    if "Strategy" not in df.columns:
        df["Strategy"] = default_strategy
    return df

def plot_summary(df):
    strategies = df["Strategy"].astype(str)
    metrics = {
        "EccCalls": "Number of Eccentricity Calls",
        "PrunedNodes": "Total Pruned Nodes",
        "TotalTime(s)": "Total Time (s)"
    }
    for col, title in metrics.items():
        plt.figure()
        plt.bar(strategies, df[col])
        plt.xlabel("Strategy")
        plt.ylabel(title)
        plt.title(f"{title} by Strategy")
        plt.tight_layout()
        plt.savefig(f"{col}.png")

def plot_iterations(df):
    df["Strategy"] = df["Strategy"].astype(str)

    # 1. |W| vs iteration
    plt.figure()
    for strat, g in df.groupby("Strategy"):
        plt.plot(g["iter"], g["|W|"], label=f"S{strat}")
    plt.xlabel("Iteration")
    plt.ylabel("Remaining |W|")
    plt.title("Candidate‐set Size vs Iteration")
    plt.legend()
    plt.tight_layout()
    plt.savefig("W_vs_iter.png")

    # 2. ΔL & ΔU vs iteration (twin‐axis)
    plt.figure()
    ax1 = plt.gca()
    for strat, g in df.groupby("Strategy"):
        ax1.plot(g["iter"], g["DeltaL"], label=f"ΔL (S{strat})")
    ax1.set_xlabel("Iteration")
    ax1.set_ylabel("ΔL")
    ax2 = ax1.twinx()
    for strat, g in df.groupby("Strategy"):
        ax2.plot(g["iter"], g["DeltaU"], linestyle="--", label=f"ΔU (S{strat})")
    ax2.set_ylabel("ΔU")
    lines1, labs1 = ax1.get_legend_handles_labels()
    lines2, labs2 = ax2.get_legend_handles_labels()
    ax1.legend(lines1 + lines2, labs1 + labs2, loc="upper right")
    plt.title("Bounds vs Iteration")
    plt.tight_layout()
    plt.savefig("bounds_vs_iter.png")

    # 3. Gap vs iteration
    plt.figure()
    for strat, g in df.groupby("Strategy"):
        plt.plot(g["iter"], g["DeltaU"] - g["DeltaL"], label=f"S{strat}")
    plt.xlabel("Iteration")
    plt.ylabel("ΔU – ΔL")
    plt.title("Gap vs Iteration")
    plt.legend()
    plt.tight_layout()
    plt.savefig("gap_vs_iter.png")

    # 4. Cumulative pruned nodes vs iteration
    plt.figure()
    for strat, g in df.groupby("Strategy"):
        pruned = g["|W|"].shift(1).fillna(g["|W|"].iloc[0]) - g["|W|"]
        plt.plot(g["iter"], pruned.cumsum(), label=f"S{strat}")
    plt.xlabel("Iteration")
    plt.ylabel("Cumulative Pruned Nodes")
    plt.title("Pruned Nodes vs Iteration")
    plt.legend()
    plt.tight_layout()
    plt.savefig("pruned_vs_iter.png")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--summary", required=True,
                        help="CSV with or without header")
    parser.add_argument("--iters", required=True,
                        help="CSV with or without header")
    args = parser.parse_args()

    # Load summary (must end up with exactly one row and a Strategy)
    df_sum = load_summary(args.summary)
    strat0 = int(df_sum["Strategy"].iloc[0])

    # Load iterations, auto‐tagging Strategy if needed
    df_it = load_iterations(args.iters, default_strategy=strat0)

    # Plot everything
    plot_summary(df_sum)
    plot_iterations(df_it)

    print("Saved plots:")
    print("  EccCalls.png, PrunedNodes.png, TotalTime(s).png")
    print("  W_vs_iter.png, bounds_vs_iter.png, gap_vs_iter.png, pruned_vs_iter.png")

if __name__ == "__main__":
    main()
