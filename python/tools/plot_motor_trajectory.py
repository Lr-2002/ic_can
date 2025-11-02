#!/usr/bin/env python3
"""
Motor Trajectory Plotting Tool

This script loads motor state data from IC_CAN log files and plots the position trajectory
for a specific motor ID.

Usage:
    python plot_motor_trajectory.py <log_directory> <motor_id> [options]

Example:
    python plot_motor_trajectory.py arm_monitor_20251102_143324 6
    python plot_motor_trajectory.py arm_monitor_20251102_143324 6 --save plot.png
"""

import argparse
import os
import sys
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from datetime import datetime
from typing import Tuple, Optional

# Motor type information for labeling
MOTOR_TYPES = {
    1: "DM10010L",
    2: "DM6248",
    3: "DM6248",
    4: "DM4340",
    5: "DM4340",
    6: "DM4310",
    7: "HT4438",
    8: "HT4438",
    9: "SERVO"
}

def parse_timestamp(timestamp_str: str) -> datetime:
    """Parse ISO format timestamp string to datetime object."""
    try:
        # Remove nanoseconds if present
        if '.' in timestamp_str:
            base, ns = timestamp_str.split('.')
            ns = ns[:6]  # Keep only microseconds
            timestamp_str = f"{base}.{ns}"
        return datetime.fromisoformat(timestamp_str)
    except ValueError as e:
        print(f"Warning: Could not parse timestamp '{timestamp_str}': {e}")
        return datetime.now()

def load_motor_states(log_dir: str) -> Tuple[pd.DataFrame, Optional[str]]:
    """
    Load motor states from CSV file in the specified log directory.

    Args:
        log_dir: Path to the log directory containing motor_states.csv

    Returns:
        Tuple of (DataFrame with motor data, path to CSV file)
    """
    # Find the motor_states.csv file
    motor_states_file = None
    for root, dirs, files in os.walk(log_dir):
        if 'motor_states.csv' in files:
            motor_states_file = os.path.join(root, 'motor_states.csv')
            break

    if motor_states_file is None:
        raise FileNotFoundError(f"motor_states.csv not found in {log_dir}")

    print(f"Loading motor states from: {motor_states_file}")

    # Load CSV with error handling
    try:
        df = pd.read_csv(motor_states_file)
        print(f"Loaded {len(df)} data rows")
        return df, motor_states_file
    except Exception as e:
        raise RuntimeError(f"Failed to load CSV file: {e}")

def process_motor_data(df: pd.DataFrame, motor_id: int) -> pd.DataFrame:
    """
    Process motor data for the specified motor ID.

    Args:
        df: Raw motor states DataFrame
        motor_id: Motor ID (1-9)

    Returns:
        Processed DataFrame with time, position, velocity, torque
    """
    if motor_id < 1 or motor_id > 9:
        raise ValueError(f"Motor ID must be between 1 and 9, got {motor_id}")

    # Extract columns for the specified motor
    position_col = f'position_motor_{motor_id}'
    velocity_col = f'velocity_motor_{motor_id}'
    torque_col = f'torque_motor_{motor_id}'

    # Check if columns exist
    missing_cols = [col for col in [position_col, velocity_col, torque_col] if col not in df.columns]
    if missing_cols:
        raise ValueError(f"Missing columns in CSV: {missing_cols}")

    # Create processed DataFrame
    motor_df = pd.DataFrame({
        'timestamp_raw': df['timestamp'],
        'position': df[position_col],
        'velocity': df[velocity_col],
        'torque': df[torque_col]
    })

    # Parse timestamps and calculate relative time
    motor_df['timestamp'] = motor_df['timestamp_raw'].apply(parse_timestamp)
    motor_df['time_seconds'] = (motor_df['timestamp'] - motor_df['timestamp'].iloc[0]).dt.total_seconds()

    # Convert position from radians to degrees for better visualization
    motor_df['position_deg'] = np.degrees(motor_df['position'])

    # Calculate additional metrics
    motor_df['position_change'] = motor_df['position'].diff().abs()
    motor_df['velocity_change'] = motor_df['velocity'].diff().abs()

    return motor_df

def analyze_motor_trajectory(motor_df: pd.DataFrame, motor_id: int) -> dict:
    """
    Analyze motor trajectory and return statistics.

    Args:
        motor_df: Processed motor data DataFrame
        motor_id: Motor ID

    Returns:
        Dictionary with trajectory statistics
    """
    stats = {
        'motor_id': motor_id,
        'motor_type': MOTOR_TYPES.get(motor_id, 'Unknown'),
        'total_duration': motor_df['time_seconds'].iloc[-1] - motor_df['time_seconds'].iloc[0],
        'data_points': len(motor_df),
        'sampling_rate': len(motor_df) / (motor_df['time_seconds'].iloc[-1] - motor_df['time_seconds'].iloc[0]),
        'position_range_rad': motor_df['position'].max() - motor_df['position'].min(),
        'position_range_deg': motor_df['position_deg'].max() - motor_df['position_deg'].min(),
        'max_velocity': motor_df['velocity'].abs().max(),
        'avg_velocity': motor_df['velocity'].abs().mean(),
        'max_position_jump': motor_df['position_change'].max(),
        'avg_position_jump': motor_df['position_change'].mean(),
        'large_jumps_count': (motor_df['position_change'] > 0.1).sum(),  # > 0.1 rad ~ 5.7°
        'torque_stats': {
            'max': motor_df['torque'].max(),
            'min': motor_df['torque'].min(),
            'avg': motor_df['torque'].abs().mean()
        }
    }

    # Detect potential "ka-ka-ka" patterns (large position changes)
    if stats['large_jumps_count'] > 0:
        stats['ka_ka_ka_severity'] = 'HIGH' if stats['large_jumps_count'] > 10 else 'MEDIUM'
    else:
        stats['ka_ka_ka_severity'] = 'LOW'

    return stats

def plot_motor_trajectory(motor_df: pd.DataFrame, motor_id: int, save_path: Optional[str] = None) -> None:
    """
    Plot motor position trajectory with analysis.

    Args:
        motor_df: Processed motor data DataFrame
        motor_id: Motor ID for plot title
        save_path: Optional path to save the plot
    """
    # Get analysis statistics
    stats = analyze_motor_trajectory(motor_df, motor_id)

    # Create figure with subplots
    fig, axes = plt.subplots(3, 1, figsize=(14, 10))
    fig.suptitle(f'Motor {motor_id} ({stats["motor_type"]}) Trajectory Analysis', fontsize=16, fontweight='bold')

    # Plot 1: Position over time (degrees)
    ax1 = axes[0]
    ax1.plot(motor_df['time_seconds'], motor_df['position_deg'], 'b-', linewidth=1.5, label='Position')
    ax1.set_ylabel('Position (degrees)', fontsize=12)
    ax1.set_title(f'Position Trajectory (Range: {stats["position_range_deg"]:.2f}°)', fontsize=12)
    ax1.grid(True, alpha=0.3)
    ax1.legend()

    # Highlight large position jumps
    large_jumps = motor_df[motor_df['position_change'] > 0.1]
    if len(large_jumps) > 0:
        ax1.scatter(large_jumps['time_seconds'], large_jumps['position_deg'],
                   color='red', s=50, alpha=0.7, label='Large Jumps (>5.7°)', zorder=5)
        ax1.legend()

    # Plot 2: Velocity over time
    ax2 = axes[1]
    ax2.plot(motor_df['time_seconds'], motor_df['velocity'], 'g-', linewidth=1.5, label='Velocity')
    ax2.set_ylabel('Velocity (rad/s)', fontsize=12)
    ax2.set_title(f'Velocity Profile (Max: {stats["max_velocity"]:.3f} rad/s, Avg: {stats["avg_velocity"]:.3f} rad/s)', fontsize=12)
    ax2.grid(True, alpha=0.3)
    ax2.legend()
    ax2.axhline(y=0, color='k', linestyle='--', alpha=0.3)

    # Plot 3: Position change analysis
    ax3 = axes[2]
    ax3.plot(motor_df['time_seconds'], np.degrees(motor_df['position_change']), 'r-', linewidth=1.0, alpha=0.7, label='Position Change')
    ax3.set_ylabel('Position Change (degrees)', fontsize=12)
    ax3.set_xlabel('Time (seconds)', fontsize=12)
    ax3.set_title(f'Position Change Analysis (Max: {np.degrees(stats["max_position_jump"]):.2f}°, Large Jumps: {stats["large_jumps_count"]})', fontsize=12)
    ax3.grid(True, alpha=0.3)
    ax3.legend()
    ax3.axhline(y=5.7, color='orange', linestyle='--', alpha=0.7, label='5.7° threshold')

    # Adjust layout
    plt.tight_layout()

    # Add statistics text box
    stats_text = f"""Trajectory Statistics:
Duration: {stats['total_duration']:.2f}s | Data Points: {stats['data_points']} | Rate: {stats['sampling_rate']:.1f} Hz
Position Range: {stats['position_range_deg']:.2f}° | Max Velocity: {stats['max_velocity']:.3f} rad/s
Large Jumps: {stats['large_jumps_count']} | Ka-Ka-Ka: {stats['ka_ka_ka_severity']}"""

    fig.text(0.02, 0.02, stats_text, fontsize=10, family='monospace',
             bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8))

    # Save or show plot
    if save_path:
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        print(f"Plot saved to: {save_path}")
    else:
        plt.show()

    plt.close()

def main():
    parser = argparse.ArgumentParser(description='Plot motor trajectory from IC_CAN log data')
    parser.add_argument('log_directory', help='Path to the log directory containing motor_states.csv')
    parser.add_argument('motor_id', type=int, help='Motor ID to plot (1-9)')
    parser.add_argument('--save', help='Save plot to specified file')
    parser.add_argument('--stats-only', action='store_true', help='Only show statistics, no plot')

    args = parser.parse_args()

    try:
        # Load motor states
        df, csv_path = load_motor_states(args.log_directory)

        # Process data for specified motor
        motor_df = process_motor_data(df, args.motor_id)

        # Analyze trajectory
        stats = analyze_motor_trajectory(motor_df, args.motor_id)

        # Print statistics
        print(f"\n{'='*60}")
        print(f"Motor {args.motor_id} ({stats['motor_type']}) Trajectory Statistics")
        print(f"{'='*60}")
        print(f"Data Source: {csv_path}")
        print(f"Duration: {stats['total_duration']:.3f} seconds")
        print(f"Data Points: {stats['data_points']}")
        print(f"Sampling Rate: {stats['sampling_rate']:.1f} Hz")
        print(f"Position Range: {stats['position_range_rad']:.4f} rad ({stats['position_range_deg']:.2f}°)")
        print(f"Max Velocity: {stats['max_velocity']:.4f} rad/s")
        print(f"Avg Velocity: {stats['avg_velocity']:.4f} rad/s")
        print(f"Max Position Jump: {np.degrees(stats['max_position_jump']):.3f}°")
        print(f"Avg Position Jump: {np.degrees(stats['avg_position_jump']):.3f}°")
        print(f"Large Jumps (>5.7°): {stats['large_jumps_count']}")
        print(f"Ka-Ka-Ka Severity: {stats['ka_ka_ka_severity']}")
        print(f"Torque Range: {stats['torque_stats']['min']:.3f} to {stats['torque_stats']['max']:.3f} Nm")
        print(f"{'='*60}")

        # Plot trajectory if not stats-only
        if not args.stats_only:
            plot_motor_trajectory(motor_df, args.motor_id, args.save)

        # Return exit code based on ka-ka-ka severity
        if stats['ka_ka_ka_severity'] == 'HIGH':
            return 2  # High severity
        elif stats['ka_ka_ka_severity'] == 'MEDIUM':
            return 1  # Medium severity
        else:
            return 0  # Low severity

    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

if __name__ == "__main__":
    sys.exit(main())