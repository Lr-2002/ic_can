# CMakeLists.txt for IC_CAN Tools
# Copyright 2025 IC_CAN Team

cmake_minimum_required(VERSION 3.16)
project(IC_CAN_Tools)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Set PKG_CONFIG_PATH for Pinocchio
set(PKG_CONFIG_PATH $ENV{PKG_CONFIG_PATH} /opt/openrobots/lib/pkgconfig)

# Find Pinocchio
set(CMAKE_PREFIX_PATH "/opt/openrobots/lib/pkgconfig/../../" ${CMAKE_PREFIX_PATH})
pkg_check_modules(PINOCCHIO REQUIRED pinocchio)

if(PINOCCHIO_FOUND)
    message(STATUS "✅ Found Pinocchio for tools: ${PINOCCHIO_VERSION}")
    set(PINOCCHIO_INCLUDE_DIRS "/opt/openrobots/include" ${PINOCCHIO_INCLUDE_DIRS})
else()
    message(WARNING "❌ Pinocchio not found for tools. Pinocchio-based torque predictor will not be available.")
    set(PINOCCHIO_INCLUDE_DIRS "")
    set(PINOCCHIO_LIBRARIES "")
    add_definitions(-DPINOCCHIO_NOT_AVAILABLE)
endif()

# Try to find libusb
pkg_check_modules(USB1 libusb-1.0)
if(NOT USB1_FOUND)
    find_library(LIBUSB_LIBRARY NAMES usb-1.0 libusb-1.0)
    if(LIBUSB_LIBRARY)
        set(USB1_LIBRARIES ${LIBUSB_LIBRARY})
        set(USB1_FOUND TRUE)
    endif()
endif()

# Set include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/dm-tools/C++/ubuntu/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/regressor
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/regressor/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/regressor/lib/regressorWithMotorDynamics
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/regressor/lib/frictionRegressor
)

# Common source files
set(IC_CAN_SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ic_can/core/ic_can.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ic_can/core/torque_predictor_pure_c.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ic_can/core/torque_predictor_regressor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ic_can/core/torque_predictor_config.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ic_can/core/torque_predictor_pinocchio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ic_can/core/torque_predictor_unified.cpp
)

# Check for DM tools objects
set(DM_TOOLS_OBJECTS)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../lib/dm-tools/C++/ubuntu/lib/usb_class.cpp.o")
    list(APPEND DM_TOOLS_OBJECTS "${CMAKE_CURRENT_SOURCE_DIR}/../lib/dm-tools/C++/ubuntu/lib/usb_class.cpp.o")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../lib/dm-tools/C++/ubuntu/lib/crc.cpp.o")
    list(APPEND DM_TOOLS_OBJECTS "${CMAKE_CURRENT_SOURCE_DIR}/../lib/dm-tools/C++/ubuntu/lib/crc.cpp.o")
endif()

# Common libraries
set(COMMON_LIBS
    ${USB1_LIBRARIES}
    Threads::Threads
    ${PINOCCHIO_LIBRARIES}
)

# Add dm-tools library if available
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../lib/dm-tools/C++/ubuntu/lib/libu2canfd.a")
    list(APPEND COMMON_LIBS "${CMAKE_CURRENT_SOURCE_DIR}/../lib/dm-tools/C++/ubuntu/lib/libu2canfd.a")
endif()

# Compiler flags
add_compile_options(-Wall -Wextra)

# Check for MATLAB regressor libraries
file(GLOB PURE_C_OBJECTS
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/matlab/lib/M_mtrx_fcn/*.o"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/matlab/lib/C_mtrx_fcn/*.o"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/matlab/lib/G_vctr_fcn/*.o"
)

# Check for regressor MATLAB libraries
file(GLOB REGRESSOR_OBJECTS
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/regressor/lib/frictionRegressor/*.o"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/regressor/lib/regressorWithMotorDynamics/*.o"
)

if(PURE_C_OBJECTS)
    message(STATUS "Found pure C MATLAB objects")
    add_definitions(-DHAVE_REGRESSOR_LIBS)
else()
    message(STATUS "Pure C MATLAB objects not found")
endif()

# Function to add a tool with common settings (all tools now in src/tools/)
function(add_ic_can_tool_from_src name)
    add_executable(${name}
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/${name}.cpp"
        ${IC_CAN_SOURCE}
    )
    target_link_libraries(${name} ${DM_TOOLS_OBJECTS} ${COMMON_LIBS})

    # Add explicit library directory for Pinocchio
    if(PINOCCHIO_FOUND)
        target_link_directories(${name} PRIVATE /opt/openrobots/lib)
    endif()
    target_include_directories(${name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../lib/dm-tools/C++/ubuntu/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/regressor
        ${CMAKE_CURRENT_SOURCE_DIR}/../lib/regressor/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/../lib/regressor/lib/regressorWithMotorDynamics
        ${CMAKE_CURRENT_SOURCE_DIR}/../lib/regressor/lib/frictionRegressor
        ${PINOCCHIO_INCLUDE_DIRS}
    )
    target_compile_options(${name} PRIVATE -Wall -Wextra)

    # Add regressor objects if available
    if(PURE_C_OBJECTS)
        target_sources(${name} PRIVATE ${PURE_C_OBJECTS})
    endif()
    if(REGRESSOR_OBJECTS)
        target_sources(${name} PRIVATE ${REGRESSOR_OBJECTS})
    endif()

    list(APPEND TOOL_LIST ${name})
    set(TOOL_LIST ${TOOL_LIST} PARENT_SCOPE)
endfunction()

# Build all tools that exist
set(TOOL_LIST)

# Basic tools
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/set_motor_zero.cpp")
    add_ic_can_tool_from_src(set_motor_zero)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/arm_position_monitor.cpp")
    add_ic_can_tool_from_src(arm_position_monitor)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/home_to_zero.cpp")
    add_ic_can_tool_from_src(home_to_zero)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/simple_motor_test.cpp")
    add_ic_can_tool_from_src(simple_motor_test)
endif()

# Logging tools
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/logging_demo.cpp")
    add_ic_can_tool_from_src(logging_demo)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/logging_demo_simple.cpp")
    add_ic_can_tool_from_src(logging_demo_simple)
endif()

# Torque tools
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/torque_predictor.cpp")
    add_ic_can_tool_from_src(torque_predictor)
endif()


if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/simple_arm_torque_demo.cpp")
    add_ic_can_tool_from_src(simple_arm_torque_demo)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/demo_real_arm_torques.cpp")
    add_ic_can_tool_from_src(demo_real_arm_torques)
endif()

# Test tools
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/test_regressor.cpp")
    add_ic_can_tool_from_src(test_regressor)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/test_torque_simple.cpp")
    add_ic_can_tool_from_src(test_torque_simple)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/test_torque_integration.cpp")
    add_ic_can_tool_from_src(test_torque_integration)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/test_torque_predictor_speed.cpp")
    add_ic_can_tool_from_src(test_torque_predictor_speed)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/test_torque_with_logging.cpp")
    add_ic_can_tool_from_src(test_torque_with_logging)
endif()

# Advanced tools
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/trajectory_executor.cpp")
    add_ic_can_tool_from_src(trajectory_executor)
endif()

# Friction identification tools
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/friction_sweep.cpp")
    add_ic_can_tool_from_src(friction_sweep)
endif()

# Friction control tools
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/friction_compensated_controller.cpp")
    add_ic_can_tool_from_src(friction_compensated_controller)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/dynamic_identifier.cpp")
    add_ic_can_tool_from_src(dynamic_identifier)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/regressor_wrapper.cpp")
    add_ic_can_tool_from_src(regressor_wrapper)
endif()

# Custom target to build all tools
if(TOOL_LIST)
    add_custom_target(tools DEPENDS ${TOOL_LIST} COMMENT "Building all IC_CAN tools")

    # Installation
    install(TARGETS ${TOOL_LIST} RUNTIME DESTINATION bin)

    message(STATUS "IC_CAN Tools to build: ${TOOL_LIST}")
else()
    message(WARNING "No tool source files found!")
endif()

# Print build information
message(STATUS "IC_CAN Tools Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Include Directories: ${CMAKE_CURRENT_SOURCE_DIR}/../include")
message(STATUS "  DM-Tools SDK: ${CMAKE_CURRENT_SOURCE_DIR}/../lib/dm-tools/C++/ubuntu/include")
message(STATUS "  Libraries: ${COMMON_LIBS}")
message(STATUS "  libusb found: ${USB1_FOUND}")
message(STATUS "  MATLAB regressor libs: ${PURE_C_OBJECTS}")
message(STATUS "  Regressor objects: ${REGRESSOR_OBJECTS}")