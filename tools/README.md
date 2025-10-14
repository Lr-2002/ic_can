# IC_CAN Tools

This directory contains utility tools for the IC_CAN system.

## Available Tools

### set_motor_zero

Interactive tool to set motor zero position calibration or move motors to zero position.

#### Features:
- **Display all motor positions** in both radians and degrees
- **Interactive motor selection** (1-9)
- **Zero position calibration** (sets current position as 0.0)
- **All motor movement to zero** (moves all motors to position 0.0)
- **Real-time monitoring** of calibration results
- **Confirmation prompts** for safety

#### Zero Calibration vs Movement:
- **Motor 1-9**: Sets **zero position calibration** (current position becomes new 0.0)
- **All motors command**: Moves all motors to **position 0.0** (physical movement)

#### Example Session:
```
=== IC_CAN Motor Zero Setting Tool ===
✅ System initialized

📊 Current Motor Positions:
======================================================================
Motor | Position (rad) | Position (deg) | Type      | Status
------|---------------|----------------|-----------|--------
    6 |       0.8340 |         47.79 | Damiao    | OFFSET
======================================================================

> 6
🎯 Setting Motor 6 zero position calibration...
Current position: 0.8340 rad (47.79°)
⚠️  This will set the CURRENT position as the new ZERO calibration
📝 After calibration, this position will be reported as 0.0 rad

🤔 Do you want to proceed? (y/N): y
🔧 Sending zero calibration command to Motor 6 (CAN ID: 0x6)
✅ Zero calibration command sent to Motor 6
⏳ Waiting for motor to process calibration...
📊 New reading: 0.0123 rad (0.71°)
✅ SUCCESS: Motor 6 zero calibration completed!
```

#### Usage:
```bash
# Build the tool
make set_motor_zero

# Run the tool
./set_motor_zero
```

#### Interactive Commands:
- `p` or `pos` - Display all motor positions
- `1`-`9` - Set specific motor to zero position
- `a` or `all` - Set all motors to zero position
- `r` or `refresh` - Refresh position readings
- `h` or `help` - Show help
- `q` or `quit` - Exit the tool

#### Example Session:
```
=== IC_CAN Motor Zero Setting Tool ===
✅ System initialized

📊 Current Motor Positions:
======================================================================
Motor | Position (rad) | Position (deg) | Type      | Status
------|---------------|----------------|-----------|--------
    1 |       0.1234 |          7.07 | Damiao    | OFFSET
    2 |      -0.0567 |         -3.25 | Damiao    | NEAR ZERO
    3 |       0.0000 |          0.00 | Damiao    | ZERO
    ...
    6 |       0.8340 |         47.79 | Damiao    | OFFSET
======================================================================

> 6
🎯 Setting Motor 6 to zero position...
Current position: 0.8340 rad (47.79°)
⚠️  Motor 6 has kp=0, setting temporary gains...
✅ Zero command sent to Motor 6
Monitoring for 3 seconds...
Time 1s: 0.7500 rad (42.97°) - moved 0.084000 rad
Time 2s: 0.4500 rad (25.78°) - moved 0.384000 rad
Time 3s: 0.0500 rad (2.86°) - moved 0.784000 rad
✅ Motor 6 is now at zero position!
```

## Building Tools

### Prerequisites:
- IC_CAN system built and available
- dm-tools SDK object files present
- libusb-1.0 development libraries

### Build Commands:
```bash
# Build specific tool
make set_motor_zero

# Build all tools
make all

# Clean build files
make clean

# Install to ../bin/
make install
```

### Manual Compilation:
If Makefile doesn't work, compile manually:
```bash
cd tools/
g++ -std=c++17 -O2 -Wall -Wextra \
    -I../include -I../dm-tools/USB2FDCAN/SDK/C++/ubuntu/include \
    ../dm-tools/USB2FDCAN/SDK/C++/ubuntu/lib/usb_class.cpp.o \
    ../dm-tools/USB2FDCAN/SDK/C++/ubuntu/lib/crc.cpp.o \
    -lusb-1.0 -pthread \
    set_motor_zero.cpp ../src/ic_can/core/ic_can.cpp \
    -o set_motor_zero
```

## Motor Information

The system controls 9 motors total:
- **Motors 1-6**: Damiao motors (various types)
  - Motor 1: DM10010L (high torque)
  - Motors 2-3: DM6248 (medium torque)
  - Motors 4-5: DM4340 (low torque)
  - Motor 6: DM4310 (high speed)
- **Motors 7-8**: HT motors (HT4438)
- **Motor 9**: Servo motor

## Safety Notes

- Always ensure motors are properly enabled before zeroing
- Motors with kp=0 will have temporary gains set for zeroing
- Monitor motor movement during zeroing operations
- Use 'q' to quit safely and disable motors

## Troubleshooting

1. **Motor doesn't move**: Check if motor has proper gains (kp > 0)
2. **Connection issues**: Verify USB2CAN device is connected
3. **Permission errors**: Ensure user has access to USB devices
4. **Compilation errors**: Check that dm-tools SDK object files exist