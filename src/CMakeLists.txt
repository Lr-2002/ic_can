# src/CMakeLists.txt
# Core library build configuration

# Collect all source files
file(GLOB_RECURSE IC_CAN_SOURCES
    "ic_can/core/*.cpp"
    "ic_can/core/*.c"
    "ic_can/motors/*.cpp"
    "ic_can/motors/*.c"
    "ic_can/protocols/*.cpp"
    "ic_can/protocols/*.c"
    "ic_can/safety/*.cpp"
    "ic_can/safety/*.c"
)

# Manual source collection to exclude simulation files
set(CORE_SOURCES
    "ic_can/core/arm_component.cpp"
    "ic_can/core/gripper_component.cpp"
    "ic_can/core/ic_can.cpp"
    "ic_can/core/torque_predictor.cpp"
)

set(MOTORS_SOURCES
    "ic_can/motors/base_motor.cpp"
    "ic_can/motors/dm_motor_real.cpp"
)

set(PROTOCOLS_SOURCES
    "ic_can/protocols/usb2can_real.cpp"
)

set(SAFETY_SOURCES
    "ic_can/safety/safety_module.cpp"
)

set(ALL_IC_CAN_SOURCES
    ${CORE_SOURCES}
    ${MOTORS_SOURCES}
    ${PROTOCOLS_SOURCES}
    ${SAFETY_SOURCES}
)

# Add pure C MATLAB function objects if they exist
file(GLOB PURE_C_OBJECTS
    "../tools/regressor/lib/M_mtrx_fcn/*.o"
    "../tools/regressor/lib/C_mtrx_fcn/*.o"
    "../tools/regressor/lib/G_vctr_fcn/*.o"
)

# Check if regressor objects exist and set appropriate flag
if(PURE_C_OBJECTS)
    message(STATUS "Found pure C MATLAB objects")
    add_definitions(-DHAVE_REGRESSOR_LIBS)
else()
    message(STATUS "Pure C MATLAB objects not found, using stub functions")
    set(PURE_C_OBJECTS)
endif()

# Create the core library
add_library(ic_can_core SHARED ${ALL_IC_CAN_SOURCES} ${PURE_C_OBJECTS})

# Set library properties
set_target_properties(ic_can_core PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "../include/ic_can/core/ic_can.hpp"
)

# Include directories
target_include_directories(ic_can_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(ic_can_core PRIVATE
    ${CMAKE_BINARY_DIR}/include
)

# Add dm-tools SDK if available
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../dm-tools")
    target_include_directories(ic_can_core PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../dm-tools/USB2FDCAN/SDK/C++/ubuntu/include
    )

    # Add dm-tools SDK library
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../dm-tools/USB2FDCAN/SDK/C++/ubuntu/lib/libu2canfd.a")
        target_link_libraries(ic_can_core
            ${CMAKE_CURRENT_SOURCE_DIR}/../dm-tools/USB2FDCAN/SDK/C++/ubuntu/lib/libu2canfd.a
        )
        message(STATUS "Found dm-tools SDK library")
    elseif(APPLE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../dm-tools/USB2FDCAN/SDK/C++/arm/lib/libu2canfd.a")
        target_link_libraries(ic_can_core
            ${CMAKE_CURRENT_SOURCE_DIR}/../dm-tools/USB2FDCAN/SDK/C++/arm/lib/libu2canfd.a
        )
        message(STATUS "Found dm-tools SDK library (ARM)")
    endif()
endif()

# Link libraries
target_link_libraries(ic_can_core
    Threads::Threads
    ${LIBUSB_LIBRARY}
)

# Platform-specific linking
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

# Install all header files
install(DIRECTORY ../include/ic_can
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

# Export the library
install(EXPORT IC_CANCoreTargets
    FILE IC_CANCoreTargets.cmake
    NAMESPACE IC_CAN::
    DESTINATION lib/cmake/IC_CAN
)