#!/usr/bin/env python3
"""
Generate scaling plots from performance CSV files
Usage: python plot_scaling.py [results_dir]
"""

import os
import sys
import glob
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path

def read_performance_data(results_dir):
    """Read all performance CSV files"""
    csv_files = glob.glob(os.path.join(results_dir, "performance_*ranks.csv"))

    if not csv_files:
        print(f"No performance CSV files found in {results_dir}")
        return None

    data = []
    for csv_file in sorted(csv_files):
        df = pd.read_csv(csv_file, index_col='metric')
        data.append(df['value'].to_dict())

    return pd.DataFrame(data)

def plot_strong_scaling(df, output_dir):
    """Generate strong scaling plot"""
    if 'n_ranks' not in df.columns or 'total_time' not in df.columns:
        print("Missing required columns for strong scaling plot")
        return

    # Sort by number of ranks
    df = df.sort_values('n_ranks')

    ranks = df['n_ranks'].values
    times = df['total_time'].values

    # Calculate speedup (relative to smallest run)
    baseline_time = times[0]
    baseline_ranks = ranks[0]
    speedup = baseline_time / times

    # Calculate efficiency
    efficiency = (speedup / (ranks / baseline_ranks)) * 100

    # Create figure with two subplots
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))

    # Speedup plot
    ax1.plot(ranks, speedup, 'o-', linewidth=2, markersize=8, label='Actual')
    ax1.plot(ranks, ranks / baseline_ranks, '--', linewidth=2, label='Ideal (linear)')
    ax1.set_xlabel('Number of MPI Ranks', fontsize=12)
    ax1.set_ylabel('Speedup', fontsize=12)
    ax1.set_title('Strong Scaling - Speedup', fontsize=14, fontweight='bold')
    ax1.grid(True, alpha=0.3)
    ax1.legend(fontsize=11)
    ax1.set_xscale('log', base=2)
    ax1.set_yscale('log', base=2)

    # Efficiency plot
    ax2.plot(ranks, efficiency, 's-', linewidth=2, markersize=8, color='green')
    ax2.axhline(y=100, color='gray', linestyle='--', label='Ideal (100%)')
    ax2.axhline(y=60, color='red', linestyle=':', label='Target (60%)')
    ax2.set_xlabel('Number of MPI Ranks', fontsize=12)
    ax2.set_ylabel('Parallel Efficiency (%)', fontsize=12)
    ax2.set_title('Strong Scaling - Efficiency', fontsize=14, fontweight='bold')
    ax2.grid(True, alpha=0.3)
    ax2.legend(fontsize=11)
    ax2.set_xscale('log', base=2)
    ax2.set_ylim([0, 110])

    plt.tight_layout()
    output_file = os.path.join(output_dir, 'strong_scaling.png')
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Saved: {output_file}")
    plt.close()

def plot_throughput(df, output_dir):
    """Generate throughput plot"""
    if 'n_ranks' not in df.columns or 'throughput' not in df.columns:
        print("Missing required columns for throughput plot")
        return

    df = df.sort_values('n_ranks')

    fig, ax = plt.subplots(figsize=(10, 6))

    ax.plot(df['n_ranks'], df['throughput'], 'o-', linewidth=2, markersize=8)
    ax.set_xlabel('Number of MPI Ranks', fontsize=12)
    ax.set_ylabel('Throughput (particle-updates/s)', fontsize=12)
    ax.set_title('Computational Throughput', fontsize=14, fontweight='bold')
    ax.grid(True, alpha=0.3)
    ax.set_xscale('log', base=2)
    ax.set_yscale('log')

    # Format y-axis with scientific notation
    ax.ticklabel_format(style='scientific', axis='y', scilimits=(0,0))

    plt.tight_layout()
    output_file = os.path.join(output_dir, 'throughput.png')
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Saved: {output_file}")
    plt.close()

def plot_time_breakdown(df, output_dir):
    """Generate time breakdown stacked bar chart"""
    required_cols = ['n_ranks', 'compute_time', 'communication_time', 'tree_build_time']
    if not all(col in df.columns for col in required_cols):
        print("Missing required columns for time breakdown plot")
        return

    df = df.sort_values('n_ranks')

    # Calculate percentages
    total_time = df['total_time'].values
    compute_pct = (df['compute_time'] / total_time) * 100
    comm_pct = (df['communication_time'] / total_time) * 100
    tree_pct = (df['tree_build_time'] / total_time) * 100
    other_pct = 100 - compute_pct - comm_pct - tree_pct

    fig, ax = plt.subplots(figsize=(10, 6))

    x = np.arange(len(df))
    width = 0.6

    ax.bar(x, compute_pct, width, label='Compute (Force calc)', color='#2E7D32')
    ax.bar(x, comm_pct, width, bottom=compute_pct, label='Communication', color='#1976D2')
    ax.bar(x, tree_pct, width, bottom=compute_pct+comm_pct, label='Tree Build', color='#F57C00')
    ax.bar(x, other_pct, width, bottom=compute_pct+comm_pct+tree_pct, label='Other (I/O, etc.)', color='#7B1FA2')

    ax.set_ylabel('Percentage of Total Time (%)', fontsize=12)
    ax.set_xlabel('Number of MPI Ranks', fontsize=12)
    ax.set_title('Time Breakdown by Component', fontsize=14, fontweight='bold')
    ax.set_xticks(x)
    ax.set_xticklabels([int(r) for r in df['n_ranks']])
    ax.legend(fontsize=10, loc='upper left')
    ax.grid(True, alpha=0.3, axis='y')

    plt.tight_layout()
    output_file = os.path.join(output_dir, 'time_breakdown.png')
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Saved: {output_file}")
    plt.close()

def generate_summary_table(df, output_dir):
    """Generate summary table"""
    if 'n_ranks' not in df.columns:
        return

    df = df.sort_values('n_ranks')

    # Calculate additional metrics
    baseline_time = df['total_time'].iloc[0]
    baseline_ranks = df['n_ranks'].iloc[0]

    summary = pd.DataFrame({
        'Ranks': df['n_ranks'].astype(int),
        'Particles': df.get('n_particles', ['N/A']*len(df)),
        'Total Time (s)': df['total_time'].round(2),
        'Speedup': (baseline_time / df['total_time']).round(2),
        'Efficiency (%)': ((baseline_time / df['total_time']) / (df['n_ranks'] / baseline_ranks) * 100).round(1),
        'Throughput (p/s)': df.get('throughput', ['N/A']*len(df)),
    })

    # Save to CSV
    output_file = os.path.join(output_dir, 'scaling_summary.csv')
    summary.to_csv(output_file, index=False)
    print(f"Saved: {output_file}")

    # Print to console
    print("\n" + "="*70)
    print("SCALING SUMMARY")
    print("="*70)
    print(summary.to_string(index=False))
    print("="*70 + "\n")

def main():
    # Get results directory from command line or use default
    results_dir = sys.argv[1] if len(sys.argv) > 1 else 'results'

    if not os.path.exists(results_dir):
        print(f"Error: Directory '{results_dir}' not found")
        sys.exit(1)

    # Create output directory for plots
    output_dir = os.path.join(results_dir, 'plots')
    os.makedirs(output_dir, exist_ok=True)

    print(f"Reading performance data from: {results_dir}")
    df = read_performance_data(results_dir)

    if df is None or df.empty:
        print("No data to plot")
        sys.exit(1)

    print(f"Found {len(df)} performance measurements")

    # Generate plots
    print("\nGenerating plots...")
    plot_strong_scaling(df, output_dir)
    plot_throughput(df, output_dir)
    plot_time_breakdown(df, output_dir)
    generate_summary_table(df, output_dir)

    print(f"\nAll plots saved to: {output_dir}")
    print("\nDone!")

if __name__ == '__main__':
    main()
