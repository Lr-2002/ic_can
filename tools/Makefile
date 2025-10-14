# Makefile for IC_CAN Tools
# Copyright 2025 IC_CAN Team

CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

# Include paths (using absolute paths)
CURRENT_DIR = $(shell pwd)
INCLUDES = -I$(CURRENT_DIR)/../include -I$(CURRENT_DIR)/../dm-tools/USB2FDCAN/SDK/C++/ubuntu/include -I$(CURRENT_DIR)/

# Libraries
LIBS = -lusb-1.0 -pthread

# DM-tools object files (using absolute paths)
DM_OBJECTS = $(CURRENT_DIR)/../dm-tools/USB2FDCAN/SDK/C++/ubuntu/lib/usb_class.cpp.o \
             $(CURRENT_DIR)/../dm-tools/USB2FDCAN/SDK/C++/ubuntu/lib/crc.cpp.o

# Source files
IC_CAN_SOURCE = $(CURRENT_DIR)/../src/ic_can/core/ic_can.cpp

# Tools
TOOLS = set_motor_zero

.PHONY: all clean

all: $(TOOLS)

set_motor_zero: set_motor_zero.cpp
	@echo "Building set_motor_zero tool..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o set_motor_zero set_motor_zero.cpp $(IC_CAN_SOURCE) $(DM_OBJECTS) $(LIBS)
	@echo "✅ set_motor_zero built successfully"

clean:
	@echo "Cleaning tools..."
	rm -f $(TOOLS)
	@echo "✅ Clean completed"

install: all
	@echo "Installing tools..."
	cp $(TOOLS) ../bin/
	@echo "✅ Tools installed to ../bin/"

help:
	@echo "Available targets:"
	@echo "  all       - Build all tools"
	@echo "  clean     - Remove built files"
	@echo "  install   - Install tools to ../bin/"
	@echo "  help      - Show this help"