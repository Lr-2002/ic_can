# IC_CAN - High Performance CAN Control Library for Robotic Arms

IC_CAN is a comprehensive C++ library with Python bindings for high-performance control of robotic arms via CAN bus communication. The library supports 9 motors (6 DM motors + 2 HT motors + 1 servo motor) on a single FDCAN line at 500Hz control frequency.

## Features

- **High Performance**: 500Hz control loop with low-latency CAN communication
- **Multi-Motor Support**: 9 motor control (6 arm joints + gripper with 3 motors)
- **Motor Types**: Damiao (DM) motors, High Torque (HT) motors, Servo motors
- **Safety System**: Comprehensive safety monitoring and emergency stop functionality
- **Python Bindings**: Full Python interface using PyBind11
- **Cross-Platform**: Support for Linux, macOS, and Windows
- **Real-time Control**: High-frequency control thread management
- **Component-based Architecture**: Modular design with Arm, Gripper, and Safety components

## System Architecture

The IC_CAN system follows a 6-layer architecture:

1. **Hardware Abstraction Layer**: USB2CAN adapter and CAN communication
2. **Motor Driver Layer**: Individual motor drivers for different motor types
3. **Component Layer**: High-level components (Arm, Gripper, Safety)
4. **Control Layer**: Main controller and coordination logic
5. **Application Layer**: User applications and interfaces
6. **Python Interface**: Python bindings and high-level APIs

## Quick Start

### Prerequisites

- CMake 3.16 or higher
- C++17 compatible compiler (GCC 7+, Clang 6+, MSVC 2019+)
- Python 3.7 or higher (for Python bindings)
- PyBind11 (for Python bindings)
- GTest (for testing, optional)

### Building

```bash
# Clone the repository
git clone https://github.com/your-org/ic_can.git
cd ic_can

# Run the build and test script
./build_and_test.sh

# For debug build with coverage
./build_and_test.sh --debug --coverage

# For clean build
./build_and_test.sh --clean
```

### C++ Usage

```cpp
#include <ic_can/core/ic_can.hpp>

int main() {
    // Create controller
    ic_can::IC_CAN controller("F561E08C892274DB09496BCC1102DBC5");

    // Initialize
    if (!controller.initialize()) {
        std::cerr << "Failed to initialize controller" << std::endl;
        return -1;
    }

    // Enable motors
    controller.enable_all();

    // Start high-frequency control (500Hz)
    controller.start_high_frequency_control();

    // Control loop
    for (int i = 0; i < 1000; ++i) {
        // Get current state
        auto positions = controller.get_joint_positions();
        auto velocities = controller.get_joint_velocities();

        // Set new positions
        std::vector<double> target_positions = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7};
        controller.set_joint_positions(target_positions);

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    // Stop control
    controller.stop_high_frequency_control();
    controller.disable_all();

    return 0;
}
```

### Python Usage

```python
from ic_can.core import IC_CANController
import time

# Create controller
controller = IC_CANController("F561E08C892274DB09496BCC1102DBC5", debug=True)

# Initialize
if not controller.initialize():
    print("Failed to initialize controller")
    exit(1)

# Enable motors
controller.enable_all()

# Start high-frequency control
controller.start_high_frequency_control(500.0)  # 500Hz

# Control loop
for i in range(1000):
    # Get current state
    positions = controller.get_joint_positions()
    velocities = controller.get_joint_velocities()

    # Set new positions
    target_positions = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7]
    controller.set_joint_positions(target_positions)

    time.sleep(0.002)  # 2ms

# Stop control
controller.stop_high_frequency_control()
controller.disable_all()
```

## Motor Configuration

The system supports 9 motors with the following configuration:

| Motor ID | Motor Type | CAN ID  | Joint        |
|----------|------------|---------|--------------|
| 1        | DM10010L     | 0x01   | Joint 1      |
| 2        | DM6248     | 0x02   | Joint 2      |
| 3        | DM6248     | 0x03   | Joint 3      |
| 4        | DM4340     | 0x05   | Joint 4      |
| 6        | DM4340     | 0x06   | Joint 5      |
| 5        | DM4310     | 0x06   | Joint 6      |
| 7        | HT_HIGH_TORQUE   | 0x07   | Gripper M1   |
| 8        | HT_HIGH_TORQUE | 0x08 | Gripper M2  |
| 9        | SERVO      | 0x09   | Gripper Servo|

## Safety Features

The library includes comprehensive safety features:

- **Position Limits**: Joint position boundary checking
- **Velocity Limits**: Maximum velocity constraints
- **Torque Limits**: Torque safety boundaries
- **Temperature Monitoring**: Motor temperature protection
- **Emergency Stop**: Immediate stop functionality
- **Watchdog Timer**: Communication timeout protection
- **Error Reporting**: Detailed error and violation reporting

## Performance

The system is designed for high-performance operation:

- **Control Frequency**: Up to 500Hz control loop
- **Latency**: Sub-millisecond command execution
- **Throughput**: High-speed CAN communication
- **Memory**: Efficient memory management
- **CPU**: Optimized for multi-core systems

## Testing

The project includes comprehensive testing:

### C++ Tests

```bash
cd build
./test_results/test_basic      # Basic functionality tests
./test_results/test_performance # Performance benchmarks
./test_results/test_motors     # Motor-specific tests
./test_results/test_safety     # Safety system tests
```

### Python Tests

```bash
python3 python/test_functionality.py  # Functional tests
python3 python/test_performance.py    # Performance tests
```

## Examples

The `examples/` directory contains several example programs:

- `basic_control.cpp`: Basic motor control example
- `high_frequency_control.cpp`: High-frequency control demonstration
- `motor_test.cpp`: Individual motor testing
- `safety_monitor.cpp`: Safety monitoring example
- `performance_benchmark.cpp`: Performance benchmarking

## Documentation

- **API Reference**: Detailed API documentation (in docs/)
- **Architecture Guide**: System architecture description
- **Motor Protocol**: CAN communication protocol details
- **Safety Manual**: Safety system documentation

## Simulation Mode

When real hardware is not available, the library operates in simulation mode:

- All motor operations are simulated
- CAN communication is mocked
- Safety systems are emulated
- Performance characteristics may differ

See `mock_exp.md` for detailed information about simulated components.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Run the test suite
6. Submit a pull request

## License

This project is licensed under the Apache License 2.0. See the [LICENSE](LICENSE) file for details.

## Support

For support and questions:

- Email: <openarm@enactic.ai>
- GitHub Issues: <https://github.com/your-org/ic_can/issues>
- Documentation: <https://ic-can.readthedocs.io>

## Acknowledgments

- dm-tools SDK for USB2CAN communication
- PyBind11 for Python bindings
- Google Test for testing framework
- CAN protocol specifications

## Version History

- **v1.0.0**: Initial release with basic functionality
- **v1.1.0**: Added high-frequency control and safety features
- **v1.2.0**: Python bindings and performance optimizations

---

**Note**: This library is designed for research and development use. Production deployment requires additional validation and safety certifications.

