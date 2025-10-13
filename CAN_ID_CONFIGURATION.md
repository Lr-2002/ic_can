# IC_CAN Multi-Motor CAN ID Configuration

This document details the CAN ID configuration for all 9 motors in the IC_CAN system.

## Overview

The IC_CAN system supports 9 motors on a single CAN-FD bus:
- **6 Arm Motors (m1-m6)**: Damiao (DM) series motors
- **2 Gripper Motors (m7-m8)**: High Torque (HT) series motors
- **1 End Effector Motor (m9)**: Standard servo motor

## CAN ID Mapping

### Arm Motors (m1-m6)

| Motor ID | Motor Type | CAN Send ID | CAN Receive ID | Position Range (rad) | Max Velocity (rad/s) | Max Torque (Nm) |
|----------|------------|-------------|----------------|---------------------|----------------------|-----------------|
| m1 | DM10010L | 0x01 | 0x11 | -52.36 to 52.36 | 15.0 | 15.0 |
| m2 | DM6248 | 0x02 | 0x12 | -10.47 to 10.47 | 10.0 | 8.0 |
| m3 | DM6248 | 0x03 | 0x13 | -10.47 to 10.47 | 10.0 | 8.0 |
| m4 | DM4340 | 0x04 | 0x14 | -6.28 to 6.28 | 8.0 | 4.0 |
| m5 | DM4340 | 0x05 | 0x15 | -6.28 to 6.28 | 8.0 | 4.0 |
| m6 | DM4310 | 0x06 | 0x16 | -3.14 to 3.14 | 5.0 | 3.0 |

### Gripper Motors (m7-m9)

| Motor ID | Motor Type | CAN Send ID | CAN Receive ID | Position Range (rad) | Max Velocity (rad/s) | Max Torque (Nm) |
|----------|------------|-------------|----------------|---------------------|----------------------|-----------------|
| m7 | HT4438 | 0x07 | 0x17 | -6.28 to 6.28 | 2.0 | 20.0 |
| m8 | HT4438 | 0x08 | 0x18 | -6.28 to 6.28 | 2.0 | 20.0 |
| m9 | Servo | 0x09 | 0x19 | -3.14 to 3.14 | 10.0 | 5.0 |

## Control Parameters

### Gains (Kp/Kd)

| Motor ID | Kp | Kd |
|----------|----|----|
| m1 | 250.0 | 5.0 |
| m2 | 120.0 | 2.0 |
| m3 | 120.0 | 2.0 |
| m4 | 40.0 | 1.0 |
| m5 | 40.0 | 1.0 |
| m6 | 30.0 | 1.0 |
| m7 | 8.0 | 1.2 |
| m8 | 8.0 | 1.2 |
| m9 | 0.0 | 0.0 |

## CAN Communication Protocol

### Damiao Motors (m1-m6)
- **Protocol**: Damiao MIT Control Protocol
- **Frame Format**: CAN-FD
- **Baud Rate**: 1 Mbps
- **Data Rate**: 5 Mbps
- **Commands**: Enable, Disable, Set Zero, MIT Control, Parameter Read/Write

### High Torque Motors (m7-m8)
- **Protocol**: High Torque Custom Protocol
- **Frame Format**: CAN-FD
- **Baud Rate**: 1 Mbps
- **Data Rate**: 5 Mbps
- **Commands**: Position Control, Force Control, Status Query

### Servo Motor (m9)
- **Protocol**: Standard PWM Servo Protocol (adapted for CAN)
- **Frame Format**: Standard CAN
- **Baud Rate**: 1 Mbps
- **Commands**: Position Control, Status Query

## Safety Considerations

### Position Limits
Each motor has software-enforced position limits as shown in the tables above. These limits are implemented in the SafetyModule and will trigger emergency stops if exceeded.

### Velocity and Torque Limits
Maximum velocity and torque limits are enforced both in software (SafetyModule) and hardware (motor controllers).

### Communication Monitoring
- **Watchdog Timeout**: 100ms default
- **Communication Timeout**: 50ms default
- **Emergency Stop**: Dedicated CAN frame (0x7FF) for immediate motor shutdown

## Frame Processing

### Incoming Frames
The system processes incoming CAN frames based on their ID:
- **0x11-0x16**: Arm motor status responses
- **0x17-0x18**: Gripper motor status responses
- **0x19**: Servo motor status response

### Outgoing Frames
Command frames are sent using the send IDs:
- **0x01-0x06**: Arm motor commands
- **0x07-0x08**: Gripper motor commands
- **0x09**: Servo motor commands

## Frequency Requirements

- **Control Loop**: 500Hz (2ms period)
- **Status Updates**: 100Hz (10ms period)
- **Safety Monitoring**: 100Hz (10ms period)
- **Watchdog Feeding**: Every control cycle (2ms)

## Error Handling

### Communication Errors
- Frame timeout detection
- CRC validation
- Frame counter verification

### Motor Errors
- Over-temperature protection
- Position/velocity/torque limit violations
- Hardware fault detection

### System Errors
- Emergency stop triggered by any safety violation
- Automatic motor disable on critical errors
- Error logging and reporting

## Configuration Examples

### Basic Motor Control
```cpp
// Control arm motor m1 (shoulder)
uint32_t can_send_id = 0x01;
uint32_t can_recv_id = 0x11;
std::vector<uint8_t> command_data = create_mit_command(0.5, 0.1, 0.2, 30.0, 1.5);
can_adapter.send_frame(can_send_id, command_data);
```

### Safety Limit Check
```cpp
// Check if position is within limits for m1
double position = get_motor_position(1);
bool within_limits = safety.check_position_limits(0, position); // Motor index 0 = m1
```

This configuration ensures reliable and safe operation of all 9 motors on a single CAN-FD bus at the required 500Hz control frequency.