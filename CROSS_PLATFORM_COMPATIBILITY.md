# IC_CAN Cross-Platform Compatibility

This document outlines the cross-platform compatibility of the IC_CAN library and provides instructions for building on different platforms.

## Supported Platforms

### ✅ macOS (Intel and Apple Silicon)
- **Minimum Version**: macOS 10.15 (Catalina)
- **Compiler**: Clang 12.0+ (included with Xcode)
- **Dependencies**:
  - libusb-1.0 (via Homebrew)
  - pybind11 (for Python bindings)
  - CMake 3.12+
  - Python 3.8+ (for Python bindings)

### ✅ Linux (Ubuntu/Debian, CentOS/RHEL, Arch)
- **Minimum Version**: Linux kernel 4.15+
- **Compiler**: GCC 7.0+ or Clang 8.0+
- **Dependencies**:
  - libusb-1.0-0-dev
  - pybind11-dev (for Python bindings)
  - CMake 3.12+
  - Python 3.8+ (for Python bindings)

### ✅ NVIDIA Jetson (Orin AGX)
- **Platform**: ARM64 aarch64
- **OS**: Ubuntu 20.04+ for Jetson
- **Compiler**: GCC 9.0+ (default on JetPack)
- **Dependencies**:
  - libusb-1.0-0-dev
  - pybind11-dev (for Python bindings)
  - CMake 3.12+
  - Python 3.8+ (for Python bindings)
  - CUDA Toolkit (optional, for GPU acceleration)

## Platform-Specific Features

### macOS
- **USB Access**: Requires user permission for USB devices
- **Real-time Performance**: Good thread scheduling, suitable for 500Hz control
- **Python**: Anaconda or Homebrew Python supported
- **Architecture**: Universal Binary (Intel x86_64 + Apple Silicon arm64)

### Linux
- **USB Access**: May require udev rules for non-root USB access
- **Real-time Performance**: Excellent with PREEMPT_RT kernel
- **Python**: System Python or conda/miniconda supported
- **Architecture**: x86_64, ARM64 (aarch64)

### NVIDIA Jetson (Orin AGX)
- **USB Access**: Direct USB access available
- **Real-time Performance**: Optimized for real-time control
- **Power Management**: Jetson-specific power modes supported
- **GPU Acceleration**: CUDA integration available for performance-critical operations

## Build Instructions

### macOS

```bash
# Install dependencies
brew install cmake libusb pybind11

# Clone and build
git clone <repository-url>
cd ic_can
mkdir build && cd build
cmake .. -DBUILD_EXAMPLES=ON -DBUILD_PYTHON_BINDINGS=ON
make -j$(sysctl -n hw.ncpu)
```

### Linux

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt update
sudo apt install cmake build-essential libusb-1.0-0-dev python3-dev pybind11-dev

# Clone and build
git clone <repository-url>
cd ic_can
mkdir build && cd build
cmake .. -DBUILD_EXAMPLES=ON -DBUILD_PYTHON_BINDINGS=ON
make -j$(nproc)
```

### NVIDIA Jetson (Orin AGX)

```bash
# Install dependencies
sudo apt update
sudo apt install cmake build-essential libusb-1.0-0-dev python3-dev pybind11-dev

# Clone and build
git clone <repository-url>
cd ic_can
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON -DBUILD_PYTHON_BINDINGS=ON
make -j$(nproc)

# Optional: Enable GPU acceleration
# cmake .. -DENABLE_CUDA=ON -DCMAKE_CUDA_ARCHITECTURES=87
```

## Configuration Options

### Build Type
- **Debug**: `-DCMAKE_BUILD_TYPE=Debug`
- **Release**: `-DCMAKE_BUILD_TYPE=Release` (default)
- **RelWithDebInfo**: `-DCMAKE_BUILD_TYPE=RelWithDebInfo`

### Feature Flags
- **Build Examples**: `-DBUILD_EXAMPLES=ON`
- **Build Tests**: `-DBUILD_TESTS=ON`
- **Python Bindings**: `-DBUILD_PYTHON_BINDINGS=ON`
- **Simulation Mode**: `-DSIMULATE_DM_TOOLS=ON` (when dm-tools SDK unavailable)

### Platform-Specific Options

#### macOS
```bash
# Target specific macOS version
cmake .. -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15

# Universal binary (Intel + Apple Silicon)
cmake .. -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
```

#### Linux
```bash
# Enable real-time scheduling
cmake .. -DENABLE_RT_SCHEDULING=ON

# Specific compiler
cmake .. -DCMAKE_CXX_COMPILER=g++-9
```

#### NVIDIA Jetson
```bash
# Optimize for Jetson
cmake .. -DJETSON_OPTIMIZATION=ON

# Enable CUDA (if available)
cmake .. -DENABLE_CUDA=ON
```

## Runtime Requirements

### Hardware Requirements
- **USB Port**: USB 2.0 or higher for USB2CAN adapter
- **Processor**: 64-bit processor required
- **Memory**: Minimum 512MB RAM, 1GB+ recommended

### Software Requirements
- **Operating System**: As listed in supported platforms
- **USB Drivers**: libusb-1.0 compatible drivers
- **Python**: 3.8+ (for Python bindings)
- **CAN Hardware**: Compatible USB2CAN adapter with dm-tools SDK

## Performance Considerations

### Real-Time Performance
- **Target Frequency**: 500Hz control loop
- **Latency Requirements**: <2ms maximum latency
- **CPU Usage**: Typically <10% on modern processors

### Platform-Specific Tuning

#### macOS
- Use `thread_policy_set` for real-time priority
- Enable power-saving mode optimizations
- Consider external GPU for intensive computations

#### Linux
- Use `sched_setscheduler` with SCHED_FIFO or SCHED_RR
- Configure CPU affinity for control threads
- Enable PREEMPT_RT kernel for best performance

#### NVIDIA Jetson
- Use NVIDIA's power management tools
- Configure CPU governors for performance
- Utilize GPU acceleration for matrix operations

## Troubleshooting

### Common Issues

#### USB Permission Errors
**macOS**: Grant USB permission in System Preferences
**Linux**: Add udev rules or run with sudo

#### Compilation Errors
- Ensure CMake 3.12+ is installed
- Check compiler version compatibility
- Verify all dependencies are installed

#### Python Import Errors
- Verify Python path configuration
- Check pybind11 version compatibility
- Ensure shared library is in LD_LIBRARY_PATH (Linux) or DYLD_LIBRARY_PATH (macOS)

#### Performance Issues
- Check system load and CPU usage
- Verify real-time scheduling permissions
- Monitor thread priorities and affinities

### Platform-Specific Debugging

#### macOS
```bash
# Check system load
top -l 1

# Monitor threads
sample <process-id> 10

# USB debugging
system_profiler SPUSBDataType
```

#### Linux
```bash
# Check system load
htop

# Monitor threads
htop -H

# USB debugging
lsusb -v

# Real-time status
chrt -f -p <priority> <thread-id>
```

#### NVIDIA Jetson
```bash
# Check system status
tegrastats

# Monitor GPU usage
tegrastats --interval 1000

# Power management
sudo nvpmodel -m 0
sudo jetson_clocks
```

## Testing

### Unit Tests
```bash
# Run all tests
cd build
ctest

# Run specific test
ctest -R test_motor
```

### Performance Benchmarks
```bash
# Run performance tests
./examples/performance_benchmark

# Monitor frequency
./examples/high_frequency_control
```

### Integration Tests
```bash
# Basic functionality test
./examples/basic_control

# Component testing
./examples/component_test
```

This ensures the IC_CAN library works reliably across macOS, Linux, and NVIDIA Jetson platforms while maintaining the required 500Hz control performance.