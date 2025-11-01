# src/CMakeLists.txt
# Core library build configuration - SINGLE SOURCE OF TRUTH

cmake_minimum_required(VERSION 3.14)

# ===================================
# CORE LIBRARY SOURCES
# ===================================

# Core component sources
set(CORE_SOURCES)
set(MOTORS_SOURCES)
set(PROTOCOLS_SOURCES)
set(TORQUE_PREDICTORS_SOURCES)

# Core components
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ic_can/core/ic_can.cpp")
    list(APPEND CORE_SOURCES "ic_can/core/ic_can.cpp")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ic_can/core/arm_component.cpp")
    list(APPEND CORE_SOURCES "ic_can/core/arm_component.cpp")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ic_can/core/gripper_component.cpp")
    list(APPEND CORE_SOURCES "ic_can/core/gripper_component.cpp")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ic_can/core/wrist_component.cpp")
    list(APPEND CORE_SOURCES "ic_can/core/wrist_component.cpp")
endif()

# Motor sources
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ic_can/motors/base_motor.cpp")
    list(APPEND MOTORS_SOURCES "ic_can/motors/base_motor.cpp")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ic_can/motors/dm_motor_real.cpp")
    list(APPEND MOTORS_SOURCES "ic_can/motors/dm_motor_real.cpp")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ic_can/motors/ht_motor.cpp")
    list(APPEND MOTORS_SOURCES "ic_can/motors/ht_motor.cpp")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ic_can/motors/servo_motor.cpp")
    list(APPEND MOTORS_SOURCES "ic_can/motors/servo_motor.cpp")
    message(STATUS "✅ Added servo motor source")
endif()

# Protocol sources - temporarily disabled due to header mismatch
# if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ic_can/protocols/usb2can_real.cpp")
#     list(APPEND PROTOCOLS_SOURCES "ic_can/protocols/usb2can_real.cpp")
# endif()

# Torque predictor sources (conditional)
if(BUILD_WITH_PINOCCHIO AND PINOCCHIO_FOUND)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ic_can/core/torque_predictor_pinocchio.cpp")
        list(APPEND TORQUE_PREDICTORS_SOURCES "ic_can/core/torque_predictor_pinocchio.cpp")
        message(STATUS "✅ Added Pinocchio torque predictor")
    endif()
endif()

if(BUILD_WITH_MATLAB)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ic_can/core/torque_predictor_pure_c.cpp")
        list(APPEND TORQUE_PREDICTORS_SOURCES "ic_can/core/torque_predictor_pure_c.cpp")
        message(STATUS "✅ Added Pure C MATLAB torque predictor")
    endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ic_can/core/torque_predictor_unified.cpp")
    list(APPEND TORQUE_PREDICTORS_SOURCES "ic_can/core/torque_predictor_unified.cpp")
    message(STATUS "✅ Added unified torque predictor")
endif()

# ===================================
# MATLAB/OBJECT FILES (Optional)
# ===================================

# Check for MATLAB regressor libraries
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
        message(STATUS "✅ Found MATLAB pure C objects")
        list(APPEND MATLAB_OBJECTS ${PURE_C_OBJECTS})
        add_definitions(-DHAVE_REGRESSOR_LIBS)
    endif()

    if(REGRESSOR_OBJECTS)
        message(STATUS "✅ Found MATLAB regressor objects")
        list(APPEND MATLAB_OBJECTS ${REGRESSOR_OBJECTS})
    endif()
else()
    message(STATUS "MATLAB support disabled")
endif()

# ===================================
# COMBINE ALL SOURCES
# ===================================

set(IC_CAN_CORE_SOURCES
    ${CORE_SOURCES}
    ${MOTORS_SOURCES}
    ${PROTOCOLS_SOURCES}
    ${TORQUE_PREDICTORS_SOURCES}
    ${MATLAB_OBJECTS}
)

# ===================================
# CREATE CORE LIBRARY
# ===================================

if(IC_CAN_CORE_SOURCES)
    list(LENGTH IC_CAN_CORE_SOURCES NUM_SOURCES)
    message(STATUS "Creating ic_can_core library with ${NUM_SOURCES} source files")

    add_library(ic_can_core SHARED ${IC_CAN_CORE_SOURCES})

    # Set library properties
    set_target_properties(ic_can_core PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        PUBLIC_HEADER "ic_can/core/ic_can.hpp"
    )

    # Include directories
    target_include_directories(ic_can_core PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
    )

    target_include_directories(ic_can_core PRIVATE
        ${CMAKE_BINARY_DIR}/include
    )

    # Link libraries
    target_link_libraries(ic_can_core
        Threads::Threads
        ${LIBUSB_LIBRARY}
    )

    # Add Pinocchio if available
    if(BUILD_WITH_PINOCCHIO AND PINOCCHIO_FOUND)
        target_include_directories(ic_can_core PRIVATE ${PINOCCHIO_INCLUDE_DIRS})
        target_compile_options(ic_can_core PRIVATE ${PINOCCHIO_CFLAGS_OTHER})
        target_link_directories(ic_can_core PRIVATE /opt/openrobots/lib)
        target_link_libraries(ic_can_core ${PINOCCHIO_LIBRARIES})

        # Set RPATH for runtime library discovery
        set_target_properties(ic_can_core PROPERTIES
            INSTALL_RPATH "/opt/openrobots/lib"
            BUILD_WITH_INSTALL_RPATH TRUE
        )
    endif()

    # Add dm-tools SDK if available
    if(BUILD_WITH_DM_TOOLS)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../lib/dm-tools/C++/ubuntu/lib/libu2canfd.a")
            target_link_libraries(ic_can_core
                ${CMAKE_CURRENT_SOURCE_DIR}/../lib/dm-tools/C++/ubuntu/lib/libu2canfd.a
            )
            message(STATUS "✅ Added dm-tools SDK library")
        endif()
    endif()

    # Platform-specific libraries
    if(APPLE)
        find_library(FOUNDATION_FRAMEWORK Foundation)
        find_library(IOKIT_FRAMEWORK IOKit)
        target_link_libraries(ic_can_core ${FOUNDATION_FRAMEWORK} ${IOKIT_FRAMEWORK})
    elseif(WIN32)
        target_link_libraries(ic_can_core setupapi)
    endif()

    # Compiler options
    target_compile_options(ic_can_core PRIVATE -Wno-unused-parameter)

    # Installation rules
    install(TARGETS ic_can_core
        EXPORT IC_CANCoreTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include/ic_can/core
    )

    # Export the library
    install(EXPORT IC_CANCoreTargets
        FILE IC_CANCoreTargets.cmake
        NAMESPACE IC_CAN::
        DESTINATION lib/cmake/IC_CAN
    )

else()
    message(WARNING "❌ No source files found for ic_can_core library!")
endif()

# ===================================
# INSTALL ALL HEADERS
# ===================================

install(DIRECTORY ../include/ic_can
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)