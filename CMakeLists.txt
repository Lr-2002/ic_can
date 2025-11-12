# Copyright 2025 IC_CAN Team
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.14)
project(IC_CAN VERSION 1.0.0 LANGUAGES CXX)

# ===================================
# PROJECT CONFIGURATION
# ===================================

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform Detection
if(APPLE)
    set(IC_CAN_PLATFORM "macOS")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS architecture")
    else()
        set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "macOS architecture")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(IC_CAN_PLATFORM "Linux")
    if(DEFINED ENV{JETSON_PLATFORM})
        set(IC_CAN_PLATFORM "Jetson")
        add_definitions(-DJETSON_PLATFORM)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a78.cortex-a53")
    endif()
elseif(WIN32)
    set(IC_CAN_PLATFORM "Windows")
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
endif()

message(STATUS "Building for platform: ${IC_CAN_PLATFORM}")

# Build Type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler Options
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")
    if(NOT WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    endif()
endif()

# ===================================
# BUILD OPTIONS
# ===================================

option(BUILD_TOOLS "Build IC_CAN tools" ON)
option(BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_TESTS "Build test suite" OFF)
option(BUILD_PYTHON_BINDINGS "Build Python bindings" ON)
option(BUILD_WITH_PINOCCHIO "Enable Pinocchio torque prediction" ON)
option(BUILD_WITH_MATLAB "Enable MATLAB regressor libraries" OFF)
option(BUILD_WITH_DM_TOOLS "Enable dm-tools SDK integration" ON)
option(BUILD_WITH_ZLG_CANFD "Enable ZLG CAN FD communication backend" ON)

# ===================================
# DEPENDENCY CONFIGURATION
# ===================================

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# YAML-cpp for configuration management
find_package(yaml-cpp REQUIRED)

# Include directories (global)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/dm-tools/C++/ubuntu/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/regressor/lib/regressorWithMotorDynamics
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/regressor/lib/frictionRegressor
    ${CMAKE_CURRENT_BINARY_DIR}/nlohmann_json
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/zlg_canfd
)

# Pinocchio Configuration
if(BUILD_WITH_PINOCCHIO)
    set(PKG_CONFIG_PATH $ENV{PKG_CONFIG_PATH} /opt/openrobots/lib/pkgconfig)
    set(CMAKE_PREFIX_PATH "/opt/openrobots/lib/pkgconfig/../../" ${CMAKE_PREFIX_PATH})

    pkg_check_modules(PINOCCHIO pinocchio)
    if(PINOCCHIO_FOUND)
        message(STATUS "✅ Found Pinocchio: ${PINOCCHIO_VERSION}")
        message(STATUS "Pinocchio include directories: ${PINOCCHIO_INCLUDE_DIRS}")
        set(PINOCCHIO_INCLUDE_DIRS "/opt/openrobots/include" ${PINOCCHIO_INCLUDE_DIRS})
        add_definitions(-DHAVE_PINOCCHIO)
    else()
        message(WARNING "❌ Pinocchio not found. Pinocchio-based torque predictor will not be available.")
        set(PINOCCHIO_INCLUDE_DIRS "")
        set(PINOCCHIO_LIBRARIES "")
        add_definitions(-DPINOCCHIO_NOT_AVAILABLE)
    endif()
else()
    message(STATUS "Pinocchio disabled by BUILD_WITH_PINOCCHIO=OFF")
    set(PINOCCHIO_INCLUDE_DIRS "")
    set(PINOCCHIO_LIBRARIES "")
    add_definitions(-DPINOCCHIO_NOT_AVAILABLE)
endif()

# libusb Configuration
if(BUILD_WITH_DM_TOOLS)
    find_library(LIBUSB_LIBRARY
        NAMES usb-1.0 libusb-1.0
        PATHS /usr/lib /usr/local/lib /opt/homebrew/lib
        DOC "libusb library for dm-tools SDK"
    )

    if(NOT LIBUSB_LIBRARY)
        message(WARNING "libusb not found. dm-tools SDK integration will use simulation.")
        set(LIBUSB_LIBRARY "")
        add_definitions(-DSIMULATE_DM_TOOLS)
    else()
        message(STATUS "✅ Found libusb: ${LIBUSB_LIBRARY}")
    endif()
else()
    message(STATUS "dm-tools SDK disabled by BUILD_WITH_DM_TOOLS=OFF")
    set(LIBUSB_LIBRARY "")
    add_definitions(-DSIMULATE_DM_TOOLS)
endif()

# Platform-specific libusb headers
if(APPLE AND BUILD_WITH_DM_TOOLS)
    if(EXISTS "/opt/homebrew/include/libusb-1.0")
        include_directories(/opt/homebrew/include/libusb-1.0)
    elseif(EXISTS "/usr/local/include/libusb-1.0")
        include_directories(/usr/local/include/libusb-1.0)
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/libusb-1.0")
        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/libusb-1.0)
    endif()
    # Boost paths for macOS
    if(EXISTS "/opt/homebrew/include")
        include_directories(/opt/homebrew/include)
    elseif(EXISTS "/usr/local/include")
        include_directories(/usr/local/include)
    endif()
endif()

# ZLG CAN FD Configuration
if(BUILD_WITH_ZLG_CANFD)
    # Check for ZLG CAN FD library
    find_library(ZLG_CANFD_LIBRARY
        NAMES controlcanfd
        PATHS
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/zlg_canfd
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/zlg_canfd/lib
            /usr/local/lib
            /usr/lib
        DOC "ZLG CAN FD communication library"
    )

    # Check for ZLG CAN FD header
    find_path(ZLG_CANFD_INCLUDE_DIR
        NAMES controlcanfd.h
        PATHS
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/zlg_canfd
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/zlg_canfd/include
            /usr/local/include
            /usr/include
        DOC "ZLG CAN FD header files"
    )

    if(ZLG_CANFD_LIBRARY AND ZLG_CANFD_INCLUDE_DIR)
        message(STATUS "✅ Found ZLG CAN FD library: ${ZLG_CANFD_LIBRARY}")
        message(STATUS "✅ Found ZLG CAN FD headers: ${ZLG_CANFD_INCLUDE_DIR}")
        add_definitions(-DHAVE_ZLG_CANFD)

        # Add the include directory if not already in global includes
        if(NOT ${ZLG_CANFD_INCLUDE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR}/lib/zlg_canfd)
            include_directories(${ZLG_CANFD_INCLUDE_DIR})
        endif()
    else()
        message(WARNING "❌ ZLG CAN FD library or headers not found. ZLG CAN FD backend will not be available.")
        message(STATUS "   - Expected library: libcontrolcanfd.so in lib/zlg_canfd/")
        message(STATUS "   - Expected header: controlcanfd.h in lib/zlg_canfd/")
        set(ZLG_CANFD_LIBRARY "")
        add_definitions(-DZLG_CANFD_NOT_AVAILABLE)
    endif()
else()
    message(STATUS "ZLG CAN FD backend disabled by BUILD_WITH_ZLG_CANFD=OFF")
    set(ZLG_CANFD_LIBRARY "")
    add_definitions(-DZLG_CANFD_NOT_AVAILABLE)
endif()

# ===================================
# CORE LIBRARY
# ===================================

# Build the core library first
add_subdirectory(src)

# ===================================
# OPTIONAL COMPONENTS
# ===================================

# Build tools
if(BUILD_TOOLS)
    add_subdirectory(tools)
endif()

# Build examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Build tests - disabled
# if(BUILD_TESTS)
#     enable_testing()
#     add_subdirectory(tests)
# endif()

# Build Python bindings
if(BUILD_PYTHON_BINDINGS)
    add_subdirectory(python)
endif()

# ===================================
# CONFIGURATION SUMMARY
# ===================================

message(STATUS "")
message(STATUS "=== IC_CAN Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Platform: ${IC_CAN_PLATFORM}")
message(STATUS "Threads found: ${CMAKE_USE_PTHREADS_INIT}")
message(STATUS "Build tools: ${BUILD_TOOLS}")
message(STATUS "Build examples: ${BUILD_EXAMPLES}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "Build Python bindings: ${BUILD_PYTHON_BINDINGS}")
message(STATUS "Build with Pinocchio: ${BUILD_WITH_PINOCCHIO}")
message(STATUS "Build with MATLAB: ${BUILD_WITH_MATLAB}")
message(STATUS "Build with dm-tools: ${BUILD_WITH_DM_TOOLS}")
message(STATUS "Build with ZLG CAN FD: ${BUILD_WITH_ZLG_CANFD}")
message(STATUS "libusb found: ${LIBUSB_LIBRARY}")
message(STATUS "ZLG CAN FD library: ${ZLG_CANFD_LIBRARY}")
if(BUILD_WITH_PINOCCHIO AND PINOCCHIO_FOUND)
    message(STATUS "Pinocchio version: ${PINOCCHIO_VERSION}")
endif()
message(STATUS "=============================")
message(STATUS "")

# ===================================
# PACKAGING
# ===================================

# Create pkg-config file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ic_can.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ic_can.pc"
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ic_can.pc
    DESTINATION lib/pkgconfig
)

# Create version file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/version.h"
    @ONLY
)

# Generate configuration header file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config.h"
    @ONLY
)