#!/usr/bin/env python3
"""
Compares Dijkstra vs greedy geographic routing performance using the CSVs
produced by ./build/metrics_demo (results/dijkstra_results.csv and
results/greedy_results.csv).

Produces results/comparison.png with three panels:
  1. Latency distribution (delivered packets only)
  2. Hop count distribution (delivered packets only)
  3. Delivery rate bar chart

Usage (from the repo root, after running ./build/metrics_demo):
    python3 scripts/plot_results.py

Requires matplotlib: pip install matplotlib
"""

import csv
import sys
from pathlib import Path

try:
    import matplotlib.pyplot as plt
except ImportError:
    print("This script requires matplotlib. Install it with:")
    print("  pip install matplotlib")
    sys.exit(1)


def load_records(csv_path):
    """Reads a results CSV into a list of dicts with the right types."""
    records = []
    with open(csv_path, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            records.append({
                "packet_id": int(row["packet_id"]),
                "delivered": row["delivered"] == "1",
                "latency_ms": float(row["latency_ms"]),
                "hop_count": int(row["hop_count"]),
            })
    return records


def summarize(records, label):
    total = len(records)
    delivered = [r for r in records if r["delivered"]]
    delivery_rate = (len(delivered) / total * 100.0) if total > 0 else 0.0
    avg_latency = (sum(r["latency_ms"] for r in delivered) / len(delivered)) if delivered else 0.0

    print(f"{label}:")
    print(f"  Packets: {total}  Delivered: {len(delivered)}  Delivery rate: {delivery_rate:.1f}%")
    print(f"  Average latency: {avg_latency:.2f} ms")
    print()

    return delivery_rate, delivered


def main():
    results_dir = Path("results")
    dijkstra_path = results_dir / "dijkstra_results.csv"
    greedy_path = results_dir / "greedy_results.csv"

    if not dijkstra_path.exists() or not greedy_path.exists():
        print(f"Expected CSVs not found at {dijkstra_path} and {greedy_path}.")
        print("Run ./build/metrics_demo first to generate them.")
        sys.exit(1)

    dijkstra_records = load_records(dijkstra_path)
    greedy_records = load_records(greedy_path)

    dijkstra_rate, dijkstra_delivered = summarize(dijkstra_records, "Dijkstra")
    greedy_rate, greedy_delivered = summarize(greedy_records, "Greedy geographic")

    fig, axes = plt.subplots(1, 3, figsize=(15, 4.5))

    # Panel 1: latency distribution
    dijkstra_latencies = [r["latency_ms"] for r in dijkstra_delivered]
    greedy_latencies = [r["latency_ms"] for r in greedy_delivered]

    axes[0].hist(dijkstra_latencies, bins=30, alpha=0.6, label="Dijkstra")
    axes[0].hist(greedy_latencies, bins=30, alpha=0.6, label="Greedy")
    axes[0].set_xlabel("Latency (ms)")
    axes[0].set_ylabel("Packet count")
    axes[0].set_title("Latency distribution (delivered packets)")
    axes[0].legend()

    # Panel 2: hop count distribution
    dijkstra_hops = [r["hop_count"] for r in dijkstra_delivered]
    greedy_hops = [r["hop_count"] for r in greedy_delivered]

    max_hops = max(dijkstra_hops + greedy_hops, default=1)
    bins = range(0, max_hops + 2)

    axes[1].hist(dijkstra_hops, bins=bins, alpha=0.6, label="Dijkstra")
    axes[1].hist(greedy_hops, bins=bins, alpha=0.6, label="Greedy")
    axes[1].set_xlabel("Hop count")
    axes[1].set_ylabel("Packet count")
    axes[1].set_title("Hop count distribution (delivered packets)")
    axes[1].legend()

    # Panel 3: delivery rate comparison
    axes[2].bar(["Dijkstra", "Greedy"], [dijkstra_rate, greedy_rate],
                color=["#4C72B0", "#DD8452"])
    axes[2].set_ylabel("Delivery rate (%)")
    axes[2].set_title("Delivery rate comparison")
    axes[2].set_ylim(0, 100)

    fig.tight_layout()

    output_path = results_dir / "comparison.png"
    fig.savefig(output_path, dpi=150)
    print(f"Saved {output_path}")


if __name__ == "__main__":
    main()
