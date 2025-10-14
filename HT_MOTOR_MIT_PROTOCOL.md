# HT Motor MIT Control Protocol

## Overview
This document describes the HT (High Torque) motor MIT control protocol discovered and implemented based on the Python code in `/home/lr-2002/project/InstantCreation/DM_Control_Python/ic_arm_control/control/ht_motor.py`.

## HT Motor Characteristics

### Motor Type: HT4438
- **Reduction Ratio**: 32:1
- **Position Units**: Turns (revolutions)
- **Control Mode**: MIT (Model-In-the-Loop)
- **CAN IDs**: 0x700 (status), 0x800 (replies), 0x8094 (MIT control)

## Protocol Details

### 1. Command Format (MIT Control)
**CAN ID**: `0x8094`
**Data Length**: 12 bytes
**Format**: `[position(2), velocity(2), torque(2), kp(2), kd(2), padding(2)]`

```
Byte 0-1: Position (int16, little-endian)
Byte 2-3: Velocity (int16, little-endian)
Byte 4-5: Torque (int16, little-endian)
Byte 6-7: Kp gain (int16, little-endian)
Byte 8-9: Kd gain (int16, little-endian)
Byte 10-11: Padding (0x00, 0x00)
```

### 2. Position Scaling
- **LSB**: 0.0001 turns per bit
- **Conversion**: `position_turns = int16_value * 0.0001`
- **To radians**: `position_rad = position_turns * 2π`

### 3. Velocity Scaling
- **LSB**: 0.00025 turns/s per bit
- **Conversion**: `velocity_turns_per_sec = int16_value * 0.00025`
- **To rad/s**: `velocity_rad_per_sec = velocity_turns_per_sec * 2π`

### 4. Torque Scaling
- **Linear Equation**: `torque_Nm = int16_value * torque_k + torque_d`
- **Constants**: `torque_k = 0.004855`, `torque_d = -0.083`
- **Reverse Conversion**: `int16_value = (torque_Nm - torque_d) / torque_k`

### 5. Gain Scaling
- **Kp, Kd LSB**: 0.1 per bit
- **Conversion**: `gain_value = int16_value * 0.1`

## 3. Response Format (Status Feedback)

### Status Frame IDs
- **0x700**: Motor status broadcast
- **0x800**: Motor reply to commands

### Response Data Structure
**Data Length**: 7+ bytes
**Format**: `[error(1), position(2), velocity(2), torque(2), ...]`

```
Byte 0:    Error code
Byte 1-2:  Position (int16, little-endian)
Byte 3-4:  Velocity (int16, little-endian)
Byte 5-6:  Torque (int16, little-endian)
Byte 7+:   Additional data (if available)
```

### Status Conversion
Same scaling as command format:
- **Position**: `position_rad = position_int16 * 0.0001 * 2π`
- **Velocity**: `velocity_rad_per_sec = velocity_int16 * 0.00025 * 2π`
- **Torque**: `torque_Nm = torque_int16 * 0.004855 + (-0.083)`

## 4. Comparison with Damiao Motors

| Feature | Damiao Motors | HT Motors |
|---------|---------------|------------|
| **Protocol** | 8-byte MIT command | 12-byte MIT command |
| **Position LSB** | Variable (depends on motor) | 0.0001 turns |
| **Velocity LSB** | Variable (depends on motor) | 0.00025 turns/s |
| **Torque Scaling** | Variable (depends on motor) | Linear: 0.004855*x + (-0.083) |
| **Command ID** | Motor ID + MIT mode | Fixed 0x8094 |
| **Response ID** | Motor master ID | 0x700/0x800 |
| **Data Order** | P,V,T,KP,KD packed | P,V,T,KP,KD + padding |

## 5. C++ Implementation

### Command Creation
```cpp
std::vector<uint8_t> create_ht_mit_command(double position, double velocity,
                                          double torque, double kp, double kd) {
    std::vector<uint8_t> data(12, 0);

    // Conversion constants
    const double RAD_TO_TURN = 1.0 / (2.0 * M_PI);
    const double torque_k = 0.004855;
    const double torque_d = -0.083;

    // Convert to motor units
    double pos_turns = position * RAD_TO_TURN;
    double vel_turns = velocity * RAD_TO_TURN;

    // Convert to int16
    int16_t pos_int = static_cast<int16_t>(pos_turns / 0.0001);
    int16_t vel_int = static_cast<int16_t>(vel_turns / 0.00025);
    int16_t kp_int = static_cast<int16_t>(kp * 10);
    int16_t kd_int = static_cast<int16_t>(kd * 10);
    int16_t torque_int = static_cast<int16_t>((torque - torque_d) / torque_k);

    // Pack as little-endian
    data[0] = pos_int & 0xFF;
    data[1] = (pos_int >> 8) & 0xFF;
    data[2] = vel_int & 0xFF;
    data[3] = (vel_int >> 8) & 0xFF;
    data[4] = torque_int & 0xFF;
    data[5] = (torque_int >> 8) & 0xFF;
    data[6] = kp_int & 0xFF;
    data[7] = (kp_int >> 8) & 0xFF;
    data[8] = kd_int & 0xFF;
    data[9] = (kd_int >> 8) & 0xFF;
    data[10] = 0x00;  // Padding
    data[11] = 0x00;  // Padding

    return data;
}
```

### Response Parsing
```cpp
void parse_ht_response(const can_value_type& frame) {
    if (frame.head.id != 0x700 && frame.head.id != 0x800) return;
    if (frame.head.dlc < 7) return;

    // Extract int16 values (little-endian)
    int16_t pos_int = frame.data[1] | (frame.data[2] << 8);
    int16_t vel_int = frame.data[3] | (frame.data[4] << 8);
    int16_t torque_int = frame.data[5] | (frame.data[6] << 8);

    // Convert to physical units
    const double TURN_TO_RAD = 2.0 * M_PI;
    double position = pos_int * 0.0001 * TURN_TO_RAD;
    double velocity = vel_int * 0.00025 * TURN_TO_RAD;
    double torque = torque_int * 0.004855 + (-0.083);

    // Use the values...
}
```

## 6. Usage Example

### Basic HT Motor Control
```cpp
// Create MIT command
auto cmd = create_ht_mit_command(0.5, 0.1, 1.0, 20.0, 0.5);

// Send command
device->fdcanFrameSend(cmd, 0x8094);

// Wait for response (handled in callback)
```

### Batch HT Motor Control (Multiple Motors)
Based on Python `HTMotorManager.mit_control()`, you can control multiple HT motors in a single command:

```cpp
// Data format for 2 HT motors: [p1,v1,t1,kp1,kd1, p2,v2,t2,kp2,kd2, padding]
std::vector<uint8_t> batch_data(22, 0);  // 10 bytes per motor + 2 bytes padding

// Pack motor 1 data
// ... pack position, velocity, torque, kp, kd for motor 1 ...

// Pack motor 2 data
// ... pack position, velocity, torque, kp, kd for motor 2 ...

// Add padding
batch_data[20] = 0x00;
batch_data[21] = 0x00;

// Send batch command
device->fdcanFrameSend(batch_data, 0x8094);
```

## 7. Testing

### Test Program
A complete test program is available: `test_ht_motor_mit`

### Running the Test
```bash
./test_ht_motor_mit
```

### Test Features
- Creates HT MIT commands with proper scaling
- Sends commands using CAN ID 0x8094
- Listens for responses on IDs 0x700/0x800
- Parses feedback using correct HT motor scaling
- Displays position in both radians and degrees

## 8. Integration with IC_CAN System

The HT motor protocol can be integrated into the main IC_CAN system by:

1. **Creating HTMotor class** that uses this protocol
2. **Setting correct CAN IDs** (0x8094 for commands, 0x700/0x800 for responses)
3. **Implementing proper scaling** in command packing and response parsing
4. **Adding to motor configuration** with HT motor type (motors 7-8)

### Motor Configuration Example
```cpp
// Motor 7: HT4438
motor_configs_.emplace_back(7, MotorType::HT_HIGH_TORQUE, 0x07, 0x07, 40.0, 0.8, 18.0, 15.0, -12.5, 12.5);

// Motor 8: HT4438
motor_configs_.emplace_back(8, MotorType::HT_HIGH_TORQUE, 0x08, 0x08, 40.0, 0.8, 18.0, 15.0, -12.5, 12.5);
```

## 9. Safety Considerations

- **Position Limits**: HT motors typically have ±12.5 rad limits
- **Velocity Limits**: Conservative limit of 15 rad/s recommended
- **Torque Limits**: Conservative limit of 18 Nm based on motor specs
- **Error Handling**: Monitor error codes in status frames
- **Calibration**: HT motors may require zero position calibration

## 10. Troubleshooting

### Common Issues
1. **No Response**: Check CAN IDs (should be 0x700/0x800, not motor ID)
2. **Wrong Scaling**: Ensure proper HT motor scaling constants are used
3. **Command Format**: Verify 12-byte command format with padding
4. **Byte Order**: HT motors use little-endian byte order

### Debug Tips
- Enable debug output to see command data in hex
- Monitor CAN traffic to verify correct IDs and data
- Compare with Python implementation for reference
- Check motor power and connection status

---

**References**:
- Python implementation: `/home/lr-2002/project/InstantCreation/DM_Control_Python/ic_arm_control/control/ht_motor.py`
- Test program: `test_ht_motor_mit.cpp`
- dm-tools SDK documentation