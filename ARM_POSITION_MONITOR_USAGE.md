# Arm Position Monitor Usage Guide

## Overview
I've created two programs for monitoring the 6-joint arm positions in real-time using the working dm-tools SDK integration:

## 1. Real-time Position Monitor (`arm_position_monitor`)

**Purpose**: Continuous real-time monitoring of all 6 arm joint positions with live display updates.

**Features**:
- Real-time display updating every 100ms
- Shows positions in both radians and degrees
- Visual indicators for connection status (✅ updated, ⏸️ not updated)
- Clean terminal display with automatic screen clearing
- Shows arm configuration with motor types and limits

**Usage**:
```bash
./arm_position_monitor
```

**Output Example**:
```
=== 6-Joint Arm Position Monitor ===
Real-time position monitoring for motors 1-6
Press Ctrl+C to stop

Joint | Motor | Position (rad) | Position (deg) | Updated
------|-------|----------------|----------------|--------
    1 | M1    |         6.645 |          380.7 | ✅
    2 | M2    |        -0.123 |           -7.0 | ✅
    3 | M3    |         0.456 |          26.1 | ✅
    4 | M4    |        -0.789 |         -45.2 | ✅
    5 | M5    |         1.234 |          70.7 | ✅
    6 | M6    |        -2.567 |         -147.0 | ✅

Arm Configuration:
  Joint 1 (M1): DM10010L - ±12.5 rad, 25 rad/s, 200 Nm
  Joint 2 (M2): DM6248   - ±12.566 rad, 20 rad/s, 120 Nm
  Joint 3 (M3): DM6248   - ±12.566 rad, 20 rad/s, 120 Nm
  Joint 4 (M4): DM4340   - ±12.5 rad, 10 rad/s, 28 Nm
  Joint 5 (M5): DM4340   - ±12.5 rad, 10 rad/s, 28 Nm
  Joint 6 (M6): DM4310   - ±12.5 rad, 30 rad/s, 10 Nm
```

## 2. Simple Position Reader (`get_arm_positions`)

**Purpose**: Demonstrates a simple function that returns current arm positions as a vector.

**Features**:
- `get_arm_positions()` function that returns `std::vector<float>` of 6 joint positions
- Simple demonstration with 5 readings spaced 2 seconds apart
- Shows how to integrate position reading into other programs
- Clean output format

**Usage**:
```bash
./get_arm_positions
```

**Function Signature**:
```cpp
std::vector<float> get_arm_positions(usb_class* device)
```

**Returns**: Vector of 6 floats (positions in radians for joints 1-6)

## Motor Configuration

The monitor correctly handles all 6 arm joint motors with their specific types:

| Joint | Motor | Type     | Position Limits | Velocity | Torque |
|-------|-------|----------|----------------|----------|--------|
| 1     | M1    | DM10010L  | ±12.5 rad      | 25 rad/s | 200 Nm |
| 2     | M2    | DM6248    | ±12.566 rad    | 20 rad/s | 120 Nm |
| 3     | M3    | DM6248    | ±12.566 rad    | 20 rad/s | 120 Nm |
| 4     | M4    | DM4340    | ±12.5 rad      | 10 rad/s | 28 Nm  |
| 5     | M5    | DM4340    | ±12.5 rad      | 10 rad/s | 28 Nm  |
| 6     | M6    | DM4310    | ±12.5 rad      | 30 rad/s | 10 Nm  |

## Technical Details

### How It Works
1. **Connection**: Uses dm-tools SDK to connect to USB2CAN device
2. **Callback**: Registers `arm_feedback_callback` to receive motor feedback
3. **Motor Enable**: Sends enable commands to all 6 motors
4. **Position Reading**: Uses status requests to trigger position feedback
5. **Data Conversion**: Applies correct motor limits for each joint type
6. **Display**: Shows real-time positions in both radians and degrees

### Key Functions

**Real-time Monitor**:
- `arm_feedback_callback()` - Handles incoming CAN data for motors 1-6
- `print_arm_positions()` - Updates the display every 100ms
- `enable_arm_motors()` - Enables all 6 arm motors
- `request_status_all()` - Sends broadcast status request

**Simple Reader**:
- `get_arm_positions()` - Main function that returns current positions
- `print_positions()` - Formats and displays position data

### Integration Examples

**Basic Integration**:
```cpp
#include "dm-tools/USB2FDCAN/SDK/C++/ubuntu/include/protocol/usb_class.h"

// Set up callback and device
usb_class *device = new usb_class(1000000, 5000000, device_sn);
device->setFrameCallback(arm_feedback_callback);
device->USB_CMD_START_CAP();

// Get current positions
auto positions = get_arm_positions(device);
for (int i = 0; i < 6; i++) {
    std::cout << "Joint " << (i+1) << ": " << positions[i] << " rad" << std::endl;
}
```

**Continuous Monitoring**:
```cpp
while (running) {
    auto positions = get_arm_positions(device);
    // Process positions (e.g., send to GUI, log to file, etc.)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
```

## Troubleshooting

### Common Issues

1. **No Position Updates**
   - Check if motors are enabled
   - Verify USB2CAN device connection
   - Ensure motor IDs (1-6) are correct

2. **Compilation Issues**
   - Ensure dm-tools SDK paths are correct
   - Check that libu2canfd and libusb-1.0 are installed
   - Verify C++17 compiler support

3. **Permission Issues**
   - May need sudo access for USB device
   - Add user to appropriate groups for USB access

### Debug Options

**Enable Verbose Output**: Uncomment the detailed feedback line in `arm_feedback_callback()`:
```cpp
std::cout << "📥 Motor " << motor_id << ": p=" << position << " rad..." << std::endl;
```

**Check Connection**: Verify device connection and status:
```bash
lsusb | grep -i usb
```

## Performance

- **Update Rate**: 100ms for real-time monitor (10 Hz)
- **Response Time**: < 3 seconds timeout for position reading
- **CPU Usage**: Low (sleep-based timing)
- **Memory**: Minimal (static arrays for position storage)

## Next Steps

These position monitoring tools provide the foundation for:

1. **GUI Integration**: Feed position data to visualization interfaces
2. **Motion Recording**: Log joint trajectories for playback
3. **Safety Monitoring**: Real-time position limit checking
4. **Calibration**: Initial position reading and zeroing
5. **500Hz Control**: High-frequency position feedback integration

Both programs are ready to use and can serve as building blocks for more complex arm control and monitoring applications.