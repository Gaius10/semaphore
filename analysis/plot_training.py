#!/usr/bin/env python3
"""Plot a training run CSV produced by the semaphore trainer.

Usage:
    python3 analysis/plot_training.py                 # latest data/training_*.csv
    python3 analysis/plot_training.py data/foo.csv    # a specific run
    python3 analysis/plot_training.py --show          # also open a window

The figure is saved to analysis/plots/<csv name>.png.
"""

import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd

ROOT = Path(__file__).resolve().parent.parent
DATA_DIR = ROOT / "data"
PLOTS_DIR = ROOT / "analysis" / "plots"


def latest_csv() -> Path:
    files = sorted(DATA_DIR.glob("training_*.csv"))
    if not files:
        raise SystemExit(f"no training_*.csv found in {DATA_DIR}")
    return files[-1]


def plot(df: pd.DataFrame, title: str):
    gen = df["generation"]

    fig, ax = plt.subplots(figsize=(10, 6), constrained_layout=True)
    ax.fill_between(gen, df["worst_fitness"], df["champion_fitness"], alpha=0.2, label="worst–champion")
    ax.plot(gen, df["champion_fitness"], label="champion")
    ax.plot(gen, df["mean_fitness"], label="mean")
    ax.set(title=title, xlabel="generation", ylabel="fitness")
    ax.legend()
    ax.grid(alpha=0.3)

    return fig


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("csv", nargs="?", type=Path, help="training CSV (default: latest in data/)")
    parser.add_argument("--show", action="store_true", help="open an interactive window")
    args = parser.parse_args()

    path = args.csv or latest_csv()
    df = pd.read_csv(path)

    fig = plot(df, path.name)
    PLOTS_DIR.mkdir(parents=True, exist_ok=True)
    out = PLOTS_DIR / f"{path.stem}.png"
    fig.savefig(out, dpi=120)
    print(f"saved {out.relative_to(ROOT)}")

    if args.show:
        plt.show()


if __name__ == "__main__":
    main()
