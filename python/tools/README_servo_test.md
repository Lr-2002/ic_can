# Servo Communication Test Script

Fast Python test script for servo angle send/read commands based on actual IC_CAN transmission code.

## Overview

This script implements the low-level servo communication commands for testing IC_CAN servo motors. Based on the actual transmission functions in `src/ic_can/core/ic_can.cpp`, it provides direct access to servo position control and status reading using the same protocol as the C++ implementation.

## Commands Implemented

Based on actual IC_CAN servo transmission protocol:

### Core Commands
- **Enable Servo**: `[0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]` - Enable servo motor
- **Disable Servo**: `[0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]` - Disable servo motor
- **Write Position**: `[0x02, pos_high, pos_low, 0x00, 0x00, 0x00, 0x00, 0x00]` - Set servo position
- **Read Status**: `[0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]` - Read servo status
- **Read Position**: `[0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]` - Read current position

### Test Commands (matching can_test.cpp)
- **Position Command**: `[0x02, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]` - Test position command
- **Status Command**: `[0x02, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]` - Test status command

### Protocol Details
- **CAN Send ID**: `0x09` (servo command)
- **CAN Response ID**: `0x19` (servo feedback)
- **Frame Format**: 8-byte CAN frames
- **Position Format**: 16-bit big-endian (0-4095 range typical)

## Usage

### Basic Usage

```bash
# Run all tests on default motor (ID 9)
python servo_test.py

# Test specific motor
python servo_test.py --motor-id 7

# Run specific test type
python servo_test.py --test basic
python servo_test.py --test position
python servo_test.py --test alternating
```

### Advanced Usage

```bash
# Custom position sequence
python servo_test.py --test position --positions 1000 2000 3000 2000 1000

# Custom alternating command test
python servo_test.py --test alternating --count 20 --interval 50

# Test specific motor with custom settings
python servo_test.py --motor-id 8 --test all --count 15 --interval 200
```

### Command Line Options

- `--motor-id`: Motor ID to test (default: 9 - gripper servo)
- `--test`: Test type - `basic`, `position`, `alternating`, `all` (default: all)
- `--positions`: Custom position values for position test
- `--count`: Number of alternating commands (default: 10)
- `--interval`: Interval between commands in ms (default: 100)

## Test Types

### 1. Basic Communication Test (`--test basic`)
- Enable servo
- Read status
- Read current position
- Verify basic communication protocol

### 2. Position Control Test (`--test position`)
- Send sequence of position commands
- Read position after each command
- Verify position tracking and response

### 3. Alternating Commands Test (`--test alternating`)
- Send alternating `02 08 00` and `02 05 00` commands
- Test timing and response patterns
- Similar to C++ can_test.cpp functionality

## Examples

### Example 1: Quick Servo Test
```bash
$ python servo_test.py --test basic

🤖 Servo Communication Test Script
Based on remote_manip_embedding I/O commands
============================================================
🔧 Servo Tester - Motor ID: 9, CAN ID: 0x09
==================================================
✅ IC_CAN interface initialized

🚀 Starting Basic Communication Test
----------------------------------------

🔌 Enabling servo...
📤 SENT: Enable Servo
   ID: 0x09 Data: 02 01 00 00 00 00 00 00
📥 RECV: ID: 0x19 Data: 02 01 00 FF FF FF FF FF

📊 Reading servo status...
📤 SENT: Read Status
   ID: 0x09 Data: 02 05 00 00 00 00 00 00
📥 RECV: ID: 0x19 Data: 02 05 00 64 00 00 00 00

📏 Reading current position...
📤 SENT: Read Position
   ID: 0x09 Data: 02 03 00 00 00 00 00 00
📥 RECV: ID: 0x19 Data: 02 03 00 E8 03 00 00 00
   Current position: 1000

✅ Basic communication test completed
```

### Example 2: Position Control Test
```bash
$ python servo_test.py --test position --positions 0 1000 2000 3000

🎯 Testing Position Control
   Target positions: [0, 1000, 2000, 3000]
----------------------------------------

📍 Move 1/4: Position 0
📤 SENT: Write Position 0
   ID: 0x09 Data: 02 08 00 00 00 00 00 00
📥 RECV: ID: 0x19 Data: 02 08 00 FF FF FF FF FF
📏 Reading current position...
📤 SENT: Read Position
   ID: 0x09 Data: 02 03 00 00 00 00 00 00
📥 RECV: ID: 0x19 Data: 02 03 00 00 00 00 00 00
   → Reached position: 0

📍 Move 2/4: Position 1000
📤 SENT: Write Position 1000
   ID: 0x09 Data: 02 08 00 E8 03 00 00 00 00
📥 RECV: ID: 0x19 Data: 02 08 00 FF FF FF FF FF
📏 Reading current position...
📤 SENT: Read Position
   ID: 0x09 Data: 02 03 00 00 00 00 00 00
📥 RECV: ID: 0x19 Data: 02 03 00 E8 03 00 00 00
   → Reached position: 1000
```

### Example 3: High-Frequency Alternating Test
```bash
$ python servo_test.py --test alternating --count 20 --interval 50

🔄 Testing Alternating Commands
   Commands: 02 08 00 ↔ 02 05 00
   Count: 20, Interval: 50ms
--------------------------------------------------
   Count        Time             SEND              RECEIVE
----------------------------------------------------------------
    1     [   15.2ms]    ✅      ✅
    2     [   65.3ms]    ✅      ✅
    3     [  115.4ms]    ✅      ✅
    ...

📊 Alternating test completed
   Actual frequency: 19.85 Hz
```

## Protocol Reference

### Command Structure (8-byte CAN frames)
```
Byte 0: Command type (0x00=disable, 0x01=enable, 0x02=position, 0x03=read)
Byte 1: Position high byte (for position commands)
Byte 2: Position low byte (for position commands)
Bytes 3-7: Padding (always 0x00)
```

### Response Structure
```
Byte 0: Echo command type
Byte 1+: Response data
Remaining: Padding
```

### Position Encoding
- **Format**: 16-bit big-endian (matching IC_CAN implementation)
- **Range**: 0-4095 (typical for 12-bit servos)
- **Bytes**: Byte 1 = position_high, Byte 2 = position_low
- **Conversion**: `pos_raw = position / (2π) * 4095` (from radians to servo units)

## Dependencies

- Python 3.7+
- IC_CAN Python bindings (`ic_can_python`)
- Built with: `-DBUILD_PYTHON_BINDINGS=ON`

### Build Requirements
```bash
mkdir -p build && cd build
cmake .. -DBUILD_PYTHON_BINDINGS=ON
make -j$(nproc)
```

## Troubleshooting

### Common Issues

1. **Module not found error**
   ```
   ❌ ic_can_python module not found
   ```
   **Solution**: Build with Python bindings enabled:
   ```bash
   mkdir -p build && cd build
   cmake .. -DBUILD_PYTHON_BINDINGS=ON && make -j$(nproc)
   ```

2. **No response from servo**
   ```
   📥 No response within 100ms
   ```
   **Possible causes**:
   - Servo not powered or connected
   - Wrong motor ID
   - CAN bus communication issue
   - Servo not enabled

3. **Invalid position values**
   ```
   Current position: 65535
   ```
   **Solution**: Check servo type and position range. Some servos use different encoding.

### Debug Tips

1. **Use basic test first**: Always start with `--test basic` to verify communication
2. **Check motor ID**: Ensure you're using the correct motor ID for your servo
3. **Verify CAN connection**: Use the C++ `can_test` tool to verify CAN communication
4. **Monitor responses**: Watch the hex responses to understand servo behavior

## Integration with IC_CAN

This script uses the same IC_CAN interface as the C++ tools, ensuring consistent behavior:

- **C++ equivalent**: `./build/tools/can_test`
- **Python equivalent**: `python servo_test.py --test alternating`

Both tools implement the same CAN protocol and command structure.