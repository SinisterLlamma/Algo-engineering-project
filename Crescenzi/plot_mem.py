#!/usr/bin/env python3
import sys
import pandas as pd
import matplotlib.pyplot as plt

def main(csv_path):
    df = pd.read_csv(csv_path)
    # map strategy codes to names (optional)
    names = {
        0: "random",
        1: "highest_deg",
        2: "4s-rand",
        3: "4s-hd"
    }
    df['strategy_name'] = df['strategy'].map(names)

    plt.figure(figsize=(6,4))
    plt.bar(df['strategy_name'], df['peak_rss_kb'] / 1024, color='C0')
    plt.ylabel("Peak RSS (MB)")
    plt.xlabel("Strategy")
    plt.title("iFUB Peak Memory by Root‐Selection Strategy")
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    if len(sys.argv)!=2:
        print("Usage: plot_mem.py mem.csv")
        sys.exit(1)
    main(sys.argv[1])
