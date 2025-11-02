#!/usr/bin/env python3
"""
Motor 6 Timing Analysis Tool (Fixed Version)

Correctly handles timestamp wrap-around in execution log to analyze timing irregularities
that could cause "ka-ka-ka" jerky motion.

Usage:
    python analyze_motor6_timing_fixed.py [execution_log_path]
"""

import sys
import os
import re
from datetime import datetime
import matplotlib.pyplot as plt
import numpy as np

def extract_motor6_commands(log_path):
    """Extract motor 6 CAN send commands with timestamps from execution log."""

    motor6_commands = []

    try:
        with open(log_path, 'r') as f:
            for line_num, line in enumerate(f, 1):
                # Look for motor 6 CAN send commands
                if "ID=0x00000006 (DM M6)" and "CAN SEND" in line:
                    # Extract timestamp using regex
                    match = re.search(r'(\d+\.\d+)ms:', line)
                    if match:
                        timestamp_ms = float(match.group(1))
                        # Extract send count
                        count_match = re.search(r'CAN SEND #(\d+)', line)
                        send_count = int(count_match.group(1)) if count_match else 0
                        # Extract data bytes
                        data_match = re.search(r'Data: (.+)', line)
                        data = data_match.group(1) if data_match else ""

                        motor6_commands.append({
                            'line_num': line_num,
                            'timestamp_ms': timestamp_ms,
                            'send_count': send_count,
                            'data': data
                        })

    except FileNotFoundError:
        print(f"Error: Could not find execution log at {log_path}")
        return []
    except Exception as e:
        print(f"Error reading log file: {e}")
        return []

    return motor6_commands

def fix_timestamp_wraparound(timestamps):
    """
    Fix timestamp wrap-around by detecting when timestamps reset from large to small values.
    This assumes timestamps should be monotonically increasing.
    """

    if len(timestamps) < 2:
        return timestamps, 0

    fixed_timestamps = [timestamps[0]]
    wrap_count = 0
    expected_interval = 2.0  # Expected 2ms intervals for 500Hz

    for i in range(1, len(timestamps)):
        current = timestamps[i]
        previous = fixed_timestamps[-1]

        # Detect wrap-around: large decrease that doesn't match expected interval
        if current < previous - expected_interval * 2:
            # This is likely a wrap-around
            wrap_count += 1
            # Estimate the wrap period and adjust
            estimated_period = previous  # Assuming wrap from previous back to ~0
            corrected_time = previous + expected_interval
            fixed_timestamps.append(corrected_time)
            print(f"🔄 Wrap-around detected at index {i}: {previous:.3f}ms → {current:.3f}ms, corrected to {corrected_time:.3f}ms")
        else:
            fixed_timestamps.append(current)

    return fixed_timestamps, wrap_count

def analyze_timing_patterns(commands):
    """Analyze timing patterns in motor 6 commands."""

    if len(commands) < 2:
        return {}

    # Extract timestamps and fix wrap-around
    raw_timestamps = [cmd['timestamp_ms'] for cmd in commands]
    timestamps, wrap_count = fix_timestamp_wraparound(raw_timestamps)

    # Calculate time differences between consecutive commands
    time_diffs = np.diff(timestamps)

    # Filter out unrealistic values (should be around 2ms for 500Hz)
    realistic_mask = (time_diffs > 0.1) & (time_diffs < 50.0)  # Between 0.1ms and 50ms
    realistic_diffs = time_diffs[realistic_mask]
    realistic_timestamps = timestamps[:-1][realistic_mask]

    # Convert to frequency (Hz)
    if len(realistic_diffs) > 0:
        frequencies = 1000.0 / realistic_diffs  # Hz = 1000ms / time_diff_ms
    else:
        frequencies = np.array([500.0])  # Default to 500Hz

    analysis = {
        'total_commands': len(commands),
        'wrap_count': wrap_count,
        'raw_duration_ms': raw_timestamps[-1] - raw_timestamps[0],
        'fixed_duration_ms': timestamps[-1] - timestamps[0],
        'time_diffs': realistic_diffs,
        'frequencies': frequencies,
        'realistic_points': len(realistic_diffs),
        'avg_frequency': np.mean(frequencies),
        'std_frequency': np.std(frequencies),
        'min_frequency': np.min(frequencies),
        'max_frequency': np.max(frequencies),
        'avg_interval_ms': np.mean(realistic_diffs),
        'std_interval_ms': np.std(realistic_diffs),
        'min_interval_ms': np.min(realistic_diffs),
        'max_interval_ms': np.max(realistic_diffs)
    }

    # Detect anomalies (intervals > 5ms or frequency < 200Hz)
    anomaly_threshold_low = 200.0  # Hz
    anomaly_threshold_high = 5.0   # ms
    anomalies = []

    for i, (time_diff, freq, ts) in enumerate(zip(realistic_diffs, frequencies, realistic_timestamps)):
        if freq < anomaly_threshold_low or time_diff > anomaly_threshold_high:
            anomalies.append({
                'index': i,
                'command_num': i + 1,
                'time_diff_ms': time_diff,
                'frequency_hz': freq,
                'timestamp_ms': ts
            })

    analysis['anomalies'] = anomalies
    analysis['anomaly_count'] = len(anomalies)
    if len(realistic_diffs) > 0:
        analysis['anomaly_percentage'] = 100.0 * len(anomalies) / len(realistic_diffs)
    else:
        analysis['anomaly_percentage'] = 0.0

    return analysis

def plot_timing_analysis(commands, analysis, save_path=None):
    """Plot timing analysis for motor 6 commands."""

    if analysis['realistic_points'] == 0:
        print("❌ No realistic timing data to plot")
        return

    fig, axes = plt.subplots(3, 1, figsize=(14, 10))
    fig.suptitle('Motor 6 CAN Command Timing Analysis (Corrected)', fontsize=16, fontweight='bold')

    timestamps = [cmd['timestamp_ms'] for cmd in commands]

    # Plot 1: Raw timestamps showing wrap-around
    ax1 = axes[0]
    ax1.plot(range(len(timestamps)), timestamps, 'b-', linewidth=1, alpha=0.7, label='Raw timestamps')
    ax1.set_ylabel('Timestamp (ms)', fontsize=12)
    ax1.set_title(f'Raw Timestamps (Wrap-arounds: {analysis["wrap_count"]})', fontsize=12)
    ax1.grid(True, alpha=0.3)
    ax1.legend()

    # Plot 2: Time intervals between commands (realistic only)
    ax2 = axes[1]
    interval_indices = range(1, len(analysis['time_diffs']) + 1)
    ax2.plot(interval_indices, analysis['time_diffs'], 'g-', linewidth=1, alpha=0.7)
    ax2.axhline(y=2.0, color='red', linestyle='--', alpha=0.7, label='Target (2ms = 500Hz)')
    ax2.axhline(y=5.0, color='orange', linestyle='--', alpha=0.7, label='Problem (>5ms)')
    ax2.set_ylabel('Interval (ms)', fontsize=12)
    ax2.set_title(f'Time Intervals (Avg: {analysis["avg_interval_ms"]:.3f}ms ± {analysis["std_interval_ms"]:.3f}ms)', fontsize=12)
    ax2.legend()
    ax2.grid(True, alpha=0.3)

    # Highlight anomalies
    if analysis['anomalies']:
        anomaly_indices = [anom['command_num'] for anom in analysis['anomalies']]
        anomaly_intervals = [anom['time_diff_ms'] for anom in analysis['anomalies']]
        ax2.scatter(anomaly_indices, anomaly_intervals, c='red', s=30, alpha=0.8,
                   label=f'Anomalies ({len(analysis["anomalies"])})', zorder=5)
        ax2.legend()

    # Plot 3: Command frequency
    ax3 = axes[2]
    ax3.plot(interval_indices, analysis['frequencies'], 'r-', linewidth=1, alpha=0.7)
    ax3.axhline(y=500, color='green', linestyle='--', alpha=0.7, label='Target (500Hz)')
    ax3.axhline(y=200, color='orange', linestyle='--', alpha=0.7, label='Low (<200Hz)')
    ax3.set_ylabel('Frequency (Hz)', fontsize=12)
    ax3.set_xlabel('Command Index', fontsize=12)
    ax3.set_title(f'Command Frequency (Avg: {analysis["avg_frequency"]:.1f} ± {analysis["std_frequency"]:.1f} Hz)', fontsize=12)
    ax3.legend()
    ax3.grid(True, alpha=0.3)

    plt.tight_layout()

    # Add statistics text box
    stats_text = f"""Timing Statistics:
Total Commands: {analysis['total_commands']}
Wrap-arounds: {analysis['wrap_count']}
Realistic Intervals: {analysis['realistic_points']}/{len(analysis['time_diffs'])}
Avg Frequency: {analysis['avg_frequency']:.1f} Hz (target: 500 Hz)
Anomalies: {analysis['anomaly_count']} ({analysis['anomaly_percentage']:.1f}%)"""

    fig.text(0.02, 0.02, stats_text, fontsize=10, family='monospace',
             bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8))

    if save_path:
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        print(f"Timing analysis plot saved to: {save_path}")
    else:
        plt.show()

    plt.close()

def print_detailed_analysis(analysis):
    """Print detailed timing analysis results."""

    print(f"\n{'='*60}")
    print("MOTOR 6 TIMING ANALYSIS RESULTS (CORRECTED)")
    print(f"{'='*60}")

    print(f"📊 BASIC STATISTICS:")
    print(f"   Total Commands: {analysis['total_commands']}")
    print(f"   Timestamp Wrap-arounds: {analysis['wrap_count']}")
    print(f"   Realistic Intervals: {analysis['realistic_points']}/{analysis['total_commands']-1}")

    if analysis['realistic_points'] > 0:
        print(f"   Average Interval: {analysis['avg_interval_ms']:.3f} ms (target: 2.0 ms)")
        print(f"   Interval Std Dev: {analysis['std_interval_ms']:.3f} ms")
        print(f"   Interval Range: {analysis['min_interval_ms']:.3f} - {analysis['max_interval_ms']:.3f} ms")

        print(f"\n🔄 FREQUENCY ANALYSIS:")
        print(f"   Average Frequency: {analysis['avg_frequency']:.1f} Hz (target: 500 Hz)")
        print(f"   Frequency Std Dev: {analysis['std_frequency']:.1f} Hz")
        print(f"   Frequency Range: {analysis['min_frequency']:.1f} - {analysis['max_frequency']:.1f} Hz")

        print(f"\n⚠️  ANOMALY DETECTION:")
        print(f"   Anomalies Found: {analysis['anomaly_count']} ({analysis['anomaly_percentage']:.1f}%)")

        if analysis['anomalies']:
            print(f"\n🔍 TOP 10 ANOMALIES (Time Gaps >5ms or Frequency <200Hz):")
            print(f"   {'#':<3} {'Interval(ms)':<12} {'Frequency(Hz)':<12}")
            print(f"   {'-'*35}")

            for i, anom in enumerate(analysis['anomalies'][:10]):
                print(f"   {anom['command_num']:<3} {anom['time_diff_ms']:<12.3f} {anom['frequency_hz']:<12.1f}")

        print(f"\n🎯 KA-KA-KA ASSESSMENT:")
        if analysis['anomaly_percentage'] > 20:
            print(f"   ❌ CRITICAL: {analysis['anomaly_percentage']:.1f}% timing irregularities detected!")
            print(f"   ❌ This explains the severe 'ka-ka-ka' jerky motion!")
            severity = "CRITICAL"
        elif analysis['anomaly_percentage'] > 5:
            print(f"   ⚠️  WARNING: {analysis['anomaly_percentage']:.1f}% timing irregularities detected")
            print(f"   ⚠️  May contribute to 'ka-ka-ka' motion")
            severity = "MEDIUM"
        else:
            print(f"   ✅ GOOD: {analysis['anomaly_percentage']:.1f}% timing irregularities")
            print(f"   ✅ Timing is not the main cause of 'ka-ka-ka'")
            severity = "LOW"
    else:
        print(f"\n❌ ERROR: No realistic timing intervals found!")
        print(f"   All intervals were filtered out as unrealistic")
        severity = "ERROR"

    print(f"{'='*60}")

    return severity

def main():
    log_path = sys.argv[1] if len(sys.argv) > 1 else "build/execution_log"

    if not os.path.exists(log_path):
        print(f"Error: Execution log not found at {log_path}")
        print("Usage: python analyze_motor6_timing_fixed.py [execution_log_path]")
        return 1

    print(f"🔍 Analyzing motor 6 timing from: {log_path}")

    # Extract motor 6 commands
    commands = extract_motor6_commands(log_path)

    if not commands:
        print("❌ No motor 6 commands found in execution log")
        return 1

    print(f"✅ Found {len(commands)} motor 6 commands")

    # Analyze timing patterns
    analysis = analyze_timing_patterns(commands)

    # Print detailed results
    severity = print_detailed_analysis(analysis)

    # Generate plot if we have realistic data
    if analysis['realistic_points'] > 0:
        plot_timing_analysis(commands, analysis, "motor6_timing_analysis_fixed.png")
    else:
        print("⚠️  Skipping plot due to lack of realistic timing data")

    # Return exit code based on severity
    if severity == "CRITICAL":
        return 2
    elif severity == "MEDIUM":
        return 1
    elif severity == "ERROR":
        return 3
    else:
        return 0

if __name__ == "__main__":
    sys.exit(main())