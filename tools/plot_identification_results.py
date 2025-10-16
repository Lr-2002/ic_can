#!/usr/bin/env python3
"""
Dynamic Parameter Identification Results Plotter

This script visualizes the results from the dynamic identification tool,
including parameter values, validation plots, and R² scores.

Usage:
    python plot_identification_results.py [--results_dir identification_results]
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os
import argparse
import sys

def load_identification_results(results_dir):
    """Load identification results from CSV files."""
    try:
        # Load parameters
        params_file = os.path.join(results_dir, 'identified_parameters.csv')
        if not os.path.exists(params_file):
            print(f"❌ Parameters file not found: {params_file}")
            return None, None

        params_df = pd.read_csv(params_file)

        # Load validation results
        validation_file = os.path.join(results_dir, 'validation_results.csv')
        if not os.path.exists(validation_file):
            print(f"❌ Validation results file not found: {validation_file}")
            return None, None

        validation_df = pd.read_csv(validation_file)

        print(f"✅ Loaded {len(params_df)} parameters")
        print(f"✅ Loaded {len(validation_df)} validation samples")

        return params_df, validation_df

    except Exception as e:
        print(f"❌ Error loading results: {e}")
        return None, None

def plot_parameter_distribution(params_df, results_dir):
    """Plot distribution of identified parameters."""
    plt.figure(figsize=(12, 8))

    # Filter out very small parameters
    significant_params = params_df[abs(params_df['value']) > 1e-6]

    plt.subplot(2, 2, 1)
    plt.hist(significant_params['value'], bins=50, alpha=0.7, edgecolor='black')
    plt.title('Distribution of Significant Parameters')
    plt.xlabel('Parameter Value')
    plt.ylabel('Frequency')
    plt.yscale('log')
    plt.grid(True, alpha=0.3)

    plt.subplot(2, 2, 2)
    plt.plot(params_df['parameter_index'], params_df['value'], 'o', markersize=2, alpha=0.6)
    plt.title('Parameter Values vs Index')
    plt.xlabel('Parameter Index')
    plt.ylabel('Parameter Value')
    plt.grid(True, alpha=0.3)

    # Show top 10 largest magnitude parameters
    top_params = params_df.reindex(params_df['value'].abs().sort_values(ascending=False).index).head(10)

    plt.subplot(2, 1, 2)
    plt.bar(range(len(top_params)), top_params['value'])
    plt.title('Top 10 Parameters by Magnitude')
    plt.xlabel('Parameter Rank')
    plt.ylabel('Parameter Value')
    plt.xticks(range(len(top_params)), [f"Param {idx}" for idx in top_params['parameter_index']], rotation=45)
    plt.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig(os.path.join(results_dir, 'parameter_analysis.png'), dpi=300, bbox_inches='tight')
    print(f"📊 Parameter analysis plot saved to: {results_dir}/parameter_analysis.png")

def plot_validation_results(validation_df, results_dir):
    """Plot validation results for each joint."""
    num_joints = 6

    fig, axes = plt.subplots(2, 3, figsize=(15, 10))
    axes = axes.flatten()

    for joint in range(num_joints):
        joint_data = validation_df[validation_df['joint'] == joint]

        ax = axes[joint]
        ax.plot(joint_data['actual_torque'], label='Actual', alpha=0.7, linewidth=1)
        ax.plot(joint_data['predicted_torque'], label='Predicted', alpha=0.7, linewidth=1)
        ax.set_title(f'Joint {joint} Torque Validation')
        ax.set_xlabel('Sample Index')
        ax.set_ylabel('Torque')
        ax.legend()
        ax.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig(os.path.join(results_dir, 'validation_plots.png'), dpi=300, bbox_inches='tight')
    print(f"📊 Validation plots saved to: {results_dir}/validation_plots.png")

def plot_error_analysis(validation_df, results_dir):
    """Plot error analysis."""
    fig, axes = plt.subplots(2, 3, figsize=(15, 10))
    axes = axes.flatten()

    overall_errors = []

    for joint in range(6):
        joint_data = validation_df[validation_df['joint'] == joint]
        errors = joint_data['predicted_torque'] - joint_data['actual_torque']

        ax = axes[joint]
        ax.plot(errors, alpha=0.7, linewidth=1)
        ax.set_title(f'Joint {joint} Prediction Error')
        ax.set_xlabel('Sample Index')
        ax.set_ylabel('Error (Predicted - Actual)')
        ax.grid(True, alpha=0.3)
        ax.axhline(y=0, color='r', linestyle='--', alpha=0.5)

        overall_errors.extend(errors.tolist())

    plt.tight_layout()
    plt.savefig(os.path.join(results_dir, 'error_analysis.png'), dpi=300, bbox_inches='tight')
    print(f"📊 Error analysis plot saved to: {results_dir}/error_analysis.png")

    # Overall error statistics
    plt.figure(figsize=(10, 6))
    plt.hist(overall_errors, bins=50, alpha=0.7, edgecolor='black')
    plt.title('Overall Prediction Error Distribution')
    plt.xlabel('Error (Predicted - Actual)')
    plt.ylabel('Frequency')
    plt.grid(True, alpha=0.3)
    plt.savefig(os.path.join(results_dir, 'error_distribution.png'), dpi=300, bbox_inches='tight')
    print(f"📊 Error distribution plot saved to: {results_dir}/error_distribution.png")

def calculate_r2_scores(validation_df):
    """Calculate R² scores for each joint."""
    r2_scores = {}

    for joint in range(6):
        joint_data = validation_df[validation_df['joint'] == joint]
        actual = joint_data['actual_torque']
        predicted = joint_data['predicted_torque']

        # Calculate R²
        ss_res = ((actual - predicted) ** 2).sum()
        ss_tot = ((actual - actual.mean()) ** 2).sum()
        r2 = 1 - (ss_res / ss_tot) if ss_tot != 0 else 0

        r2_scores[f'joint_{joint}'] = r2

    return r2_scores

def print_identification_summary(params_df, validation_df, results_dir):
    """Print summary of identification results."""
    print("\n" + "="*60)
    print("DYNAMIC PARAMETER IDENTIFICATION SUMMARY")
    print("="*60)

    print(f"📊 Total parameters identified: {len(params_df)}")

    # Parameter statistics
    significant_params = params_df[abs(params_df['value']) > 1e-6]
    print(f"📈 Significant parameters (|value| > 1e-6): {len(significant_params)}")

    if len(significant_params) > 0:
        print(f"📐 Parameter value range: [{significant_params['value'].min():.6f}, {significant_params['value'].max():.6f}]")

    # R² scores
    r2_scores = calculate_r2_scores(validation_df)
    print(f"\n🎯 R² Scores by Joint:")
    for joint, r2 in r2_scores.items():
        status = "🎉 EXCELLENT" if r2 > 0.8 else "✅ GOOD" if r2 > 0.6 else "⚠️ MODERATE" if r2 > 0.4 else "❌ POOR"
        print(f"   {joint}: {r2:.4f} ({status})")

    overall_r2 = np.mean(list(r2_scores.values()))
    overall_status = "🎉 EXCELLENT" if overall_r2 > 0.8 else "✅ GOOD" if overall_r2 > 0.6 else "⚠️ MODERATE" if overall_r2 > 0.4 else "❌ POOR"
    print(f"\n📊 Overall R² Score: {overall_r2:.4f} ({overall_status})")

    print(f"\n📁 Results saved in: {results_dir}")
    print("   - parameter_analysis.png: Parameter distribution analysis")
    print("   - validation_plots.png: Torque validation for each joint")
    print("   - error_analysis.png: Prediction error analysis")
    print("   - error_distribution.png: Overall error distribution")

def main():
    parser = argparse.ArgumentParser(description='Plot dynamic identification results')
    parser.add_argument('--results_dir', default='identification_results',
                       help='Directory containing identification results')
    args = parser.parse_args()

    if not os.path.exists(args.results_dir):
        print(f"❌ Results directory not found: {args.results_dir}")
        sys.exit(1)

    print(f"🔄 Loading results from: {args.results_dir}")
    params_df, validation_df = load_identification_results(args.results_dir)

    if params_df is None or validation_df is None:
        print("❌ Failed to load results")
        sys.exit(1)

    print("📊 Generating plots...")

    # Generate plots
    plot_parameter_distribution(params_df, args.results_dir)
    plot_validation_results(validation_df, args.results_dir)
    plot_error_analysis(validation_df, args.results_dir)

    # Print summary
    print_identification_summary(params_df, validation_df, args.results_dir)

    print(f"\n🎉 Plotting completed successfully!")
    print(f"📊 Check '{args.results_dir}' for all generated plots")

if __name__ == "__main__":
    main()