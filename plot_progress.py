#!/usr/bin/env python3
import os
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent
os.environ.setdefault("MPLCONFIGDIR", str(ROOT / ".matplotlib-cache"))
os.environ.setdefault("TMPDIR", str(ROOT / ".tmp"))
(ROOT / ".matplotlib-cache").mkdir(exist_ok=True)
(ROOT / ".tmp").mkdir(exist_ok=True)

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


def parse_log(path):
    rows = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.startswith("|"):
            continue
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if len(cells) < 6 or not cells[0].isdigit():
            continue
        score_match = re.search(r"(\d+(?:\.\d+)?)", cells[3])
        if not score_match:
            continue
        rows.append(
            {
                "iter": int(cells[0]),
                "change": cells[1].replace("`", ""),
                "score": float(score_match.group(1)),
                "decision": cells[4],
            }
        )
    return rows


def best_so_far(rows):
    best = None
    values = []
    for row in rows:
        if row["decision"].lower() == "keep":
            best = row["score"] if best is None else min(best, row["score"])
        values.append(best if best is not None else row["score"])
    return values


def main():
    rows = parse_log(ROOT / "program.md")
    if not rows:
        raise SystemExit("No experiment rows found in program.md")

    xs = [row["iter"] for row in rows]
    ys = [row["score"] for row in rows]
    best = best_so_far(rows)
    colors = ["#15803d" if row["decision"].lower() == "keep" else "#b91c1c" for row in rows]

    plt.style.use("seaborn-v0_8-whitegrid")
    fig, ax = plt.subplots(figsize=(10.5, 6), dpi=160)

    ax.plot(xs, ys, color="#64748b", linewidth=1.5, alpha=0.65, label="Experiment score")
    ax.plot(xs, best, color="#0f766e", linewidth=2.5, marker="o", label="Best kept score")
    ax.scatter(xs, ys, c=colors, s=95, edgecolors="white", linewidths=1.4, zorder=3)

    for row in rows:
        label = f"{row['score']:.2f}"
        offset = -18 if row["decision"].lower() == "keep" else 12
        ax.annotate(
            label,
            (row["iter"], row["score"]),
            textcoords="offset points",
            xytext=(0, offset),
            ha="center",
            fontsize=9,
            color="#0f172a",
        )

    baseline = rows[0]["score"]
    current = best[-1]
    improvement = ((baseline - current) / baseline) * 100.0

    ax.set_title("Order Book AI Optimization Progress", fontsize=16, weight="bold", pad=14)
    ax.set_xlabel("Iteration")
    ax.set_ylabel("Median time (ms, lower is better)")
    ax.set_xticks(xs)
    ax.text(
        0.02,
        0.04,
        f"Baseline: {baseline:.2f} ms   Best: {current:.2f} ms   Improvement: {improvement:.2f}%",
        transform=ax.transAxes,
        fontsize=10,
        color="#334155",
        bbox={"boxstyle": "round,pad=0.4", "facecolor": "#f8fafc", "edgecolor": "#cbd5e1"},
    )

    keep_proxy = ax.scatter([], [], c="#15803d", s=70, label="Keep")
    discard_proxy = ax.scatter([], [], c="#b91c1c", s=70, label="Discard")
    handles, labels = ax.get_legend_handles_labels()
    handles.extend([keep_proxy, discard_proxy])
    labels.extend(["Keep", "Discard"])
    ax.legend(handles, labels, loc="upper right", frameon=True)

    margin = max(0.1, (max(ys) - min(ys)) * 0.25)
    ax.set_ylim(min(ys) - margin, max(ys) + margin)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    fig.tight_layout()

    output = ROOT / "log.png"
    fig.savefig(output)
    print(output)


if __name__ == "__main__":
    main()
