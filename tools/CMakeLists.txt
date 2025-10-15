# CMakeLists.txt for IC_CAN Tools
# Copyright 2025 IC_CAN Team

cmake_minimum_required(VERSION 3.16)
project(IC_CAN_Tools)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(USB1 REQUIRED libusb-1.0)

# Set include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../dm-tools/USB2FDCAN/SDK/C++/ubuntu/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Common source files
set(IC_CAN_SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ic_can/core/ic_can.cpp
)

set(DM_TOOLS_OBJECTS
    ${CMAKE_CURRENT_SOURCE_DIR}/../dm-tools/USB2FDCAN/SDK/C++/ubuntu/lib/usb_class.cpp.o
    ${CMAKE_CURRENT_SOURCE_DIR}/../dm-tools/USB2FDCAN/SDK/C++/ubuntu/lib/crc.cpp.o
)

# Common libraries
set(COMMON_LIBS
    ${USB1_LIBRARIES}
    pthread
)

# Compiler flags
add_compile_options(-Wall -Wextra)

# Tool 1: set_motor_zero
add_executable(set_motor_zero
    set_motor_zero.cpp
    ${IC_CAN_SOURCE}
)

target_link_libraries(set_motor_zero
    ${DM_TOOLS_OBJECTS}
    ${COMMON_LIBS}
)

target_include_directories(set_motor_zero PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../dm-tools/USB2FDCAN/SDK/C++/ubuntu/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Tool 2: arm_position_monitor
add_executable(arm_position_monitor
    arm_position_monitor.cpp
    ${IC_CAN_SOURCE}
)

target_link_libraries(arm_position_monitor
    ${DM_TOOLS_OBJECTS}
    ${COMMON_LIBS}
)

target_include_directories(arm_position_monitor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../dm-tools/USB2FDCAN/SDK/C++/ubuntu/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Tool 3: home_to_zero
add_executable(home_to_zero
    home_to_zero.cpp
    ${IC_CAN_SOURCE}
)

target_link_libraries(home_to_zero
    ${DM_TOOLS_OBJECTS}
    ${COMMON_LIBS}
)

target_include_directories(home_to_zero PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../dm-tools/USB2FDCAN/SDK/C++/ubuntu/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Tool 4: logging_demo
add_executable(logging_demo
    logging_demo.cpp
    ${IC_CAN_SOURCE}
)

target_link_libraries(logging_demo
    ${DM_TOOLS_OBJECTS}
    ${COMMON_LIBS}
)

target_include_directories(logging_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../dm-tools/USB2FDCAN/SDK/C++/ubuntu/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Custom target to build all tools
add_custom_target(tools
    DEPENDS set_motor_zero arm_position_monitor home_to_zero logging_demo
    COMMENT "Building all IC_CAN tools"
)

# Custom target for cleaning
add_custom_target(clean-tools
    COMMAND ${CMAKE_COMMAND} -E remove -f set_motor_zero arm_position_monitor home_to_zero logging_demo
    COMMENT "Cleaning all built tools"
)

# Installation
install(TARGETS set_motor_zero arm_position_monitor home_to_zero logging_demo
    RUNTIME DESTINATION bin
)

# Print build information
message(STATUS "IC_CAN Tools Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Include Directories: ${CMAKE_CURRENT_SOURCE_DIR}/../include")
message(STATUS "  DM-Tools SDK: ${CMAKE_CURRENT_SOURCE_DIR}/../dm-tools/USB2FDCAN/SDK/C++/ubuntu/include")
message(STATUS "  Libraries: ${COMMON_LIBS}")
message(STATUS "  Tools to build: set_motor_zero, arm_position_monitor, home_to_zero, logging_demo")