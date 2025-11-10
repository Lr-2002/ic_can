# STM32 Servo Communication Test Script

Comprehensive Python test script for STM32 servo control system based on `remote_manip_embedding/Core/Src/main.c`. This script implements the complete servo protocol including direct UART communication, external command interface, and CAN bridge functionality.

## Overview

This script implements the multi-layered servo control system found in the STM32 main.c implementation:

- **UART2 Interface**: Direct servo communication with checksum validation
- **UART1 Interface**: External command interface (10-byte packets)
- **CAN Bridge**: Bridge functionality for industrial integration
- **Multi-servo Support**: Expandable for multiple servo motors

## Protocol Analysis from main.c

### Architecture Overview

The STM32 system implements a three-layer architecture:

1. **Direct Servo Layer (UART2)**: Commands like `FF FF 01 04 03 28 01 CE`
2. **External Command Layer (UART1)**: 10-byte packets for external control
3. **Bridge Layer (CAN)**: 8-byte CAN frames for industrial integration

### Core Servo Commands (Direct UART2)

#### **Enable Torque**
```
Command: FF FF 01 04 03 28 01 CE
Function: Enable servo torque output
```

#### **Disable Torque**
```
Command: FF FF 01 04 03 28 00 CF
Function: Disable servo torque output
```

#### **Set Middle Position**
```
Command: FF FF 01 04 03 28 80 4F
Function: Set servo to middle position (2048)
```

#### **Read Position**
```
Command: FF FF 01 04 02 38 02 BE
Response: FF FF 01 04 02 38 pos_low pos_high checksum
Function: Read current servo position
```

#### **Position Control**
```
Command: FF FF 01 09 03 2A pos_low pos_high time_0 time_1 vel_low vel_high checksum
Example: FF FF 01 09 03 2A E8 03 00 00 64 00 42
Function: Move servo to position with specified velocity
```

### External Command Interface (UART1)

#### **10-Byte Packet Format**
```
[can_id_high, can_id_low, command, data_0, data_1, data_2, data_3, data_4, data_5, data_6]
```

#### **Command Types**
- **0x00**: Disable servo
- **0x01**: Enable servo
- **0x02**: Write position (data = pos_high, pos_low, vel_high, vel_low)
- **0x03**: Read position
- **0x04**: Set middle position

### CAN Bridge Protocol

#### **8-Byte CAN Frame**
```
ID: 0x000 (configurable)
Data: [pos_high, pos_low, speed_high, speed_low, ...] (up to 2 servos)
```

#### **Servo Data Format**
```
For each servo (4 bytes):
- Byte 0: Position high byte
- Byte 1: Position low byte
- Byte 2: Speed high byte
- Byte 3: Speed low byte
```

### Checksum Calculation

The protocol uses a BITXOR checksum method:
```python
checksum = (sum(data) & 0xFF) ^ 0xFF
```

Applied to bytes 2-11 (excluding FF FF header and checksum byte).

## Usage

### Hardware Setup

1. **STM32 Board**: Running the remote_manip_embedding firmware
2. **UART2 Connection**: Direct servo communication (typically `/dev/ttyUSB0`)
3. **UART1 Connection**: External command interface (typically `/dev/ttyUSB1`)
4. **Servo Motor**: Compatible with the protocol (e.g., FEETECH/STS series)

### Basic Usage

```bash
# Run comprehensive test with default settings
python stm32_servo_test.py

# Test specific functionality
python stm32_servo_test.py --test basic
python stm32_servo_test.py --test position
python stm32_servo_test.py --test external
python stm32_servo_test.py --test can

# Custom UART ports
python stm32_servo_test.py --servo-uart /dev/ttyACM0 --external-uart /dev/ttyACM1

# Custom position sequence
python stm32_servo_test.py --test position --positions 1000 2000 3000 2048

# Simulation mode (no hardware required)
python stm32_servo_test.py --simulation
```

### Command Line Options

- `--servo-uart`: UART port for direct servo communication (default: `/dev/ttyUSB0`)
- `--external-uart`: UART port for external commands (default: `/dev/ttyUSB1`)
- `--test`: Test type - `basic`, `position`, `external`, `can`, `all` (default: all)
- `--positions`: Custom position values for position test
- `--simulation`: Run in simulation mode without hardware

## Test Types

### 1. Basic Servo Commands Test (`--test basic`)
- Enable/disable servo torque
- Read current position
- Set middle position
- Verify direct UART communication

### 2. Position Control Test (`--test position`)
- Move servo to multiple positions
- Test position control with velocity
- Read position after movement
- Verify position accuracy

### 3. External Commands Test (`--test external`)
- Test 10-byte command packet interface
- Enable/disable via external commands
- Position control via external interface
- Read position via external commands

### 4. CAN Bridge Test (`--test can`)
- Format servo data for CAN transmission
- Send position/speed data via CAN
- Test bridge functionality
- Verify CAN frame format

## Examples

### Example 1: Basic Servo Test
```bash
$ python stm32_servo_test.py --test basic --servo-uart /dev/ttyUSB0

🤖 STM32 Servo Communication Test Script
Based on remote_manip_embedding/main.c protocol
============================================================
🔧 STM32 Servo Protocol Interface
   Based on remote_manip_embedding/main.c
==================================================
✅ UART /dev/ttyUSB0 initialized at 115200 baud
✅ IC_CAN interface initialized

🚀 Testing Basic Servo Commands (Direct UART)
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

### Example 2: Position Control Test
```bash
$ python stm32_servo_test.py --test position --positions 1000 2048 3000

🎯 Testing Position Control
   Positions: [1000, 2048, 3000]
----------------------------------------

📍 Move 1/3: Position 1000
🎯 Position control: pos=1000, vel=200
   Sent: FF FF 01 09 03 2A E8 03 00 00 C8 00 D3
📏 Reading servo position...
   Sent: FF FF 01 04 02 38 02 BE
   Recv: FF FF 01 04 02 38 E8 03 42
   Current position: 1000

📍 Move 2/3: Position 2048
🎯 Position control: pos=2048, vel=200
   Sent: FF FF 01 09 03 2A 00 08 00 00 C8 00 F4
📏 Reading servo position...
   Sent: FF FF 01 04 02 38 02 BE
   Recv: FF FF 01 04 02 38 00 08 38
   Current position: 2048
```

### Example 3: External Commands Test
```bash
$ python stm32_servo_test.py --test external --external-uart /dev/ttyUSB1

🌐 Testing External Command Interface
---------------------------------------------
📤 External command: Enable
   Packet: 00 09 01 00 00 00 00 00 00 00
📤 External command: Write Position
   Packet: 00 09 02 08 00 64 00 00 00 00
📤 External command: Read Position
   Packet: 00 09 03 00 00 00 00 00 00 00
📤 External command: Disable
   Packet: 00 09 00 00 00 00 00 00 00 00
✅ External commands test completed
```

### Example 4: Simulation Mode
```bash
$ python stm32_servo_test.py --simulation

🔧 Running in simulation mode
✅ IC_CAN interface initialized

🚀 Testing Basic Servo Commands (Direct UART)
--------------------------------------------------
❌ Servo UART not connected
❌ Servo UART not connected
❌ Servo UART not connected
✅ Basic servo commands completed
```

## Protocol Implementation Details

### UART2 Direct Servo Communication

**Baud Rate**: 115200 (configurable in main.c)
**Data Format**: 8N1 (8 data bits, no parity, 1 stop bit)
**Command Structure**: FF FF header + command bytes + checksum

```python
def servo_position_control(position, velocity):
    cmd = [0xFF, 0xFF, 0x01, 0x09, 0x03, 0x2A]
    cmd.extend([position & 0xFF, (position >> 8) & 0xFF])  # Position
    cmd.extend([0x00, 0x00])                              # Time (reserved)
    cmd.extend([velocity & 0xFF, (velocity >> 8) & 0xFF]) # Velocity
    cmd.append(calculate_checksum(cmd[2:]))                # Checksum
    servo_uart.write(bytes(cmd))
```

### UART1 External Command Interface

**Baud Rate**: 115200 (configurable in main.c)
**Packet Size**: 10 bytes fixed
**Command Processing**: State machine in main.c while loop

```python
def send_position_command(position, velocity):
    packet = [0x00, 0x09, 0x02]  # CAN ID + Command
    packet.extend([(position >> 8) & 0xFF, position & 0xFF])  # Position
    packet.extend([(velocity >> 8) & 0xFF, velocity & 0xFF])  # Velocity
    packet.extend([0x00] * 4)  # Padding to 10 bytes
    external_uart.write(bytes(packet))
```

### CAN Bridge Integration

**CAN ID**: 0x000 (configurable in main.c)
**Data Length**: 8 bytes
**Update Rate**: 5-10ms intervals (from HAL_Delay in main.c)

```python
def format_servo_data():
    data = []
    for i in range(num_servos):
        data.extend([
            (servo_position[i] >> 8) & 0xFF,  # Position high
            servo_position[i] & 0xFF,           # Position low
            (servo_speed[i] >> 8) & 0xFF,      # Speed high
            servo_speed[i] & 0xFF              # Speed low
        ])
    return data[:8]  # Ensure 8 bytes
```

## Dependencies

### Required Libraries
- **Python 3.7+**
- **pyserial**: `pip install pyserial`

### Optional Dependencies
- **ic_can_python**: Build with `-DBUILD_PYTHON_BINDINGS=ON`
- **NumPy**: For advanced data processing (optional)

### Installation
```bash
# Install pyserial
pip install pyserial

# Build IC_CAN with Python bindings (optional)
mkdir -p build && cd build
cmake .. -DBUILD_PYTHON_BINDINGS=ON
make -j$(nproc)
```

## Troubleshooting

### Common Issues

1. **UART Permission Denied**
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
   **Solution**: Check available devices:
   ```bash
   ls /dev/ttyUSB* /dev/ttyACM*
   dmesg | grep tty
   ```

3. **Invalid Response from Servo**
   ```
   ❌ Invalid response header
   ```
   **Possible causes**:
   - Wrong servo model or protocol
   - UART wiring issues
   - Baud rate mismatch
   - Servo not powered

4. **No IC_CAN Interface**
   ```
   ⚠️ IC_CAN not available: ic_can_python module not found
   ```
   **Solution**: Build with Python bindings or use `--simulation` mode.

### Debug Tips

1. **Use Simulation Mode**: Test with `--simulation` flag first
2. **Check UART Connections**: Verify wiring and device permissions
3. **Monitor Traffic**: Use logic analyzer or serial monitor
4. **Test Individual Components**: Run specific tests with `--test` option
5. **Adjust Timeouts**: Increase delays if communication is unreliable

## Integration with Other Scripts

This STM32 servo script complements the existing IC_CAN servo test script:

- **`servo_test.py`**: For direct IC_CAN servo control
- **`stm32_servo_test.py`**: For STM32-based servo systems
- Both scripts use similar interfaces and test patterns

Use the appropriate script based on your hardware configuration:
- IC_CAN direct servo control → `servo_test.py`
- STM32 bridge/remote system → `stm32_servo_test.py`

## Protocol Compatibility

This implementation is compatible with:

- **FEETECH STS/SCS series servos**
- **Arduino-based servo controllers**
- **Custom STM32 servo implementations**
- **Industrial CAN servo systems**

The protocol can be adapted for other servo types by modifying the command structures and checksum calculation.