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

## Notes

- The project directory is named `ic_can` suggesting this may be related to Controller Area Network (CAN) protocol implementation
- Parent directory is `instantcreation` indicating this might be part of a larger project suite
- use  /home/lr-2002/anaconda3/bin/python for python
- do not use mock any time
- follow the old cmake, if you add new app, remeber to fix the cmake and notice to test the build;
- in this system, there are 3 big component: arm, wrist, gripper; their relation are: themselves  could be used singlely, data read, action execution; and they could be combiniation, while there are include info, the arm could include wirst, wrist+gripper; the wrist could include the gripper; so you need to achieve such pipeline.