#!/usr/bin/env python3
"""
Motor 6 Timing Analysis Tool

Analyzes timestamp differences for motor 6 commands from execution_log to identify
timing irregularities that could cause "ka-ka-ka" jerky motion.

Usage:
    python analyze_motor6_timing.py [execution_log_path]
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

def analyze_timing_patterns(commands):
    """Analyze timing patterns in motor 6 commands."""

    if len(commands) < 2:
        return {}

    # Calculate time differences between consecutive commands
    timestamps = [cmd['timestamp_ms'] for cmd in commands]
    time_diffs = np.diff(timestamps)

    # Convert to frequency (Hz)
    frequencies = 1000.0 / time_diffs  # Hz = 1000ms / time_diff_ms

    analysis = {
        'total_commands': len(commands),
        'duration_ms': timestamps[-1] - timestamps[0],
        'time_diffs': time_diffs,
        'frequencies': frequencies,
        'avg_frequency': np.mean(frequencies),
        'std_frequency': np.std(frequencies),
        'min_frequency': np.min(frequencies),
        'max_frequency': np.max(frequencies),
        'avg_interval_ms': np.mean(time_diffs),
        'std_interval_ms': np.std(time_diffs),
        'min_interval_ms': np.min(time_diffs),
        'max_interval_ms': np.max(time_diffs)
    }

    # Detect anomalies (intervals > 10ms or frequency < 100Hz)
    anomaly_threshold_low = 100.0  # Hz
    anomaly_threshold_high = 10.0  # ms
    anomalies = []

    for i, (time_diff, freq) in enumerate(zip(time_diffs, frequencies)):
        if freq < anomaly_threshold_low or time_diff > anomaly_threshold_high:
            anomalies.append({
                'index': i,
                'command_num': i + 1,
                'time_diff_ms': time_diff,
                'frequency_hz': freq,
                'timestamp_ms': timestamps[i + 1]
            })

    analysis['anomalies'] = anomalies
    analysis['anomaly_count'] = len(anomalies)
    analysis['anomaly_percentage'] = 100.0 * len(anomalies) / len(time_diffs)

    return analysis

def plot_timing_analysis(commands, analysis, save_path=None):
    """Plot timing analysis for motor 6 commands."""

    fig, axes = plt.subplots(3, 1, figsize=(14, 10))
    fig.suptitle('Motor 6 CAN Command Timing Analysis', fontsize=16, fontweight='bold')

    timestamps = [cmd['timestamp_ms'] / 1000.0 for cmd in commands]  # Convert to seconds

    # Plot 1: Command timestamps over time
    ax1 = axes[0]
    ax1.plot(timestamps, range(len(commands)), 'b-', linewidth=1, alpha=0.7)
    ax1.scatter(timestamps, range(len(commands)), c='blue', s=10, alpha=0.5)
    ax1.set_ylabel('Command Count', fontsize=12)
    ax1.set_title('Motor 6 Command Timeline', fontsize=12)
    ax1.grid(True, alpha=0.3)

    # Plot 2: Time intervals between commands
    ax2 = axes[1]
    intervals_s = analysis['time_diffs'] / 1000.0  # Convert to seconds
    ax2.plot(timestamps[1:], intervals_s, 'g-', linewidth=1, alpha=0.7)
    ax2.axhline(y=0.002, color='red', linestyle='--', alpha=0.7, label='Target (2ms = 500Hz)')
    ax2.axhline(y=0.01, color='orange', linestyle='--', alpha=0.7, label='Problem (>10ms)')
    ax2.set_ylabel('Interval (seconds)', fontsize=12)
    ax2.set_title(f'Time Intervals (Avg: {analysis["avg_interval_ms"]:.3f}ms ± {analysis["std_interval_ms"]:.3f}ms)', fontsize=12)
    ax2.legend()
    ax2.grid(True, alpha=0.3)

    # Highlight anomalies
    if analysis['anomalies']:
        anomaly_timestamps = [timestamps[anom['index'] + 1] for anom in analysis['anomalies']]
        anomaly_intervals = [anom['time_diff_ms'] / 1000.0 for anom in analysis['anomalies']]
        ax2.scatter(anomaly_timestamps, anomaly_intervals, c='red', s=30, alpha=0.8,
                   label=f'Anomalies ({len(analysis["anomalies"])})', zorder=5)
        ax2.legend()

    # Plot 3: Command frequency
    ax3 = axes[2]
    ax3.plot(timestamps[1:], analysis['frequencies'], 'r-', linewidth=1, alpha=0.7)
    ax3.axhline(y=500, color='green', linestyle='--', alpha=0.7, label='Target (500Hz)')
    ax3.axhline(y=100, color='orange', linestyle='--', alpha=0.7, label='Low (<100Hz)')
    ax3.set_ylabel('Frequency (Hz)', fontsize=12)
    ax3.set_xlabel('Time (seconds)', fontsize=12)
    ax3.set_title(f'Command Frequency (Avg: {analysis["avg_frequency"]:.1f} ± {analysis["std_frequency"]:.1f} Hz)', fontsize=12)
    ax3.legend()
    ax3.grid(True, alpha=0.3)

    plt.tight_layout()

    # Add statistics text box
    stats_text = f"""Timing Statistics:
Total Commands: {analysis['total_commands']}
Duration: {analysis['duration_ms']/1000:.1f}s
Avg Frequency: {analysis['avg_frequency']:.1f} Hz (target: 500 Hz)
Frequency Range: {analysis['min_frequency']:.1f} - {analysis['max_frequency']:.1f} Hz
Anomalies: {analysis['anomaly_count']} ({analysis['anomaly_percentage']:.1f}%)
Avg Interval: {analysis['avg_interval_ms']:.3f}ms (target: 2ms)"""

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
    print("MOTOR 6 TIMING ANALYSIS RESULTS")
    print(f"{'='*60}")

    print(f"📊 BASIC STATISTICS:")
    print(f"   Total Commands: {analysis['total_commands']}")
    print(f"   Duration: {analysis['duration_ms']/1000:.2f} seconds")
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
        print(f"\n🔍 TOP 10 ANOMALIES (Time Gaps >10ms or Frequency <100Hz):")
        print(f"   {'#':<3} {'Time(ms)':<10} {'Interval(ms)':<12} {'Frequency(Hz)':<12}")
        print(f"   {'-'*45}")

        for i, anom in enumerate(analysis['anomalies'][:10]):
            print(f"   {anom['command_num']:<3} {anom['timestamp_ms']:<10.1f} {anom['time_diff_ms']:<12.3f} {anom['frequency_hz']:<12.1f}")

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

    print(f"{'='*60}")

    return severity

def main():
    log_path = sys.argv[1] if len(sys.argv) > 1 else "build/execution_log"

    if not os.path.exists(log_path):
        print(f"Error: Execution log not found at {log_path}")
        print("Usage: python analyze_motor6_timing.py [execution_log_path]")
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

    # Generate plot
    plot_timing_analysis(commands, analysis, "motor6_timing_analysis.png")

    # Return exit code based on severity
    if severity == "CRITICAL":
        return 2
    elif severity == "MEDIUM":
        return 1
    else:
        return 0

if __name__ == "__main__":
    sys.exit(main())