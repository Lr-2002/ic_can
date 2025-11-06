# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Status

This project is to build the robust control pipeline for ic_arm(our team's arm);
We Now have 9 motor on one fdcan line. And we need to control all of them at the target of 500 hz, which need the cpp deployment a lot.

You should consider the continue-makable of the project and organize the system properly. And you could take /Users/lr-2002/project/instantcreation/ic_can/openarm_can for reference.

We use /Users/lr-2002/project/instantcreation/ic_can/dm-tools/USB2FDCAN/SDK/C++ as our control pipeline, you should send and receive according to it.

You should read /Users/lr-2002/project/instantcreation/IC_arm_control/ic_arm_control/control/IC_ARM.py to read the system of our environment.

1. c++ as low-level controller
2. python communication enable(pybind11 for now )
3. cross platform necessary(use less platform-specific function or library)
4. should achieve ic_arm's basic function .
5. for now, do not try to communicate with usb2fdcan, as the robot is not beside me.
6. we have different motors, 8 for arm, 1 for gripper.
    1. DM 10010l
    2. DM 6248
    3. DM 6248
    4. DM 4340
    5. DM 4340
    6. DM 4310
    7. HT 4438
    8. HT 4438

    ----
    servo motor

## Development Setup

When code is added to this repository, this file should be updated with:

- Build commands
- Test execution instructions
- Linting and formatting procedures
- Project architecture overview
- Development workflow guidelines
- update the /home/lr-2002/project/InstantCreation/ic_can/progress.md to display the status.
- write the detail log of update and use git to memorize the change history
- do not delete any file out of /home/lr-2002/project/InstantCreation/ic_can

## Build System

The IC_CAN project uses a modern CMake-based build system with proper dependency management.

### Quick Start (Build Everything)

```bash
# From project root
mkdir build && cd build
cmake .. && make -j$(nproc)
```

### Build Options

You can customize the build with CMake options:

```bash
cmake .. -DBUILD_TOOLS=ON \
         -DBUILD_EXAMPLES=ON \
         -DBUILD_TESTS=ON \
         -DBUILD_PYTHON_BINDINGS=ON \
         -DBUILD_WITH_PINOCCHIO=ON \
         -DBUILD_WITH_MATLAB=OFF \
         -DBUILD_WITH_DM_TOOLS=ON
```

**Available Options:**
- `BUILD_TOOLS` (default: ON) - Build command-line tools
- `BUILD_EXAMPLES` (default: ON) - Build example programs
- `BUILD_TESTS` (default: ON) - Build test suite
- `BUILD_PYTHON_BINDINGS` (default: ON) - Build Python bindings
- `BUILD_WITH_PINOCCHIO` (default: ON) - Enable Pinocchio torque prediction
- `BUILD_WITH_MATLAB` (default: OFF) - Enable MATLAB regressor libraries
- `BUILD_WITH_DM_TOOLS` (default: ON) - Enable dm-tools SDK integration

### Dependencies

**Required:**
- CMake 3.14+
- C++17 compiler
- pkg-config
- pthreads

**Optional (but recommended):**
- Pinocchio 3.x - for advanced torque prediction
- libusb-1.0 - for dm-tools SDK integration
- Python 3.11+ - for Python bindings
- pybind11 - for Python bindings

### Platform-Specific Setup

**Linux (Ubuntu/Debian):**
```bash
# Core dependencies
sudo apt update
sudo apt install cmake build-essential pkg-config

# Pinocchio (robotics library)
sudo apt install libpinocchio-dev

# libusb for dm-tools
sudo apt install libusb-1.0-0-dev

# Python (optional)
sudo apt install python3-dev python3-pip
pip install pybind11
```

**macOS:**
```bash
# Core dependencies
brew install cmake pkg-config

# Pinocchio (requires homebrew-science)
brew tap gepetto/homebrew-robots
brew install pinocchio

# libusb
brew install libusb

# Python
brew install python pybind11
```

### Build Targets

The build system creates the following targets:

**Core Library:**
- `ic_can_core` - Main shared library with all components

**Tools (examples):**
- `set_motor_zero` - Set motors to zero position
- `arm_position_monitor` - Monitor arm joint positions
- `wrist_motor_monitor` - Monitor wrist motors
- `home_to_zero` - Move robot to home position
- `simple_motor_test` - Basic motor testing
- `logging_demo` - Data logging demonstration
- And many more specialized tools...

**Examples:**
- `wrist_gripper_simple_demo` - Simple wrist/gripper demo

**Python Bindings:**
- `ic_can_python` - Python module for IC_CAN

### Build Procedures

**Full Build (Recommended):**
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

**Minimal Build (Core + Basic Tools):**
```bash
mkdir build && cd build
cmake .. -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF -DBUILD_PYTHON_BINDINGS=OFF
make -j$(nproc)
```

**Development Build (Debug + Tests):**
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
make -j$(nproc)
ctest
```

### Installation

```bash
# Install to system (requires sudo)
sudo make install

# Or install to local directory
make install DESTDIR=$HOME/.local
```

### Troubleshooting

**Common Issues:**

1. **Pinocchio not found:**
   ```bash
   export PKG_CONFIG_PATH=/opt/openrobots/lib/pkgconfig:$PKG_CONFIG_PATH
   ```

2. **libusb not found:**
   ```bash
   # Ubuntu/Debian
   sudo apt install libusb-1.0-0-dev

   # macOS
   brew install libusb
   ```

3. **Build fails with cache issues:**
   ```bash
   rm -rf build CMakeCache.txt CMakeFiles/
   mkdir build && cd build
   cmake .. && make -j$(nproc)
   ```

4. **MATLAB tools fail to build:**
   ```bash
   # MATLAB support is experimental and disabled by default
   # Enable with: -DBUILD_WITH_MATLAB=ON
   # Requires MATLAB compiler and proper library setup
   ```

5. **dm-tools integration issues:**
   ```bash
   # Tools will fall back to simulation mode if dm-tools SDK not found
   # Force disable with: -DBUILD_WITH_DM_TOOLS=OFF
   ```

## System Architecture & Components

### Motor Configuration
- **Total Motors**: 9 motors on single FDCAN line
- **Arm Motors (6)**: DM10010L, DM6248 (x2), DM4340 (x2), DM4310
- **Wrist Motors (2)**: HT4438 (x2)
- **Gripper Motor (1)**: Servo motor (CAN ID: 0x19)

### Control Modes
- **TEACH_MODE**: Gravity + friction compensation only (kp=0, kd=0)
  - Arm motors: Receive compensation torques only
  - Wrist motors: Receive compensation torques only
  - Gripper: **Position commands skipped** (stays open naturally)
- **EXECUTION_MODE**: Full position/velocity/torque control with specified gains

### Communication System
- **Primary Backend**: ZLG CAN FD (64-byte frames, 5 Mbps)
- **Fallback**: DM Tools (8-byte frames, 1 Mbps)
- **Target Frequency**: 500 Hz control loop
- **CAN Bus Logging**: Integrated microsecond-precision logging system

### Key Components
1. **Arm Component**: DM motors 1-6 with MIT protocol
2. **Wrist Component**: HT motors 7-8 with MIT protocol
3. **Gripper Component**: Servo motor 9 with position control
4. **CAN Bus Logger**: Core system logging all CAN traffic
5. **Torque Prediction**: Pinocchio-based dynamics + friction compensation

## Recent Updates (2025-11-05)

### ✅ Completed
- **CAN Bus Logger Integration**: Moved from tools to core system architecture
- **Servo Protocol Fix**: Corrected position scaling (radians → 0-4095 range) to match Python implementation
- **TEACH_MODE Gripper Control**: Implemented logic to skip gripper position commands in TEACH_MODE
- **Servo Motor Modes**: All modes implemented (DISABLE, ENABLE, POSITION, READ, MID)

### 🔧 Current Implementation
- **Gripper Logic**:
  - TEACH_MODE: No position commands sent (gripper stays open)
  - Other modes: Full position control available
- **Servo Protocol**: Matches Python `servo_motor.py` implementation exactly
- **Performance**: Achieving 400+ Hz communication rates with microsecond precision logging

### ⚠️ Known Issues
- Gripper hardware behavior may need further testing/verification
- Some DM motors show communication timeouts (normal for hardware)

## Development Guidelines

- **Critical Safety**: All zeros are not safe - motors have installation errors and require calibration
- **Movement Commands**: Ask user before running any commands that cause movement
- **Build Testing**: Always test build after adding new applications/tools
- **File Management**: Do not delete files from project directory
- **Python Path**: Use `/home/lr-2002/anaconda3/bin/python` for Python execution
- **No Mocking**: Use real hardware interfaces, avoid mock implementations

### Component Relationships
- **Independent Use**: Each component (arm, wrist, gripper) can operate standalone
- **Combined Operation**:
  - Arm can include wrist + gripper
  - Wrist can include gripper
  - Hierarchical pipeline design supports flexible composition

## Notes

- The project directory is named `ic_can` related to Controller Area Network (CAN) protocol implementation
- Parent directory is `instantcreation` part of larger project suite
- Target: 500 Hz control loop for 9-motor system on single FDCAN line