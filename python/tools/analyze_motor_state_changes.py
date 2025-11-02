#!/usr/bin/env python3
"""
Motor State Change Analysis Tool

Analyzes how many times the actual motor state data changes for each motor
by examining the CSV logs. This helps identify which motors have stale data
or insufficient feedback updates.

Usage:
    python analyze_motor_state_changes.py <motor_states_csv_file>
"""

import sys
import os
import pandas as pd
import numpy as np
from datetime import datetime

def parse_timestamp(timestamp_str):
    """Parse ISO format timestamp string to datetime object."""
    try:
        if '.' in timestamp_str:
            base, ns = timestamp_str.split('.')
            ns = ns[:6]  # Keep only microseconds
            timestamp_str = f"{base}.{ns}"
        return datetime.fromisoformat(timestamp_str)
    except ValueError as e:
        print(f"Warning: Could not parse timestamp '{timestamp_str}': {e}")
        return datetime.now()

def load_motor_states(csv_file):
    """Load motor states from CSV file."""
    try:
        df = pd.read_csv(csv_file)
        print(f"✅ Loaded {len(df)} rows from {csv_file}")
        return df
    except Exception as e:
        print(f"❌ Failed to load CSV file: {e}")
        return None

def analyze_motor_state_changes(df):
    """Analyze motor state changes for all motors."""

    results = {}

    # Motor type information
    motor_types = {
        1: "DM10010L", 2: "DM6248", 3: "DM6248", 4: "DM4340", 5: "DM4340",
        6: "DM4310", 7: "HT4438", 8: "HT4438", 9: "SERVO"
    }

    print(f"\n🔍 Analyzing motor state changes for {len(df)} data points...")

    # Parse timestamps
    df['timestamp_parsed'] = df['timestamp'].apply(parse_timestamp)
    df['time_seconds'] = (df['timestamp_parsed'] - df['timestamp_parsed'].iloc[0]).dt.total_seconds()

    # Calculate overall data frequency
    duration = df['time_seconds'].iloc[-1] - df['time_seconds'].iloc[0]
    overall_frequency = (len(df) - 1) / duration if duration > 0 else 0

    print(f"\n📊 Overall Data:")
    print(f"   Duration: {duration:.3f} seconds")
    print(f"   Data points: {len(df)}")
    print(f"   Overall frequency: {overall_frequency:.1f} Hz")

    # Analyze each motor
    for motor_id in range(1, 10):
        position_col = f'position_motor_{motor_id}'
        velocity_col = f'velocity_motor_{motor_id}'
        torque_col = f'torque_motor_{motor_id}'

        if position_col not in df.columns:
            print(f"⚠️ Motor {motor_id} data not found in CSV")
            continue

        # Extract motor data
        positions = df[position_col].values
        velocities = df[velocity_col].values
        torques = df[torque_col].values

        # Calculate position changes
        position_changes = np.abs(np.diff(positions))
        velocity_changes = np.abs(np.diff(velocities))
        torque_changes = np.abs(np.diff(torques))

        # Count significant changes (using thresholds)
        position_threshold = 1e-6  # Very small threshold
        velocity_threshold = 1e-6
        torque_threshold = 1e-6

        position_change_count = np.sum(position_changes > position_threshold)
        velocity_change_count = np.sum(velocity_changes > velocity_threshold)
        torque_change_count = np.sum(torque_changes > torque_threshold)

        # Calculate change percentages
        total_intervals = len(position_changes)
        position_change_percentage = 100.0 * position_change_count / total_intervals if total_intervals > 0 else 0
        velocity_change_percentage = 100.0 * velocity_change_count / total_intervals if total_intervals > 0 else 0
        torque_change_percentage = 100.0 * torque_change_count / total_intervals if total_intervals > 0 else 0

        # Calculate effective update frequency for this motor
        effective_frequency = overall_frequency * position_change_percentage / 100.0

        # Store results
        results[motor_id] = {
            'motor_type': motor_types.get(motor_id, 'Unknown'),
            'total_data_points': len(positions),
            'position_changes': position_change_count,
            'velocity_changes': velocity_change_count,
            'torque_changes': torque_change_count,
            'position_change_percentage': position_change_percentage,
            'velocity_change_percentage': velocity_change_percentage,
            'torque_change_percentage': torque_change_percentage,
            'effective_frequency': effective_frequency,
            'max_position_change': np.max(position_changes),
            'avg_position_change': np.mean(position_changes),
            'position_range': np.max(positions) - np.min(positions),
            'max_velocity': np.max(np.abs(velocities)),
            'avg_velocity': np.mean(np.abs(velocities))
        }

        print(f"\n🦾 Motor {motor_id} ({motor_types.get(motor_id, 'Unknown')}):")
        print(f"   Data points: {len(positions)}")
        print(f"   Position changes: {position_change_count}/{total_intervals} ({position_change_percentage:.1f}%)")
        print(f"   Velocity changes: {velocity_change_count}/{total_intervals} ({velocity_change_percentage:.1f}%)")
        print(f"   Torque changes: {torque_change_count}/{total_intervals} ({torque_change_percentage:.1f}%)")
        print(f"   Effective update freq: {effective_frequency:.1f} Hz")
        print(f"   Position range: {results[motor_id]['position_range']:.4f} rad ({np.degrees(results[motor_id]['position_range']):.2f}°)")
        print(f"   Max position jump: {np.degrees(results[motor_id]['max_position_change']):.3f}°")
        print(f"   Avg position change: {np.degrees(results[motor_id]['avg_position_change']):.4f}°")
        print(f"   Max velocity: {results[motor_id]['max_velocity']:.4f} rad/s")
        print(f"   Avg velocity: {results[motor_id]['avg_velocity']:.4f} rad/s")

    return results, overall_frequency

def identify_problem_motors(results, overall_frequency):
    """Identify motors with data quality issues."""

    print(f"\n🚨 PROBLEM MOTOR ANALYSIS:")
    print(f"{'Motor':<6} {'Type':<10} {'Data%':<8} {'Freq(Hz)':<10} {'Issue':<20}")
    print(f"{'-'*60}")

    problem_motors = []

    for motor_id, data in results.items():
        issues = []

        # Check for low data change percentage
        if data['position_change_percentage'] < 50:
            issues.append("Stale data")
            problem_motors.append(motor_id)

        # Check for low effective frequency
        if data['effective_frequency'] < 100:
            issues.append("Low update rate")
            if motor_id not in problem_motors:
                problem_motors.append(motor_id)

        # Check for large position jumps (potential ka-ka-ka)
        if np.degrees(data['max_position_change']) > 5:
            issues.append("Large jumps")
            if motor_id not in problem_motors:
                problem_motors.append(motor_id)

        # Check for zero velocity changes (might indicate stuck motor)
        if data['velocity_change_percentage'] < 10 and data['avg_velocity'] < 0.01:
            issues.append("No motion")

        issue_str = ", ".join(issues) if issues else "OK"

        print(f"{motor_id:<6} {data['motor_type']:<10} {data['position_change_percentage']:<8.1f} {data['effective_frequency']:<10.1f} {issue_str:<20}")

    print(f"\n🎯 SUMMARY:")
    print(f"   Total motors analyzed: {len(results)}")
    print(f"   Motors with issues: {len(problem_motors)}")
    print(f"   Overall data frequency: {overall_frequency:.1f} Hz")

    if problem_motors:
        print(f"   Problem motors: {sorted(problem_motors)}")

        # Identify the most problematic motor
        worst_motor = min(results.keys(), key=lambda m: results[m]['position_change_percentage'])
        print(f"   WORST: Motor {worst_motor} ({results[worst_motor]['motor_type']}) - "
              f"only {results[worst_motor]['position_change_percentage']:.1f}% data freshness")
    else:
        print(f"   ✅ All motors have good data freshness")

    return problem_motors

def generate_recommendations(results, problem_motors):
    """Generate specific recommendations based on the analysis."""

    print(f"\n🔧 RECOMMENDATIONS:")

    if not problem_motors:
        print("   ✅ All motors are performing well - no action needed")
        return

    for motor_id in sorted(problem_motors):
        data = results[motor_id]

        print(f"\n   Motor {motor_id} ({data['motor_type']}):")

        if data['position_change_percentage'] < 50:
            print(f"     ⚠️  Stale data issue ({data['position_change_percentage']:.1f}% changes only)")
            print(f"     → Check CAN communication for this motor")
            print(f"     → Verify motor connection and wiring")

        if data['effective_frequency'] < 100:
            print(f"     ⚠️  Low update frequency ({data['effective_frequency']:.1f} Hz)")
            print(f"     → Increase control loop frequency for this motor")

        if np.degrees(data['max_position_change']) > 5:
            print(f"     ⚠️  Large position jumps detected ({np.degrees(data['max_position_change']):.1f}° max)")
            print(f"     → This explains 'ka-ka-ka' jerky motion")
            print(f"     → Reduce control gains or filter trajectory data")

    print(f"\n🎯 SYSTEM-LEVEL FIXES:")

    # Check if motor 6 is problematic
    if 6 in results and results[6]['position_change_percentage'] < 50:
        print(f"   🚨 Motor 6 (DM4310) shows stale data - primary cause of 'ka-ka-ka'")
        print(f"   → Fix motor 6 CAN communication priority")
        print(f"   → Increase motor 6 control gains from 30→120")

    # Check overall system issues
    low_freshness_motors = [m for m, d in results.items() if d['position_change_percentage'] < 70]
    if len(low_freshness_motors) > 3:
        print(f"   📡 Multiple motors show stale data - system-wide issue")
        print(f"   → Check CAN bus load and bandwidth")
        print(f"   → Reduce overall control frequency or optimize message ordering")

def main():
    if len(sys.argv) < 2:
        print("Usage: python analyze_motor_state_changes.py <motor_states_csv_file>")
        print("Example: python analyze_motor_state_changes.py build/arm_monitor_*/ic_can_log_*/motor_states.csv")
        return 1

    csv_file = sys.argv[1]

    # If directory is provided, find the motor_states.csv file
    if os.path.isdir(csv_file):
        for root, dirs, files in os.walk(csv_file):
            if 'motor_states.csv' in files:
                csv_file = os.path.join(root, 'motor_states.csv')
                break
        else:
            print(f"❌ motor_states.csv not found in directory: {sys.argv[1]}")
            return 1

    if not os.path.exists(csv_file):
        print(f"❌ File not found: {csv_file}")
        return 1

    print(f"🔍 Analyzing motor state changes from: {csv_file}")

    # Load and analyze data
    df = load_motor_states(csv_file)
    if df is None:
        return 1

    results, overall_frequency = analyze_motor_state_changes(df)

    # Identify problems
    problem_motors = identify_problem_motors(results, overall_frequency)

    # Generate recommendations
    generate_recommendations(results, problem_motors)

    # Return exit code based on severity
    if 6 in problem_motors and results[6]['position_change_percentage'] < 50:
        print(f"\n❌ CRITICAL: Motor 6 has severe data freshness issues - this explains 'ka-ka-ka'!")
        return 2
    elif len(problem_motors) > 0:
        print(f"\n⚠️  WARNING: {len(problem_motors)} motors have data quality issues")
        return 1
    else:
        print(f"\n✅ SUCCESS: All motors have good data freshness")
        return 0

if __name__ == "__main__":
    sys.exit(main())