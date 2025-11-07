# IC_CAN Configuration System

This directory contains configuration files for the IC_CAN robot control system.

## Configuration Files

- `ic_can_config.yaml` - Main configuration file with all system settings

## Configuration Structure

The configuration is organized into several main sections:

### System Configuration
- **frequency**: Control frequency in Hz (default: 500.0)
- **control_mode**: TEACH_MODE or EXECUTION_MODE
- **safety_enabled**: Enable safety monitoring
- **performance_monitoring**: Enable performance monitoring

### Debug Configuration
- **debug_level**: DEBUG, INFO, WARN, ERROR
- **can_debug**: Enable CAN frame debugging
- **motor_debug**: Enable motor-specific debugging
- **system_debug**: Enable system-level debugging

### Logging Configuration
- **can_logger**: CAN bus logging settings
- **states_logger**: Motor states logging settings
- **cmd_logger**: Command logging settings

### Motor Configuration
- **arm_motors**: Motors 1-6 (arm joints)
- **wrist_motors**: Motors 7-8 (wrist joints)
- **gripper_motors**: Motor 9 (gripper servo)

Each motor configuration includes:
- Joint number and motor type
- CAN communication IDs
- Control gains (Kp, Kd, Ki)
- Position, velocity, and torque limits
- Friction compensation parameters
- Zero calibration offset

### Communication Configuration
- **preferred_backend**: zlg, dm_tools, simulation, auto
- **zlg_can_fd**: ZLG CAN FD adapter settings
- **dm_tools**: DM Tools SDK settings
- **simulation**: Simulation backend settings

### Advanced Features
- **gravity_compensation**: Gravity compensation settings
- **friction_compensation**: Friction compensation parameters
- **trajectory_execution**: Trajectory execution settings
- **safety_monitoring**: Safety monitoring configuration

## Usage

To use a custom configuration file:

```cpp
#include "ic_can/core/config.hpp"

// Create config manager with custom file
ic_can::ConfigManager config_manager("/path/to/your/config.yaml");

// Check if configuration loaded successfully
if (!config_manager.is_config_loaded()) {
    std::cerr << "Failed to load configuration" << std::endl;
    return -1;
}

// Get configuration sections
auto system_config = config_manager.get_system_config();
auto motors_config = config_manager.get_motors_config();
auto comm_config = config_manager.get_communication_config();

// Print configuration summary
config_manager.print_config_summary();
```

## Configuration Validation

The configuration system validates:
- Motor joint numbers (no duplicates)
- System frequency range (0-10000 Hz)
- Communication backend availability
- Motor limits and parameters

## Environment Variables

Configuration values can reference environment variables using `${VARIABLE_NAME}` syntax. These will be automatically substituted when loading the configuration.

## Default Values

If a configuration file is not provided, the system uses sensible defaults that are suitable for most IC_CAN applications.