# Makefile for IC_CAN Tools
# Copyright 2025 IC_CAN Team

CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

# Include paths (using absolute paths)
CURRENT_DIR = $(shell pwd)
INCLUDES = -I$(CURRENT_DIR)/../include -I$(CURRENT_DIR)/../lib/dm-tools/C++/ubuntu/include -I$(CURRENT_DIR)/ -I/usr/local/include

# Libraries
LIBS = -lusb-1.0 -pthread

# DM-tools object files (using absolute paths)
DM_OBJECTS = $(CURRENT_DIR)/../lib/dm-tools/C++/ubuntu/lib/usb_class.cpp.o \
             $(CURRENT_DIR)/../lib/dm-tools/C++/ubuntu/lib/crc.cpp.o

# Source files (all in src/ directory)
IC_CAN_SOURCE = $(CURRENT_DIR)/../src/ic_can/core/ic_can.cpp
TORQUE_SOURCE = $(CURRENT_DIR)/../src/ic_can/core/torque_predictor.cpp
TOOLS_SOURCE_DIR = $(CURRENT_DIR)/../src/tools

# Tools (all available tools)
TOOLS = set_motor_zero arm_position_monitor home_to_zero logging_demo logging_demo_simple simple_motor_test \
        torque_predictor torque_predictor_pure_c simple_arm_torque_demo demo_real_arm_torques \
        test_regressor test_torque_simple test_torque_integration test_torque_predictor_speed test_torque_with_logging \
        trajectory_executor dynamic_identifier regressor_wrapper

# Tools that need MATLAB regressor libraries
MATLAB_TOOLS = torque_predictor_pure_c test_torque_predictor_speed test_regressor regressor_wrapper

# Pure C MATLAB object files
MATLAB_OBJECTS = $(shell find ../lib/matlab/lib -name "*.o" 2>/dev/null)

.PHONY: all clean install help cmake

all: $(TOOLS)

# Tools that need torque predictor
TORQUE_TOOLS = set_motor_zero arm_position_monitor home_to_zero logging_demo logging_demo_simple simple_motor_test \
               torque_predictor simple_arm_torque_demo demo_real_arm_torques \
               test_torque_simple test_torque_integration test_torque_with_logging trajectory_executor dynamic_identifier

# Tools without torque predictor
SIMPLE_TOOLS = test_regressor test_torque_predictor_speed regressor_wrapper

# Build rule for tools with torque predictor (source files now in src/tools/)
$(TORQUE_TOOLS): %:
	@echo "Building $@ tool..."
	@if [ -n "$(MATLAB_OBJECTS)" ]; then \
		$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(TOOLS_SOURCE_DIR)/$@.cpp $(IC_CAN_SOURCE) $(TORQUE_SOURCE) $(MATLAB_OBJECTS) $(DM_OBJECTS) $(LIBS); \
		echo "✅ $@ built successfully with MATLAB libraries"; \
	else \
		echo "⚠️  MATLAB libraries not found, torque functionality will be limited"; \
		$(CXX) $(CXXFLAGS) $(INCLUDES) -DHAVE_REGRESSOR_LIBS -o $@ $(TOOLS_SOURCE_DIR)/$@.cpp $(IC_CAN_SOURCE) $(TORQUE_SOURCE) $(DM_OBJECTS) $(LIBS); \
		echo "✅ $@ built successfully (with stubs)"; \
	fi

# Build rule for simple tools (regressor tools will be handled separately)
$(SIMPLE_TOOLS): %:
	@echo "Building $@ tool..."

# Build rule for regressor tools with MATLAB support (source files now in src/tools/)
test_regressor test_torque_predictor_speed regressor_wrapper: %:
	@echo "Building $@ tool..."
	@if [ -n "$(MATLAB_OBJECTS)" ]; then \
		$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(TOOLS_SOURCE_DIR)/$@.cpp $(IC_CAN_SOURCE) $(TORQUE_SOURCE) $(MATLAB_OBJECTS) $(DM_OBJECTS) $(LIBS); \
		echo "✅ $@ built successfully with MATLAB libraries"; \
	else \
		echo "⚠️  MATLAB libraries not found, building without them"; \
		$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(TOOLS_SOURCE_DIR)/$@.cpp $(IC_CAN_SOURCE) $(TORQUE_SOURCE) $(DM_OBJECTS) $(LIBS); \
		echo "✅ $@ built successfully (stubs)"; \
	fi

# Special rule for torque_predictor_pure_c (doesn't need torque predictor but needs MATLAB)
torque_predictor_pure_c:
	@echo "Building torque_predictor_pure_c tool..."
	@if [ -n "$(MATLAB_OBJECTS)" ]; then \
		$(CXX) $(CXXFLAGS) $(INCLUDES) -o torque_predictor_pure_c $(TOOLS_SOURCE_DIR)/torque_predictor_pure_c.cpp $(IC_CAN_SOURCE) $(MATLAB_OBJECTS) $(DM_OBJECTS) $(LIBS); \
		echo "✅ torque_predictor_pure_c built successfully with MATLAB libraries"; \
	else \
		echo "⚠️  MATLAB libraries not found, building without them"; \
		$(CXX) $(CXXFLAGS) $(INCLUDES) -o torque_predictor_pure_c $(TOOLS_SOURCE_DIR)/torque_predictor_pure_c.cpp $(IC_CAN_SOURCE) $(DM_OBJECTS) $(LIBS); \
		echo "✅ torque_predictor_pure_c built successfully (stubs)"; \
	fi

clean:
	@echo "Cleaning tools..."
	rm -f $(TOOLS)
	rm -rf build/
	@echo "✅ Clean completed"

install: all
	@echo "Installing tools..."
	mkdir -p ../bin
	cp $(TOOLS) ../bin/
	@echo "✅ Tools installed to ../bin/"

cmake:
	@echo "Building with CMake..."
	mkdir -p build
	cd build && cmake .. && make
	@echo "✅ CMake build completed"

help:
	@echo "Available targets:"
	@echo "  all       - Build all tools with Makefile"
	@echo "  clean     - Remove built files"
	@echo "  install   - Install tools to ../bin/"
	@echo "  cmake     - Build using CMake"
	@echo "  help      - Show this help"
	@echo ""
	@echo "Available tools:"
	@echo "  Basic tools:"
	@echo "    set_motor_zero         - Motor zero position calibration tool"
	@echo "    arm_position_monitor    - Real-time arm position monitoring"
	@echo "    home_to_zero           - Move all motors to zero position"
	@echo "    simple_motor_test      - Basic motor movement test"
	@echo ""
	@echo "  Logging tools:"
	@echo "    logging_demo           - Logging and frequency monitoring demo"
	@echo "    logging_demo_simple    - Simple logging demonstration"
	@echo ""
	@echo "  Torque tools:"
	@echo "    torque_predictor       - Torque prediction tool"
	@echo "    torque_predictor_pure_c - Pure C torque predictor with MATLAB"
	@echo "    simple_arm_torque_demo - Simple torque demonstration"
	@echo "    demo_real_arm_torques  - Real arm torque demonstration"
	@echo ""
	@echo "  Test tools:"
	@echo "    test_regressor         - Regressor testing tool (requires MATLAB)"
	@echo "    test_torque_simple     - Simple torque testing"
	@echo "    test_torque_integration - Torque integration testing"
	@echo "    test_torque_predictor_speed - Torque predictor speed test"
	@echo "    test_torque_with_logging - Torque testing with logging"
	@echo ""
	@echo "  Advanced tools:"
	@echo "    trajectory_executor    - JSON trajectory file executor"
	@echo "    dynamic_identifier     - Dynamic system identification"
	@echo "    regressor_wrapper      - Regressor wrapper tool"
	@echo ""
	@echo "Usage examples:"
	@echo "  make                    # Build all tools"
	@echo "  make set_motor_zero     # Build specific tool"
	@echo "  make torque_predictor   # Build torque predictor"
	@echo "  make install            # Build and install"
	@echo "  make cmake              # Use CMake build system"
	@echo ""
	@echo "Unified directory structure:"
	@echo "  - Source files:  ../src/tools/"
	@echo "  - Executables:    ./tools/"
	@echo "  - Libraries:      ../lib/dm-tools/, ../lib/matlab/"
	@echo "  - Headers:        ../include/"