# IC_CAN High-Level Python API

Production-ready Python interface for the IC_ARM robotic system, optimized for reinforcement learning and deep neural network applications.

## Overview

The high-level API provides a clean, unified interface for controlling the 9-DOF IC_ARM robot with just **4 core methods**:

1. `enable()` - Enable motors for control
2. `refresh()` - Read current motor state
3. `step(p, v, t)` - Send position/velocity/torque commands
4. `set_control_mode()` - Switch between TEACH/EXECUTION modes

This interface abstracts away low-level motor details while maintaining **500Hz real-time performance** and comprehensive safety features.

## Quick Start

```python
import ic_can
import numpy as np

# Initialize robot
robot = ic_can.quick_start()  # Auto-initializes

# Basic control loop
robot.enable()
robot.set_control_mode(ic_can.ControlMode.EXECUTION)

# Read state
state = robot.refresh()
print(f"Positions: {state.positions}")

# Send command
target = np.array([0.1, -0.1, 0.1, -0.1, 0.1, -0.1, 0.0, 0.0, 0.0])
robot.step(target)

# Cleanup
robot.shutdown()
```

## Key Features

### 🚀 Production Ready
- **Real-time Performance**: 500Hz control loop capability
- **Safety Systems**: Built-in limits and emergency stop
- **Error Handling**: Comprehensive error detection and recovery
- **Thread Safety**: Safe for multi-threaded applications

### 🧠 RL/DNN Optimized
- **Clean State Space**: 9-dimensional unified motor state
- **NumPy Integration**: Seamless ML framework compatibility
- **Vectorized Operations**: Efficient batch processing
- **Flat State Representation**: Easy integration with RL algorithms

### ⚡ High Performance
- **Direct C++ Binding**: No Python overhead in control loop
- **Memory Efficient**: Optimized state representation
- **Non-blocking**: Fast state refresh and command sending

## API Reference

### RobotArm Class

#### Initialization

```python
robot = RobotArm(device_sn="F561E08C892274DB09496BCC1102DBC5",
                 debug=False,
                 safety_limits=True)
```

**Parameters:**
- `device_sn`: Device serial number for CAN communication
- `debug`: Enable debug logging
- `safety_limits`: Enable automatic position/velocity/torque limiting

#### Core Methods

##### enable(motor_ids=None)
Enable motors for control.

**Parameters:**
- `motor_ids`: Optional list of motor IDs (1-9). If None, enables all motors.

**Returns:** `bool` - True if successful

```python
# Enable all motors
robot.enable()

# Enable specific motors
robot.enable([1, 2, 3, 4, 5, 6])  # Arm motors only
```

##### refresh()
Read current motor state.

**Returns:** `MotorState` object with current state of all 9 motors

```python
state = robot.refresh()
print(f"Positions: {state.positions}")
print(f"Velocities: {state.velocities}")
print(f"Torques: {state.torques}")
print(f"Safe: {state.is_safe()}")
```

##### step(positions, velocities=None, torques=None)
Execute a single control step.

**Parameters:**
- `positions`: Target positions for all 9 motors (radians)
- `velocities`: Optional velocity limits (rad/s)
- `torques`: Optional torque limits (Nm)

**Returns:** `bool` - True if command sent successfully

```python
# Position control only
robot.step([0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.0, 0.0, 0.0])

# With velocity limits
robot.step(positions, velocities=[1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.5, 0.5, 0.5])

# With torque limits
robot.step(positions, torques=[5.0, 5.0, 5.0, 5.0, 3.0, 3.0, 2.0, 2.0, 1.0])
```

##### set_control_mode(mode)
Switch between control modes.

**Parameters:**
- `mode`: "TEACH" or "EXECUTION" (or ControlMode enum)

**Returns:** `bool` - True if mode switched successfully

```python
# Teach mode: gravity compensation only
robot.set_control_mode("TEACH")

# Execution mode: full position control
robot.set_control_mode("EXECUTION")

# Using enum
robot.set_control_mode(ic_can.ControlMode.EXECUTION)
```

#### Safety Methods

##### emergency_stop()
Immediately stop all robot motion.

```python
robot.emergency_stop()
```

##### is_safe()
Check if robot is in a safe state.

**Returns:** `bool`

```python
if robot.is_safe():
    robot.step(action)
else:
    robot.emergency_stop()
```

### MotorState Class

The `MotorState` class provides a clean representation of robot state:

```python
@dataclass
class MotorState:
    positions: np.ndarray      # Joint angles (radians)
    velocities: np.ndarray     # Joint velocities (rad/s)
    torques: np.ndarray        # Joint torques (Nm)
    temperatures: np.ndarray   # Motor temperatures (Celsius)
    enabled: np.ndarray        # Motor enable status
    errors: np.ndarray         # Motor error status
    timestamp: float           # Unix timestamp
    control_mode: ControlMode  # Current control mode
    motor_count: int           # Always 9
```

#### Key Methods

##### to_vector()
Convert state to flat vector for RL algorithms:

```python
state_vector = state.to_vector()  # Shape: (27,) = 9 pos + 9 vel + 9 torque
```

##### is_safe()
Check if state is safe:

```python
if state.is_safe():
    print("Robot is in safe state")
```

##### get_arm_positions(), get_wrist_positions(), get_gripper_position()
Access specific motor groups:

```python
arm_pos = state.get_arm_positions()      # Motors 1-6
wrist_pos = state.get_wrist_positions()  # Motors 7-8
gripper_pos = state.get_gripper_position()  # Motor 9
```

### MotorCommand Class

The `MotorCommand` class represents control commands:

```python
command = MotorCommand(positions=np.zeros(9))
command = MotorCommand.from_arm_wrist_gripper(
    arm_pos=[0.1, 0.1, 0.1, 0.1, 0.1, 0.1],
    wrist_pos=[0.0, 0.0],
    gripper_pos=0.0
)
```

## Control Modes

### TEACH_MODE
- Gravity compensation + friction compensation only
- Motors remain free to move
- Safe for manual teaching and interaction
- Position commands are ignored (except gripper)

### EXECUTION_MODE
- Full position/velocity/torque control
- Motors actively track target positions
- Requires proper gain tuning
- Used for automated control and RL

## RL Integration

### Basic RL Loop

```python
import numpy as np
import ic_can

# Initialize environment
robot = ic_can.create_robot_arm()
robot.enable()
robot.set_control_mode(ic_can.ControlMode.EXECUTION)

# RL control loop
for episode in range(num_episodes):
    state = robot.refresh()
    state_vector = state.to_vector()

    while not done:
        # Get action from policy
        action = policy.get_action(state_vector)

        # Send action to robot
        success = robot.step(action)

        # Get new state
        new_state = robot.refresh()
        new_state_vector = new_state.to_vector()

        # Compute reward
        reward = compute_reward(new_state)

        # Update policy
        policy.update(state_vector, action, reward, new_state_vector)

        state = new_state
        state_vector = new_state_vector
```

### Gym Environment Template

See `examples/rl_environment_template.py` for a complete gym-compatible environment template that integrates with popular RL frameworks like Stable Baselines3, RLlib, etc.

## Safety Features

### Built-in Limits

When `safety_limits=True` (default), the system automatically enforces:

- **Position limits**: ±π radians
- **Velocity limits**: 2.0 rad/s
- **Torque limits**: 10.0 Nm

### Safety Monitoring

```python
# Check robot safety
if robot.is_safe():
    print("Robot is safe")
else:
    print("Robot safety compromised!")

# Check individual motor state
state = robot.refresh()
if state.has_errors():
    print(f"Motor errors detected: {state.errors}")

if not state.is_within_limits():
    print("Joint limits exceeded!")
```

### Emergency Procedures

```python
# Emergency stop
robot.emergency_stop()

# Safe shutdown
robot.set_control_mode(ic_can.ControlMode.TEACH)  # Safe mode
robot.disable()  # Disable motors
robot.shutdown()  # Full shutdown
```

## Performance Considerations

### Real-time Operation

The API is designed for real-time performance:

- **State refresh**: ~1-2ms
- **Command sending**: ~1-2ms
- **Maximum frequency**: 500Hz+

### Performance Monitoring

```python
# Get performance statistics
stats = robot.get_performance_stats()
print(f"Average refresh time: {stats['last_refresh_time']*1000:.2f}ms")
print(f"Total refreshes: {stats['refresh_count']}")
```

### Memory Efficiency

The state representation uses contiguous NumPy arrays for optimal performance with ML frameworks.

## Examples

### Basic Control

```python
import ic_can
import numpy as np

with ic_can.RobotArm() as robot:
    robot.enable()
    robot.set_control_mode(ic_can.ControlMode.EXECUTION)

    # Move to zero position
    robot.step(np.zeros(9))

    # Sinusoidal motion
    for t in range(100):
        action = 0.1 * np.sin(t * 0.1) * np.ones(9)
        robot.step(action)
        state = robot.refresh()
        print(f"Step {t}: pos_range=[{np.min(state.positions):.3f}, {np.max(state.positions):.3f}]")
```

### Context Manager Usage

```python
# Automatic cleanup
with ic_can.quick_start() as robot:
    robot.enable()
    state = robot.refresh()
    robot.step([0.1] * 9)
# Automatic shutdown occurs here
```

### Error Handling

```python
try:
    robot = ic_can.create_robot_arm()
    robot.enable()

    # Your control code here

except Exception as e:
    print(f"Robot error: {e}")
    robot.emergency_stop()
finally:
    robot.shutdown()
```

## Troubleshooting

### Common Issues

1. **Initialization fails**: Check device connection and serial number
2. **Motors don't enable**: Ensure safety systems are not triggered
3. **Commands ignored**: Check control mode is set to EXECUTION
4. **High latency**: Reduce debug logging and check system load

### Debug Mode

Enable debug logging for detailed information:

```python
robot = ic_can.RobotArm(debug=True)
# or
robot = ic_can.create_robot_arm(debug=True)
```

### Hardware Simulation

For development without hardware, the system supports simulation mode through the underlying C++ implementation.

## Architecture

### High-Level Overview

```
RL/DNN Application
         ↓
RobotArm Python Class (High-level API)
         ↓
MotorState/MotorCommand Classes (Data structures)
         ↓
pybind11 Bindings (C++ interface)
         ↓
IC_CAN C++ Library (Real-time control)
         ↓
Hardware Interface (CAN communication)
```

### Design Principles

1. **Abstraction**: Hide low-level complexity
2. **Performance**: Maintain real-time capabilities
3. **Safety**: Built-in protection systems
4. **Flexibility**: Support various use cases
5. **Reliability**: Production-ready error handling

## File Structure

```
python/
├── ic_can/
│   ├── __init__.py              # Package initialization
│   ├── motor_state.py           # State/command data structures
│   ├── robot_arm.py             # High-level RobotArm class
│   ├── core.py                  # Existing controller interface
│   └── exceptions.py            # Custom exceptions
├── src/
│   └── ic_can_python.cpp        # pybind11 bindings (extended)
├── include/
│   └── ic_can/
│       └── motor_state.hpp      # C++ state structures
└── examples/
    ├── high_level_api_demo.py   # API demonstration
    └── rl_environment_template.py # RL integration template
```

## Contributing

When extending the API:

1. **Maintain backwards compatibility**
2. **Add comprehensive tests**
3. **Update documentation**
4. **Follow existing code style**
5. **Consider RL/DNN use cases**

## License

Apache License 2.0 - see LICENSE file for details.