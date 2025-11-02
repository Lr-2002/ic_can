#!/usr/bin/env python3
"""
Simple Motor 6 Timing Analysis

Analyzes basic timing patterns of motor 6 commands to identify regular intervals
and potential issues causing "ka-ka-ka" motion.

Usage:
    python simple_motor6_timing.py [execution_log_path]
"""

import sys
import os
import re
import numpy as np

def extract_motor6_timestamps(log_path):
    """Extract just the timestamps from motor 6 commands."""

    timestamps = []
    send_counts = []

    try:
        with open(log_path, 'r') as f:
            for line_num, line in enumerate(f, 1):
                if "ID=0x00000006 (DM M6)" and "CAN SEND" in line:
                    # Extract timestamp using regex
                    match = re.search(r'(\d+\.\d+)ms:', line)
                    if match:
                        timestamp_ms = float(match.group(1))
                        timestamps.append(timestamp_ms)

                        # Extract send count
                        count_match = re.search(r'CAN SEND #(\d+)', line)
                        send_count = int(count_match.group(1)) if count_match else 0
                        send_counts.append(send_count)

    except FileNotFoundError:
        print(f"Error: Could not find execution log at {log_path}")
        return [], []

    return timestamps, send_counts

def analyze_basic_timing(timestamps, send_counts):
    """Perform basic timing analysis on motor 6 commands."""

    if len(timestamps) < 2:
        return {}

    # Basic statistics
    print(f"📊 BASIC ANALYSIS:")
    print(f"   Total commands: {len(timestamps)}")
    print(f"   Time range: {min(timestamps):.3f} - {max(timestamps):.3f} ms")

    # Look at timing patterns within each 1000ms block
    blocks = {}
    for i, ts in enumerate(timestamps):
        block_id = int(ts // 1000)  # 0-999ms = block 0, 1000-1999ms = block 1, etc.
        if block_id not in blocks:
            blocks[block_id] = []
        blocks[block_id].append((ts, send_counts[i] if i < len(send_counts) else 0))

    print(f"\n🕐 TIMING BLOCKS ANALYSIS:")
    print(f"   Found {len(blocks)} distinct timing blocks")

    intervals_in_blocks = []
    large_gaps = []

    for block_id, events in sorted(blocks.items()):
        if len(events) < 2:
            continue

        block_timestamps = [e[0] for e in sorted(events)]
        block_intervals = np.diff(block_timestamps)

        intervals_in_blocks.extend(block_intervals)

        # Check for large gaps within this block
        for i, interval in enumerate(block_intervals):
            if interval > 10.0:  # More than 10ms gap
                large_gaps.append({
                    'block': block_id,
                    'start_time': block_timestamps[i],
                    'interval_ms': interval,
                    'frequency_hz': 1000.0 / interval
                })

    if intervals_in_blocks:
        intervals_array = np.array(intervals_in_blocks)
        frequencies_array = 1000.0 / intervals_array

        print(f"\n📈 INTERVAL STATISTICS (within blocks):")
        print(f"   Total intervals analyzed: {len(intervals_array)}")
        print(f"   Average interval: {np.mean(intervals_array):.3f} ms")
        print(f"   Interval std dev: {np.std(intervals_array):.3f} ms")
        print(f"   Interval range: {np.min(intervals_array):.3f} - {np.max(intervals_array):.3f} ms")

        print(f"\n🔄 FREQUENCY STATISTICS:")
        print(f"   Average frequency: {np.mean(frequencies_array):.1f} Hz")
        print(f"   Frequency std dev: {np.std(frequencies_array):.1f} Hz")
        print(f"   Frequency range: {np.min(frequencies_array):.1f} - {np.max(frequencies_array):.1f} Hz")

        # Count intervals by ranges
        good_intervals = np.sum((intervals_array >= 1.5) & (intervals_array <= 3.0))  # 1.5-3ms
        slow_intervals = np.sum(intervals_array > 5.0)  # >5ms
        very_slow_intervals = np.sum(intervals_array > 20.0)  # >20ms

        print(f"\n⚡ INTERVAL QUALITY:")
        print(f"   Good intervals (1.5-3ms): {good_intervals}/{len(intervals_array)} ({100*good_intervals/len(intervals_array):.1f}%)")
        print(f"   Slow intervals (>5ms): {slow_intervals}/{len(intervals_array)} ({100*slow_intervals/len(intervals_array):.1f}%)")
        print(f"   Very slow intervals (>20ms): {very_slow_intervals}/{len(intervals_array)} ({100*very_slow_intervals/len(intervals_array):.1f}%)")

        if large_gaps:
            print(f"\n⚠️  LARGE GAPS DETECTED (>10ms):")
            print(f"   Total large gaps: {len(large_gaps)}")
            print(f"   {'Block':<6} {'Start(ms)':<10} {'Gap(ms)':<8} {'Freq(Hz)':<8}")
            print(f"   {'-'*40}")
            for gap in large_gaps[:10]:  # Show first 10
                print(f"   {gap['block']:<6} {gap['start_time']:<10.1f} {gap['interval_ms']:<8.3f} {gap['frequency_hz']:<8.1f}")

            if len(large_gaps) > 10:
                print(f"   ... and {len(large_gaps) - 10} more")

        # Assessment
        print(f"\n🎯 TIMING ASSESSMENT:")
        slow_percentage = 100.0 * slow_intervals / len(intervals_array)
        if slow_percentage > 20:
            print(f"   ❌ CRITICAL: {slow_percentage:.1f}% slow intervals detected!")
            print(f"   ❌ This explains the 'ka-ka-ka' jerky motion!")
            return "CRITICAL"
        elif slow_percentage > 5:
            print(f"   ⚠️  WARNING: {slow_percentage:.1f}% slow intervals detected")
            print(f"   ⚠️  May contribute to 'ka-ka-ka' motion")
            return "MEDIUM"
        else:
            print(f"   ✅ GOOD: {slow_percentage:.1f}% slow intervals only")
            print(f"   ✅ Timing appears consistent")
            return "GOOD"

    return "INSUFFICIENT_DATA"

def main():
    log_path = sys.argv[1] if len(sys.argv) > 1 else "build/execution_log"

    if not os.path.exists(log_path):
        print(f"Error: Execution log not found at {log_path}")
        print("Usage: python simple_motor6_timing.py [execution_log_path]")
        return 1

    print(f"🔍 Analyzing motor 6 timing from: {log_path}")

    # Extract timestamps
    timestamps, send_counts = extract_motor6_timestamps(log_path)

    if not timestamps:
        print("❌ No motor 6 commands found in execution log")
        return 1

    print(f"✅ Found {len(timestamps)} motor 6 command timestamps")

    # Analyze timing
    result = analyze_basic_timing(timestamps, send_counts)

    print(f"\n{'='*50}")
    print("CONCLUSION:")
    print(f"Motor 6 timing quality: {result}")
    print(f"{'='*50}")

    # Return exit code
    if result == "CRITICAL":
        return 2
    elif result == "MEDIUM":
        return 1
    else:
        return 0

if __name__ == "__main__":
    sys.exit(main())