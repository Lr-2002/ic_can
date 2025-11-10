# STM32 Servo USB Communication Test Script

USB-only version for testing STM32 servo control system without CAN dependencies. Perfect for USB-connected STM32 boards and servo testing with direct serial communication.

## Overview

This script focuses on USB/UART communication only, removing all CAN dependencies while maintaining full compatibility with the `remote_manip_embedding/Core/Src/main.c` protocol:

- **Direct Servo USB**: UART2 equivalent for direct servo commands
- **External Command USB**: UART1 equivalent for external control interface
- **Interactive Mode**: Manual servo control with real-time feedback
- **USB Monitoring**: Real-time communication monitoring and debugging
- **Bidirectional Testing**: Test communication between both USB interfaces

## USB-Only Protocol Features

### Core Servo Commands (Direct USB)

#### **Enable/Disable Torque**
```
Enable:  FF FF 01 04 03 28 01 CE
Disable: FF FF 01 04 03 28 00 CF
```

#### **Position Control**
```
Command: FF FF 01 09 03 2A pos_low pos_high time_0 time_1 vel_low vel_high checksum
Example: FF FF 01 09 03 2A E8 03 00 00 C8 00 D3
```

#### **Read Position**
```
Command: FF FF 01 04 02 38 02 BE
Response: FF FF 01 04 02 38 pos_low pos_high checksum
```

### External Command Interface (10-byte USB packets)

#### **Command Types**
- **0x00**: Disable servo
- **0x01**: Enable servo
- **0x02**: Write position (data = pos_high, pos_low, vel_high, vel_low)
- **0x03**: Read position
- **0x04**: Set middle position

#### **Packet Format**
```
[can_id_high, can_id_low, command, data_0, data_1, data_2, data_3, data_4, data_5, data_6]
Example: 00 09 02 08 00 64 00 00 00 00  # Position 2048, velocity 100
```

## Usage

### Quick Start

```bash
# List available USB ports
python stm32_servo_usb_test.py --list-ports

# Test with default USB ports
python stm32_servo_usb_test.py

# Test specific USB ports
python stm32_servo_usb_test.py --servo-usb /dev/ttyACM0 --external-usb /dev/ttyACM1

# Interactive mode for manual control
python stm32_servo_usb_test.py --test interactive

# Monitor USB communication
python stm32_servo_usb_test.py --monitor 10
```

### Command Line Options

- `--servo-usb`: USB port for direct servo communication (default: `/dev/ttyUSB0`)
- `--external-usb`: USB port for external commands (default: `/dev/ttyUSB1`)
- `--test`: Test type - `basic`, `position`, `external`, `bidirectional`, `interactive`, `all`
- `--positions`: Custom position values for position test
- `--baudrate`: USB communication baudrate (default: 115200)
- `--list-ports`: List available USB ports and exit
- `--monitor`: Monitor servo USB for specified seconds

### USB Port Discovery

```bash
# Find available USB devices
python stm32_servo_usb_test.py --list-ports

# Example output:
Available USB serial devices:
   /dev/ttyUSB0
   /dev/ttyUSB1
   /dev/ttyACM0
```

## Test Types

### 1. Basic Servo Commands Test (`--test basic`)
- Enable/disable servo torque
- Read current position
- Set middle position
- Verify direct USB communication

### 2. Position Control Test (`--test position`)
- Move servo through position sequence
- Test position control with velocity
- Read position after movement
- Verify position accuracy

### 3. External Commands Test (`--test external`)
- Test 10-byte command packet interface
- Enable/disable via external commands
- Position control via external interface
- Read position via external commands

### 4. Bidirectional Communication Test (`--test bidirectional`)
- Send commands via external USB
- Monitor responses on servo USB
- Test communication between interfaces
- Verify complete data flow

### 5. Interactive Mode (`--test interactive`)
- Manual servo control commands
- Real-time position reading
- On-the-fly parameter adjustment
- Live communication monitoring

### 6. USB Monitoring (`--monitor`)
- Real-time USB traffic monitoring
- Timestamped packet display
- Protocol debugging
- Communication analysis

## Examples

### Example 1: USB Port Discovery
```bash
$ python stm32_servo_usb_test.py --list-ports

Available USB serial devices:
   /dev/ttyUSB0
   /dev/ttyACM0
```

### Example 2: Basic USB Test
```bash
$ python stm32_servo_usb_test.py --servo-usb /dev/ttyUSB0 --test basic

🤖 STM32 Servo USB Communication Test Script
USB-only version - no CAN dependencies required
Based on remote_manip_embedding/main.c protocol
======================================================================
🔧 STM32 Servo USB Protocol Interface
   USB-only version (no CAN dependencies)
   Based on remote_manip_embedding/main.c
=======================================================
✅ Servo USB (/dev/ttyUSB0) initialized at 115200 baud

🚀 Testing Basic Servo Commands (Direct USB)
--------------------------------------------------
🔌 Enabling servo torque...
   Sent: FF FF 01 04 03 28 01 CE
📏 Reading servo position...
   Sent: FF FF 01 04 02 38 02 BE
   Recv: FF FF 01 04 02 38 E8 03 42
   Position: 1000
🔌 Disabling servo torque...
   Sent: FF FF 01 04 03 28 00 CF
✅ Basic servo commands completed
```

### Example 3: Position Control Test
```bash
$ python stm32_servo_usb_test.py --test position --positions 1000 2048 3000

🎯 Testing Position Control
   Positions: [1000, 2048, 3000]
----------------------------------------

📍 Move 1/3: Position 1000
🎯 Position control: pos=1000, vel=200
   Sent: FF FF 01 09 03 2A E8 03 00 00 C8 00 D3
📏 Reading servo position...
   Sent: FF FF 01 04 02 38 02 BE
   Recv: FF FF 01 04 02 38 E8 03 42
   Position: 1000

📍 Move 2/3: Position 2048
🎯 Position control: pos=2048, vel=200
   Sent: FF FF 01 09 03 2A 00 08 00 00 C8 00 F4
📏 Reading servo position...
   Sent: FF FF 01 04 02 38 02 BE
   Recv: FF FF 01 04 02 38 00 08 38
   Position: 2048
```

### Example 4: Interactive Mode
```bash
$ python stm32_servo_usb_test.py --test interactive

🎮 Interactive Servo Control Mode
========================================
Commands:
  enable    - Enable servo torque
  disable   - Disable servo torque
  mid       - Set middle position
  read      - Read current position
  pos <0-4095> [vel] - Move to position
  monitor <seconds> - Monitor servo USB
  quit      - Exit interactive mode
----------------------------------------

🎮 servo> enable
🔌 Enabling servo torque...
   Sent: FF FF 01 04 03 28 01 CE

🎮 servo> pos 1500 200
🎯 Position control: pos=1500, vel=200
   Sent: FF FF 01 09 03 2A DC 05 00 00 C8 00 B5

🎮 servo> read
📏 Reading servo position...
   Sent: FF FF 01 04 02 38 02 BE
   Recv: FF FF 01 04 02 38 DC 05 31
   Position: 1500

🎮 servo> monitor 5
👂 Monitoring servo USB for 5s...
   Press Ctrl+C to stop early
   [14:32:15.123] RX: FF FF 01 04 02 38 DC 05 31
   [14:32:15.234] RX: FF FF 01 04 02 38 DC 05 31

🎮 servo> disable
🔌 Disabling servo torque...
   Sent: FF FF 01 04 03 28 00 CF

🎮 servo> quit
✅ Interactive mode completed
```

### Example 5: Bidirectional Communication Test
```bash
$ python stm32_servo_usb_test.py --test bidirectional

🔄 Testing Bidirectional Communication
---------------------------------------------
📤 Sending enable via external USB...
📤 External command: Enable
   Packet: 00 09 01 00 00 00 00 00 00 00
👂 Monitoring servo USB for responses...
👂 Monitoring servo USB for 2.0s...
   Press Ctrl+C to stop early
   [14:35:22.456] RX: FF FF 01 04 03 28 01 CE

📤 Sending position command via external USB...
📤 External command: Write Position
   Packet: 00 09 02 05 DC 00 C8 00 00 00
👂 Monitoring servo USB for responses...
👂 Monitoring servo USB for 2.0s...
   Press Ctrl+C to stop early
   [14:35:24.123] RX: FF FF 01 09 03 2A DC 05 00 00 C8 00 B5
   [14:35:24.234] RX: FF FF 01 04 02 38 DC 05 31

✅ Bidirectional communication test completed
```

### Example 6: USB Communication Monitoring
```bash
$ python stm32_servo_usb_test.py --monitor 10

👂 Monitoring servo USB for 10.0s...
   Press Ctrl+C to stop early
   [14:40:15.123] RX: FF FF 01 04 02 38 DC 05 31
   [14:40:15.234] RX: FF FF 01 04 02 38 DC 05 31
   [14:40:16.345] RX: FF FF 01 09 03 2A E8 03 00 00 C8 00 D3
   [14:40:16.456] RX: FF FF 01 04 02 38 E8 03 42
   [14:40:17.567] RX: FF FF 01 04 03 28 00 CF

✅ Monitoring completed
```

## Hardware Setup

### Required Hardware
1. **STM32 Board**: Running remote_manip_embedding firmware
2. **USB Connection**: USB cable for communication
3. **Servo Motor**: Compatible servo (FEETECH/STS series recommended)
4. **Power Supply**: Appropriate power for servo and STM32

### USB Connection Types

#### **Single USB Setup**
- One USB cable for direct servo communication
- Use `--servo-usb` parameter only
- Tests basic servo functionality

#### **Dual USB Setup**
- Two USB connections for full functionality
- USB 1: Direct servo communication (`--servo-usb`)
- USB 2: External command interface (`--external-usb`)
- Enables bidirectional testing

### USB Port Identification

**Linux/Ubuntu:**
```bash
# List USB devices
ls /dev/ttyUSB* /dev/ttyACM*

# Check device details
dmesg | grep tty
```

**Windows:**
```powershell
# List COM ports
Get-WmiObject Win32_SerialPort
```

**macOS:**
```bash
# List USB devices
ls /dev/tty.usbserial* /dev/cu.usbserial*
```

## Protocol Implementation Details

### USB Communication Settings
- **Baudrate**: 115200 (configurable with `--baudrate`)
- **Data Format**: 8N1 (8 data bits, no parity, 1 stop bit)
- **Timeout**: 100ms (for response waiting)
- **Flow Control**: None

### Command Structure

**Direct Servo Commands (8 bytes):**
```python
cmd = [0xFF, 0xFF, 0x01, 0x04, 0x03, 0x28, torque_enable, checksum]
```

**Position Control Commands (13 bytes):**
```python
cmd = [0xFF, 0xFF, 0x01, 0x09, 0x03, 0x2A,
       pos_low, pos_high,  # Position (little-endian)
       0x00, 0x00,        # Time (reserved)
       vel_low, vel_high,  # Velocity (little-endian)
       checksum]
```

**External Command Packets (10 bytes):**
```python
packet = [0x00, 0x09,  # CAN ID
          command,       # Command type (0x00-0x04)
          data_0...data_6]  # Data bytes (padded with 0)
```

### Checksum Calculation
```python
checksum = (sum(data) & 0xFF) ^ 0xFF
```
Applied to command bytes (excluding FF FF header).

## Troubleshooting

### Common USB Issues

1. **Permission Denied**
   ```
   PermissionError: [Errno 13] Permission denied: '/dev/ttyUSB0'
   ```
   **Solution**: Add user to dialout group:
   ```bash
   sudo usermod -a -G dialout $USER
   # Log out and log back in
   ```

2. **Device Not Found**
   ```
   serial.serialutil.SerialException: [Errno 2] No such file or directory: '/dev/ttyUSB0'
   ```
   **Solution**:
   - Check device connections
   - Use `--list-ports` to find available devices
   - Verify STM32 is powered and connected

3. **No Response from Servo**
   ```
   ❌ Unexpected response length: 0
   ```
   **Possible causes**:
   - Servo not powered
   - Wrong baudrate (try different values with `--baudrate`)
   - USB cable issues
   - STM32 firmware not running

4. **Invalid Response Header**
   ```
   ❌ Invalid response header
   ```
   **Possible causes**:
   - Communication timing issues
   - Servo type incompatibility
   - Noise on USB line

### Debug Steps

1. **List Available Ports**: Use `--list-ports` to see connected devices
2. **Monitor Communication**: Use `--monitor` to see raw USB traffic
3. **Test Basic Commands**: Start with `--test basic` to verify connectivity
4. **Check Baudrate**: Try different baudrates with `--baudrate`
5. **Verify Connections**: Check USB cables and power supply

### Performance Tips

1. **Reduce Delays**: For faster testing, reduce `time.sleep()` values in script
2. **Increase Timeout**: For slower connections, increase timeout values
3. **Use Interactive Mode**: For manual debugging and testing
4. **Monitor Traffic**: Use `--monitor` to analyze communication patterns

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

### System Requirements
- USB port (for STM32 connection)
- Compatible operating system (Linux, macOS, Windows)
- Python environment with pyserial

## Advanced Features

### Real-time Monitoring
The script provides real-time USB communication monitoring with timestamps:
```bash
python stm32_servo_usb_test.py --monitor 30  # Monitor for 30 seconds
```

### Interactive Control
Full interactive mode for manual servo control:
```bash
python stm32_servo_usb_test.py --test interactive
```

### Bidirectional Testing
Test communication between external and servo USB interfaces:
```bash
python stm32_servo_usb_test.py --test bidirectional
```

### Custom Test Sequences
Create custom position sequences:
```bash
python stm32_servo_usb_test.py --test position --positions 500 1500 2500 3500
```

This USB-only version provides a complete testing environment for STM32 servo systems without requiring any CAN hardware, making it ideal for development, debugging, and educational purposes.