# tools/CMakeLists.txt
# IC_CAN Tools Configuration - DEPENDS ON PRE-BUILT CORE LIBRARY

cmake_minimum_required(VERSION 3.14)

# ===================================
# DEPENDENCY CHECK
# ===================================

# Tools depend on the core library being already built
# The core library is built in src/ directory before this file is processed
if(NOT TARGET ic_can_core)
    message(FATAL_ERROR "❌ ic_can_core target not found. Core library must be built before tools.")
endif()

message(STATUS "✅ Found ic_can_core target - building tools")

# ===================================
# TOOL CONFIGURATION
# ===================================

# Check for optional dependencies
if(BUILD_WITH_DM_TOOLS)
    # Find dm-tools objects
    set(DM_TOOLS_OBJECTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../lib/dm-tools/C++/ubuntu/lib/usb_class.cpp.o")
        list(APPEND DM_TOOLS_OBJECTS "${CMAKE_CURRENT_SOURCE_DIR}/../lib/dm-tools/C++/ubuntu/lib/usb_class.cpp.o")
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../lib/dm-tools/C++/ubuntu/lib/crc.cpp.o")
        list(APPEND DM_TOOLS_OBJECTS "${CMAKE_CURRENT_SOURCE_DIR}/../lib/dm-tools/C++/ubuntu/lib/crc.cpp.o")
    endif()

    # Check for dm-tools static library
    set(DM_TOOLS_LIBRARY)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../lib/dm-tools/C++/ubuntu/lib/libu2canfd.a")
        set(DM_TOOLS_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/../lib/dm-tools/C++/ubuntu/lib/libu2canfd.a")
        message(STATUS "✅ Found dm-tools SDK library")
    endif()
else()
    message(STATUS "dm-tools SDK disabled - tools will use simulation mode")
    set(DM_TOOLS_OBJECTS)
    set(DM_TOOLS_LIBRARY)
endif()

# Check for MATLAB objects (if enabled)
set(MATLAB_OBJECTS)
if(BUILD_WITH_MATLAB)
    file(GLOB PURE_C_OBJECTS
        "${CMAKE_CURRENT_SOURCE_DIR}/../lib/matlab/lib/M_mtrx_fcn/*.o"
        "${CMAKE_CURRENT_SOURCE_DIR}/../lib/matlab/lib/C_mtrx_fcn/*.o"
        "${CMAKE_CURRENT_SOURCE_DIR}/../lib/matlab/lib/G_vctr_fcn/*.o"
    )

    file(GLOB REGRESSOR_OBJECTS
        "${CMAKE_CURRENT_SOURCE_DIR}/../lib/regressor/lib/frictionRegressor/*.o"
        "${CMAKE_CURRENT_SOURCE_DIR}/../lib/regressor/lib/regressorWithMotorDynamics/*.o"
    )

    if(PURE_C_OBJECTS)
        message(STATUS "✅ Found MATLAB pure C objects for tools")
        list(APPEND MATLAB_OBJECTS ${PURE_C_OBJECTS})
    endif()

    if(REGRESSOR_OBJECTS)
        message(STATUS "✅ Found MATLAB regressor objects for tools")
        list(APPEND MATLAB_OBJECTS ${REGRESSOR_OBJECTS})
    endif()
endif()

# Common libraries for all tools
set(COMMON_LIBS
    ic_can_core  # Link against the core library target
    Threads::Threads
)

# Add dm-tools if available
if(DM_TOOLS_LIBRARY)
    list(APPEND COMMON_LIBS ${DM_TOOLS_LIBRARY})
endif()

# Add Pinocchio if available
if(BUILD_WITH_PINOCCHIO AND PINOCCHIO_FOUND)
    list(APPEND COMMON_LIBS ${PINOCCHIO_LIBRARIES})
endif()

# ===================================
# TOOL BUILD FUNCTION
# ===================================

function(add_ic_can_tool name)
    # Check if source file exists
    set(source_file "${CMAKE_CURRENT_SOURCE_DIR}/../src/tools/${name}.cpp")
    if(NOT EXISTS "${source_file}")
        message(WARNING "Tool source file not found: ${source_file}")
        return()
    endif()

    # Create executable
    add_executable(${name} "${source_file}")

    # Link against common libraries
    target_link_libraries(${name} ${COMMON_LIBS})

    # Add dm-tools objects if available
    if(DM_TOOLS_OBJECTS)
        target_sources(${name} PRIVATE ${DM_TOOLS_OBJECTS})
    endif()

    # Add MATLAB objects if available
    if(MATLAB_OBJECTS)
        target_sources(${name} PRIVATE ${MATLAB_OBJECTS})
    endif()

    # Include directories
    target_include_directories(${name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../lib/dm-tools/C++/ubuntu/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../lib/regressor/lib/regressorWithMotorDynamics
        ${CMAKE_CURRENT_SOURCE_DIR}/../lib/regressor/lib/frictionRegressor
    )

    # Add Pinocchio include directories if available
    if(BUILD_WITH_PINOCCHIO AND PINOCCHIO_FOUND)
        target_include_directories(${name} PRIVATE ${PINOCCHIO_INCLUDE_DIRS})
        target_link_directories(${name} PRIVATE /opt/openrobots/lib)
    endif()

    # Compiler options
    target_compile_options(${name} PRIVATE -Wall -Wextra)

    # Installation
    install(TARGETS ${name} RUNTIME DESTINATION bin)

    message(STATUS "✅ Added tool: ${name}")
endfunction()

# ===================================
# BASIC TOOLS
# ===================================

set(BASIC_TOOLS
    set_motor_zero
    arm_position_monitor
    arm_position_monitor_simple
    wrist_motor_monitor
    wrist_motor_only_monitor
    home_to_zero
    simple_motor_test
    servo_motor_test
    gripper_component_test
    test_control_modes
    test_sync_can
    test_unified_control
    demo_unified_control_features
)

foreach(tool ${BASIC_TOOLS})
    add_ic_can_tool(${tool})
endforeach()

# ===================================
# LOGGING TOOLS
# ===================================

set(LOGGING_TOOLS
    logging_demo
    logging_demo_simple
)

foreach(tool ${LOGGING_TOOLS})
    add_ic_can_tool(${tool})
endforeach()

# ===================================
# TORQUE TOOLS (Conditional)
# ===================================

# Only build torque tools if we have torque predictors
set(TORQUE_TOOLS
    simple_arm_torque_demo  # Requires TorquePredictorPureC - may fail if MATLAB disabled
    demo_real_arm_torques
    test_torque_simple
    test_torque_integration
    test_torque_predictor_speed
    test_torque_with_logging
    test_regressor
)

foreach(tool ${TORQUE_TOOLS})
    if(BUILD_WITH_MATLAB OR tool STREQUAL "demo_real_arm_torques" OR tool STREQUAL "test_torque_simple" OR tool STREQUAL "test_torque_integration")
        add_ic_can_tool(${tool})
    endif()
endforeach()

# ===================================
# ADVANCED TOOLS
# ===================================

set(ADVANCED_TOOLS
    trajectory_executor
    friction_sweep
    friction_compensated_controller
    dynamic_identifier
    regressor_wrapper
)

foreach(tool ${ADVANCED_TOOLS})
    add_ic_can_tool(${tool})
endforeach()

# ===================================
# MOTOR-SPECIFIC TOOLS
# ===================================

set(MOTOR_TOOLS
    wrist_kinematics_test
    wrist_safe_monitor
    ht_frame_test
    wrist_motor_monitor_clean
)

foreach(tool ${MOTOR_TOOLS})
    add_ic_can_tool(${tool})
endforeach()

# ===================================
# MONITORING TEST TOOLS
# ===================================

set(TEST_TOOLS
    test_wrist_monitor
    test_wrist_monitor_simple
    test_wrist_minimal
    test_wrist_standalone
)

foreach(tool ${TEST_TOOLS})
    add_ic_can_tool(${tool})
endforeach()

# ===================================
# CONFIGURATION SUMMARY
# ===================================

message(STATUS "")
message(STATUS "=== IC_CAN Tools Configuration ===")
message(STATUS "Build with dm-tools: ${BUILD_WITH_DM_TOOLS}")
message(STATUS "Build with MATLAB: ${BUILD_WITH_MATLAB}")
message(STATUS "Build with Pinocchio: ${BUILD_WITH_PINOCCHIO}")
list(LENGTH DM_TOOLS_OBJECTS NUM_DM_OBJECTS)
list(LENGTH MATLAB_OBJECTS NUM_MATLAB_OBJECTS)
message(STATUS "dm-tools objects found: ${NUM_DM_OBJECTS}")
message(STATUS "MATLAB objects found: ${NUM_MATLAB_OBJECTS}")
message(STATUS "=====================================")
message(STATUS "")