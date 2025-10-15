# Makefile for IC_CAN Tools
# Copyright 2025 IC_CAN Team

CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

# Include paths (using absolute paths)
CURRENT_DIR = $(shell pwd)
INCLUDES = -I$(CURRENT_DIR)/../include -I$(CURRENT_DIR)/../dm-tools/USB2FDCAN/SDK/C++/ubuntu/include -I$(CURRENT_DIR)/ -I/usr/local/include

# Libraries
LIBS = -lusb-1.0 -pthread

# DM-tools object files (using absolute paths)
DM_OBJECTS = $(CURRENT_DIR)/../dm-tools/USB2FDCAN/SDK/C++/ubuntu/lib/usb_class.cpp.o \
             $(CURRENT_DIR)/../dm-tools/USB2FDCAN/SDK/C++/ubuntu/lib/crc.cpp.o

# Source files
IC_CAN_SOURCE = $(CURRENT_DIR)/../src/ic_can/core/ic_can.cpp

# Tools
TOOLS = set_motor_zero arm_position_monitor home_to_zero logging_demo simple_motor_test trajectory_executor

.PHONY: all clean install help cmake

all: $(TOOLS)

set_motor_zero: set_motor_zero.cpp
	@echo "Building set_motor_zero tool..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o set_motor_zero set_motor_zero.cpp $(IC_CAN_SOURCE) $(DM_OBJECTS) $(LIBS)
	@echo "✅ set_motor_zero built successfully"

arm_position_monitor: arm_position_monitor.cpp
	@echo "Building arm_position_monitor tool..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o arm_position_monitor arm_position_monitor.cpp $(IC_CAN_SOURCE) $(DM_OBJECTS) $(LIBS)
	@echo "✅ arm_position_monitor built successfully"

home_to_zero: home_to_zero.cpp
	@echo "Building home_to_zero tool..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o home_to_zero home_to_zero.cpp $(IC_CAN_SOURCE) $(DM_OBJECTS) $(LIBS)
	@echo "✅ home_to_zero built successfully"

logging_demo: logging_demo.cpp
	@echo "Building logging_demo tool..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o logging_demo logging_demo.cpp $(IC_CAN_SOURCE) $(DM_OBJECTS) $(LIBS)
	@echo "✅ logging_demo built successfully"

simple_motor_test: simple_motor_test.cpp
	@echo "Building simple_motor_test tool..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o simple_motor_test simple_motor_test.cpp $(IC_CAN_SOURCE) $(DM_OBJECTS) $(LIBS)
	@echo "✅ simple_motor_test built successfully"

trajectory_executor: trajectory_executor.cpp
	@echo "Building trajectory_executor tool..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o trajectory_executor trajectory_executor.cpp $(IC_CAN_SOURCE) $(DM_OBJECTS) $(LIBS)
	@echo "✅ trajectory_executor built successfully"

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
	@echo "  set_motor_zero       - Motor zero position calibration tool"
	@echo "  arm_position_monitor  - Real-time arm position monitoring"
	@echo "  home_to_zero         - Move all motors to zero position"
	@echo "  logging_demo         - Logging and frequency monitoring demo"
	@echo "  simple_motor_test    - Basic motor movement test"
	@echo "  trajectory_executor  - JSON trajectory file executor"
	@echo ""
	@echo "Usage examples:"
	@echo "  make                    # Build all tools"
	@echo "  make set_motor_zero     # Build specific tool"
	@echo "  make install            # Build and install"
	@echo "  make cmake              # Use CMake build system"