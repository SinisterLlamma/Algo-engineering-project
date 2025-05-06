#!/usr/bin/env python3
"""
generate_master_csv.py

Runs your diameter‐bounding binary for strategies 1, 2 and 3 on a given graph,
and writes out:

  - master_summary.csv : one row per strategy (for Table 1)
  - master_iters.csv   : all per‐iteration logs, with a Strategy column

Usage:
    python generate_master_csv.py --binary ./bounding --graph mygraph.mtx
"""
import subprocess
import pandas as pd
import io
import argparse
import sys

def run_strategy(binary, strategy, graph):
    """Run `binary --strategy {strategy} {graph}` and return its stdout."""
    cmd = [binary, "--strategy", str(strategy), graph]
    out = subprocess.check_output(cmd, text=True)
    return out

def split_blocks(output):
    """
    Given the full stdout, split into two blocks:
      block1 = lines before the first line starting with '#'
      block2 = lines from that line (inclusive) to the end
    """
    lines = output.strip().splitlines()
    for i,line in enumerate(lines):
        if line.lstrip().startswith('#'):
            return ("\n".join(lines[:i]), "\n".join(lines[i:]))
    raise ValueError("Could not find iteration‐log header (starting with '#')")

def parse_summary(block1):
    """Read the first CSV block into a DataFrame (headered or not)."""
    # try with header:
    try:
        df = pd.read_csv(io.StringIO(block1))
        # if no 'Strategy' column, it's headerless
        if "Strategy" not in df.columns:
            raise ValueError
    except Exception:
        # headerless: assign the eight known columns
        cols = ["Dataset","|V|","|E|","AvgDeg",
                "Strategy","EccCalls","PrunedNodes","TotalTime(s)","Memory(KB)"]
        df = pd.read_csv(io.StringIO(block1), header=None, names=cols)
    return df

def parse_iters(block2, strategy):
    """
    Read the second CSV block into a DataFrame. 
    Ensure we have columns iter,|W|,DeltaL,DeltaU,Strategy.
    The block2 starts with a comment like "# iter,|W|,DeltaL,DeltaU".
    """
    # strip leading '#' from the header:
    lines = block2.strip().splitlines()
    header = lines[0].lstrip('#').strip()
    data   = lines[1:]
    df = pd.read_csv(io.StringIO(header + "\n" + "\n".join(data)))
    if "Strategy" not in df.columns:
        df["Strategy"] = strategy
    return df

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--binary", required=True,
                   help="Path to your compiled C++ binary (e.g. ./bounding)")
    p.add_argument("--graph", required=True,
                   help="Path to the .mtx graph file")
    args = p.parse_args()

    all_summaries = []
    all_iters     = []

    for strat in [1,2,3]:
        sys.stdout.write(f"Running strategy {strat}...\n")
        out = run_strategy(args.binary, strat, args.graph)

        block1, block2 = split_blocks(out)
        df_sum = parse_summary(block1)
        df_it  = parse_iters(block2, strat)

        all_summaries.append(df_sum)
        all_iters.append(df_it)

    # concatenate
    master_sum = pd.concat(all_summaries, ignore_index=True)
    master_it  = pd.concat(all_iters,     ignore_index=True)

    # write out
    master_sum.to_csv("master_summary.csv", index=False)
    master_it .to_csv("master_iters.csv",   index=False)

    print("Wrote master_summary.csv and master_iters.csv")

if __name__=="__main__":
    main()
