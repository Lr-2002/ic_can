# Motor Trajectory Analysis Tools

This directory contains Python tools for analyzing motor trajectory data from IC_CAN logs.

## plot_motor_trajectory.py

Loads motor state data from IC_CAN log files and plots position trajectories for specific motors.

### Usage

```bash
cd /home/lr-2002/project/InstantCreation/ic_can

# Basic usage - plot motor 6 trajectory
/home/lr-2002/anaconda3/bin/python python/tools/plot_motor_trajectory.py build/arm_monitor_20251102_143324 6

# Show statistics only (no plot)
/home/lr-2002/anaconda3/bin/python python/tools/plot_motor_trajectory.py build/arm_monitor_20251102_143324 6 --stats-only

# Save plot to file
/home/lr-2002/anaconda3/bin/python python/tools/plot_motor_trajectory.py build/arm_monitor_20251102_143324 6 --save motor_6_plot.png

# Analyze different motors
/home/lr-2002/anaconda3/bin/python python/tools/plot_motor_trajectory.py build/arm_monitor_20251102_143324 1 --stats-only  # Motor 1 (DM10010L)
/home/lr-2002/anaconda3/bin/python python/tools/plot_motor_trajectory.py build/arm_monitor_20251102_143324 3 --stats-only  # Motor 3 (DM6248)
```

### Output

The script generates:

1. **Three-panel plot showing:**
   - Position trajectory (degrees) over time
   - Velocity profile (rad/s) over time
   - Position change analysis with large jump detection

2. **Detailed statistics:**
   - Duration and data points
   - Sampling rate (Hz)
   - Position range (radians and degrees)
   - Velocity statistics
   - Large jump detection (>5.7° threshold)
   - "Ka-Ka-Ka" severity assessment
   - Torque range

3. **Exit codes:**
   - 0: LOW severity (good trajectory)
   - 1: MEDIUM severity (some large jumps)
   - 2: HIGH severity (many large jumps, problematic trajectory)

### Motor Types

- **Motor 1**: DM10010L (Large arm motor)
- **Motors 2-3**: DM6248 (Medium arm motors)
- **Motors 4-5**: DM4340 (Small arm motors)
- **Motor 6**: DM4310 (Smallest arm motor - problematic one)
- **Motors 7-8**: HT4438 (Wrist motors)
- **Motor 9**: SERVO (Gripper motor)

### Analysis Features

- **Large Jump Detection**: Identifies position changes >5.7° that cause "ka-ka-ka" jerky motion
- **Trajectory Smoothness**: Analyzes position change patterns
- **Motor-Specific Analysis**: Tailored thresholds for different motor types
- **Time-Series Visualization**: Clear plots for trajectory debugging

## Dependencies

```bash
# Install required packages
/home/lr-2002/anaconda3/bin/pip install pandas matplotlib numpy
```

## Example Output

```
============================================================
Motor 6 (DM4310) Trajectory Statistics
============================================================
Duration: 15.805 seconds
Data Points: 7648
Sampling Rate: 483.9 Hz
Position Range: 0.6184 rad (35.43°)
Max Velocity: 1.8681 rad/s
Max Position Jump: 7.366°
Large Jumps (>5.7°): 1
Ka-Ka-Ka Severity: MEDIUM
============================================================
```

This tool is essential for debugging trajectory replay issues and identifying motors with problematic "ka-ka-ka" jerky motion patterns.