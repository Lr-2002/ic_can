# Simple USB Servo Test Script

Direct USB connection to servo - single port implementation. Perfect for testing with one USB connection to your servo motor.

## Overview

This script connects directly to a servo via USB and provides complete servo control functionality based on the `remote_manip_embedding/Core/Src/main.c` protocol:

- **Direct Servo Commands**: Enable/disable torque, read position, set middle
- **Position Control**: Move to specific positions with velocity control
- **Interactive Mode**: Manual servo control with real-time feedback
- **USB Monitoring**: Real-time communication monitoring
- **Continuous Movement**: Sweep through all positions automatically

## Quick Start

```bash
# Find available USB ports
python servo_usb_test.py --list-ports

# Test with your USB servo
python servo_usb_test.py --usb-port /dev/ttyUSB0

# Interactive mode for manual control
python servo_usb_test.py --test interactive

# Test specific positions
python servo_usb_test.py --test position --positions 800 1175 1500
```

## Usage Examples

### 1. Port Discovery
```bash
$ python servo_usb_test.py --list-ports

Available USB serial devices:
   /dev/ttyUSB0
   /dev/ttyACM0
```

### 2. Basic Testing
```bash
$ python servo_usb_test.py --usb-port /dev/ttyUSB0 --test basic

🤖 Simple USB Servo Test Script
Direct USB connection to servo - no CAN dependencies
Based on remote_manip_embedding/main.c protocol
=================================================================
🔧 Direct USB Servo Protocol Interface
   Single USB connection to servo
   Position range: 700-1650
   Middle position: 1175
   Based on remote_manip_embedding/main.c
==================================================
✅ Servo USB (/dev/ttyUSB0) initialized at 115200 baud

🚀 Testing Basic Servo Commands
----------------------------------------
🔌 Enabling servo torque...
   Sent: FF FF 01 04 03 28 01 CE
📏 Reading servo position...
   Sent: FF FF 01 04 02 38 02 BE
   Recv: FF FF 01 04 02 38 E8 03 42
   Position: 1000
🔌 Disabling servo torque...
   Sent: FF FF 01 04 03 28 00 CF
✅ Basic commands completed
```

### 3. Interactive Mode
```bash
$ python servo_usb_test.py --test interactive

🎮 Interactive Servo Control Mode
===================================
Commands:
  enable    - Enable servo torque
  disable   - Disable servo torque
  mid       - Set middle position (2048)
  read      - Read current position
  pos <0-4095> [vel] - Move to position
  sweep     - Sweep through all positions
  monitor <seconds> - Monitor USB traffic
  quit      - Exit interactive mode
-----------------------------------

🎮 servo> enable
🔌 Enabling servo torque...
   Sent: FF FF 01 04 03 28 01 CE

🎮 servo> pos 1200 50
🎯 Position control: pos=1200, vel=50
   Sent: FF FF 01 09 03 2A B0 04 00 00 32 00 99

🎮 servo> read
📏 Reading servo position...
   Sent: FF FF 01 04 02 38 02 BE
   Recv: FF FF 01 04 02 38 B0 04 35
   Position: 1200

🎮 servo> mid
🎯 Setting servo to middle position (1175)...

🎮 servo> sweep
🔄 Testing Continuous Movement
-----------------------------------
   Sweeping through 20 positions (700-1650)...
   Position 20/20: 700
   Sweep completed

🎮 servo> quit
✅ Interactive mode completed
```

### 4. Custom Position Testing
```bash
$ python servo_usb_test.py --test position --positions 800 1175 1500

🎯 Testing Position Control
   Positions: [800, 1175, 1500]
-----------------------------------

📍 Move 1/3: Position 800
🎯 Position control: pos=800, vel=50
   Sent: FF FF 01 09 03 2A 20 03 00 00 32 00 F5
📏 Reading servo position...
   Sent: FF FF 01 04 02 38 02 BE
   Recv: FF FF 01 04 02 38 20 03 31
   Position: 800

📍 Move 2/3: Position 1175
🎯 Position control: pos=1175, vel=50
   Sent: FF FF 01 09 03 2A 97 04 00 00 32 00 A0
📏 Reading servo position...
   Sent: FF FF 01 04 02 38 02 BE
   Recv: FF FF 01 04 02 38 97 04 6A
   Position: 1175
```

### 5. USB Monitoring
```bash
$ python servo_usb_test.py --monitor 10

👂 Monitoring servo USB for 10.0s...
   Press Ctrl+C to stop early
   [14:45:23.123] RX: FF FF 01 04 02 38 F4 01 35
   [14:45:23.234] RX: FF FF 01 04 02 38 F4 01 35
   [14:45:25.345] RX: FF FF 01 09 03 2A E8 03 00 00 C8 00 D3

✅ Monitoring completed
```

## Command Line Options

```bash
python servo_usb_test.py [OPTIONS]

Options:
  --usb-port PORT       Servo USB port (default: /dev/ttyUSB0)
  --test TYPE          Test type: basic, position, continuous, all, interactive
  --positions P1 P2... Custom positions for position test
  --baudrate RATE      USB baudrate (default: 115200)
  --list-ports         List available USB ports and exit
  --monitor SECONDS    Monitor USB for specified seconds
  --help               Show help message
```

## Test Types

### **Basic Test** (`--test basic`)
- Enable servo torque
- Read current position
- Disable servo torque
- Verify basic communication

### **Position Test** (`--test position`)
- Move through position sequence
- Read position after each move
- Verify position accuracy

### **Continuous Test** (`--test continuous`)
- Sweep through all positions (0 → 4095 → 0)
- Test continuous movement
- Verify servo range

### **Interactive Mode** (`--test interactive`)
- Manual servo control
- Real-time position reading
- On-the-fly parameter adjustment
- Live communication monitoring

## Interactive Commands

When in interactive mode (`--test interactive`):

```
🎮 servo> enable           # Enable servo torque
🎮 servo> disable          # Disable servo torque
🎮 servo> mid              # Set middle position (2048)
🎮 servo> read             # Read current position
🎮 servo> pos 1500 50      # Move to position 1500 with velocity 50
🎮 servo> sweep            # Sweep through all positions
🎮 servo> monitor 5        # Monitor USB traffic for 5 seconds
🎮 servo> quit             # Exit interactive mode
```

## Servo Protocol

The script implements the exact protocol from `main.c`:

### **Enable Torque**
```
Command: FF FF 01 04 03 28 01 CE
```

### **Disable Torque**
```
Command: FF FF 01 04 03 28 00 CF
```

### **Position Control**
```
Command: FF FF 01 09 03 2A pos_low pos_high time_0 time_1 vel_low vel_high checksum
Example: FF FF 01 09 03 2A E8 03 00 00 32 00 3A  # Position 1000, velocity 50
```

### **Read Position**
```
Command: FF FF 01 04 02 38 02 BE
Response: FF FF 01 04 02 38 pos_low pos_high checksum
```

### **Set Middle Position**
```
Command: FF FF 01 04 03 28 80 4F  # Position 2048
```

## Position Range and Speed Limits

- **Position Range**: 700-1650 (specific servo range)
- **Middle Position**: 1175
- **Speed Range**: 0-100 (limited for safety)
- **Default Speed**: 50
- **Common Positions**:
  - 700: Minimum position
  - 937: Quarter position
  - 1175: Middle position
  - 1412: Three-quarter position
  - 1650: Maximum position

The position and speed ranges are automatically enforced for your specific servo:
- Position commands outside 700-1650 will be rejected
- Speed commands outside 0-100 will be rejected
- Default safe speed of 50 is used when no speed is specified

## Hardware Setup

### Required Hardware
1. **Servo Motor**: Compatible with the protocol (FEETECH/STS series recommended)
2. **USB Connection**: USB cable or USB-to-serial adapter
3. **Power Supply**: Appropriate power for the servo (check servo specifications)

### Connection
1. Connect servo to USB port (`/dev/ttyUSB0` or similar)
2. Ensure servo is powered
3. Run the script with correct port designation

### Troubleshooting

#### **Permission Denied**
```bash
sudo usermod -a -G dialout $USER
# Log out and log back in
```

#### **Device Not Found**
```bash
# List available devices
python servo_usb_test.py --list-ports

# Check physical connections
dmesg | grep tty
```

#### **No Response from Servo**
- Check servo power supply
- Verify USB connection
- Try different baudrate (`--baudrate 1000000` for some servos)
- Ensure servo is compatible with the protocol

#### **Invalid Position**
- Position must be between 700 and 1650
- Use `pos 1175` for middle position
- Check servo's position range specification

## Dependencies

### Required Libraries
- **Python 3.7+**
- **pyserial**: `pip install pyserial`

### Installation
```bash
# Install pyserial
pip install pyserial

# Verify installation
python -c "import serial; print('pyserial installed successfully')"
```

## Advanced Features

### **Custom Position Sequences**
```bash
python servo_usb_test.py --test position --positions 100 500 1000 1500 2000
```

### **Different Baudrates**
```bash
python servo_usb_test.py --baudrate 1000000 --test basic
```

### **Real-time Monitoring**
```bash
python servo_usb_test.py --monitor 30  # Monitor for 30 seconds
```

### **Continuous Movement Testing**
```bash
python servo_usb_test.py --test continuous
```

This simple USB servo test script provides everything you need to test and control your servo via a single USB connection, with no CAN dependencies required!