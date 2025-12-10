#!/usr/bin/env python3
"""
Plotting script for memory allocator benchmark results
Requires: matplotlib, pandas
Install with: pip3 install matplotlib pandas
"""

import sys
import os
import argparse
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib
matplotlib.use('Agg')

def plot_results(csv_file, output_file=None):
    """Plot benchmark results from CSV file"""
    
    # Check if file exists
    if not os.path.exists(csv_file):
        print(f"Error: File not found: {csv_file}")
        return False
    
    # Read CSV file
    try:
        df = pd.read_csv(csv_file)
    except Exception as e:
        print(f"Error reading CSV file: {e}")
        return False
    
    # Check required columns
    required_cols = ['Allocator', 'Benchmark', 'Ops_per_sec']
    if not all(col in df.columns for col in required_cols):
        print(f"Error: CSV file must contain columns: {required_cols}")
        return False
    
    # Create figure with subplots
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    
    # Plot 1: Operations per second by benchmark
    pivot_data = df.pivot(index='Benchmark', columns='Allocator', values='Ops_per_sec')
    pivot_data.plot(kind='bar', ax=ax1, rot=45)
    ax1.set_title('Operations per Second by Benchmark', fontsize=14, fontweight='bold')
    ax1.set_xlabel('Benchmark Type', fontsize=12)
    ax1.set_ylabel('Operations per Second', fontsize=12)
    ax1.legend(title='Allocator', fontsize=10)
    ax1.grid(True, alpha=0.3)
    
    # Plot 2: Time comparison
    pivot_time = df.pivot(index='Benchmark', columns='Allocator', values='Time_us')
    pivot_time.plot(kind='bar', ax=ax2, rot=45, color=['#1f77b4', '#ff7f0e'])
    ax2.set_title('Execution Time by Benchmark', fontsize=14, fontweight='bold')
    ax2.set_xlabel('Benchmark Type', fontsize=12)
    ax2.set_ylabel('Time (microseconds)', fontsize=12)
    ax2.legend(title='Allocator', fontsize=10)
    ax2.grid(True, alpha=0.3)
    
    # Adjust layout
    plt.tight_layout()
    
    # Save or show plot
    if output_file:
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"Plot saved to: {output_file}")
    else:
        # Generate default filename
        output_file = csv_file.replace('.csv', '.png')
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"Plot saved to: {output_file}")
    
    plt.close()
    return True

def plot_comparison(files, output_file='comparison.png'):
    """Plot comparison of multiple result files"""
    
    all_data = []
    for file in files:
        if not os.path.exists(file):
            print(f"Warning: File not found: {file}")
            continue
        try:
            df = pd.read_csv(file)
            all_data.append(df)
        except Exception as e:
            print(f"Warning: Error reading {file}: {e}")
    
    if not all_data:
        print("Error: No valid data files")
        return False
    
    # Combine all data
    combined = pd.concat(all_data, ignore_index=True)
    
    # Create comprehensive comparison plot
    fig = plt.figure(figsize=(16, 10))
    
    # Plot 1: Overall performance comparison
    ax1 = plt.subplot(2, 2, 1)
    pivot_ops = combined.pivot_table(
        index='Benchmark', 
        columns='Allocator', 
        values='Ops_per_sec', 
        aggfunc='mean'
    )
    pivot_ops.plot(kind='bar', ax=ax1, rot=45)
    ax1.set_title('Average Operations per Second', fontsize=12, fontweight='bold')
    ax1.set_xlabel('Benchmark Type')
    ax1.set_ylabel('Operations per Second')
    ax1.legend(title='Allocator', fontsize=9)
    ax1.grid(True, alpha=0.3)
    
    # Plot 2: Execution time comparison
    ax2 = plt.subplot(2, 2, 2)
    pivot_time = combined.pivot_table(
        index='Benchmark', 
        columns='Allocator', 
        values='Time_us', 
        aggfunc='mean'
    )
    pivot_time.plot(kind='bar', ax=ax2, rot=45)
    ax2.set_title('Average Execution Time', fontsize=12, fontweight='bold')
    ax2.set_xlabel('Benchmark Type')
    ax2.set_ylabel('Time (microseconds)')
    ax2.legend(title='Allocator', fontsize=9)
    ax2.grid(True, alpha=0.3)
    
    # Plot 3: Performance by allocator
    ax3 = plt.subplot(2, 2, 3)
    allocator_avg = combined.groupby('Allocator')['Ops_per_sec'].mean()
    allocator_avg.plot(kind='bar', ax=ax3, rot=45, color=['#2ca02c', '#d62728'])
    ax3.set_title('Overall Allocator Performance', fontsize=12, fontweight='bold')
    ax3.set_xlabel('Allocator')
    ax3.set_ylabel('Average Ops per Second')
    ax3.grid(True, alpha=0.3)
    
    # Plot 4: Summary statistics
    ax4 = plt.subplot(2, 2, 4)
    ax4.axis('off')
    
    # Calculate summary statistics
    summary_text = "Summary Statistics\n\n"
    for allocator in combined['Allocator'].unique():
        alloc_data = combined[combined['Allocator'] == allocator]
        avg_ops = alloc_data['Ops_per_sec'].mean()
        avg_time = alloc_data['Time_us'].mean()
        summary_text += f"{allocator}:\n"
        summary_text += f"  Avg Ops/sec: {avg_ops:,.0f}\n"
        summary_text += f"  Avg Time: {avg_time:,.0f} μs\n\n"
    
    ax4.text(0.1, 0.9, summary_text, transform=ax4.transAxes,
             fontsize=11, verticalalignment='top', family='monospace',
             bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))
    
    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Comparison plot saved to: {output_file}")
    plt.close()
    
    return True

def main():
    parser = argparse.ArgumentParser(
        description='Plot memory allocator benchmark results'
    )
    parser.add_argument(
        'input_files', 
        nargs='+', 
        help='CSV file(s) containing benchmark results'
    )
    parser.add_argument(
        '-o', '--output',
        help='Output image file (default: same as input with .png extension)'
    )
    parser.add_argument(
        '-c', '--comparison',
        action='store_true',
        help='Create comparison plot from multiple files'
    )
    
    args = parser.parse_args()
    
    if args.comparison and len(args.input_files) > 1:
        output = args.output if args.output else 'comparison.png'
        plot_comparison(args.input_files, output)
    elif len(args.input_files) == 1:
        plot_results(args.input_files[0], args.output)
    else:
        print("Error: For single file plotting, provide one input file")
        print("       For comparison, use -c/--comparison flag with multiple files")
        return 1
    
    return 0

if __name__ == '__main__':
    sys.exit(main())
