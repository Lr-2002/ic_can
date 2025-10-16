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

cmake_minimum_required(VERSION 3.12)
project(IC_CAN VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 平台检测
if(APPLE)
    set(IC_CAN_PLATFORM "macOS")
    # macOS 特定配置
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS architecture")
    else()
        set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "macOS architecture")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(IC_CAN_PLATFORM "Linux")
    # Linux 特定配置
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

# 编译选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")
    if(NOT WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    endif()
endif()

# 构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/dm-tools/C++/ubuntu/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/regressor/lib/regressorWithMotorDynamics
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/regressor/lib/frictionRegressor
)

# 添加 libusb 头文件路径 (macOS Homebrew)
if(APPLE)
    if(EXISTS "/opt/homebrew/include/libusb-1.0")
        include_directories(/opt/homebrew/include/libusb-1.0)
    elseif(EXISTS "/usr/local/include/libusb-1.0")
        include_directories(/usr/local/include/libusb-1.0)
    endif()
    # 添加项目内的符号链接路径
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/libusb-1.0")
        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/libusb-1.0)
    endif()
    # 添加boost包含路径
    if(EXISTS "/opt/homebrew/include")
        include_directories(/opt/homebrew/include)
    elseif(EXISTS "/usr/local/include")
        include_directories(/usr/local/include)
    endif()
endif()

# 查找依赖包
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# 查找 libusb (可选，用于 dm-tools SDK)
find_library(LIBUSB_LIBRARY
    NAMES usb-1.0 libusb-1.0
    PATHS /usr/lib /usr/local/lib /opt/homebrew/lib
    DOC "libusb library for dm-tools SDK"
)

# 如果找不到 libusb，给出警告但不阻止构建
if(NOT LIBUSB_LIBRARY)
    message(WARNING "libusb not found. dm-tools SDK integration will use simulation.")
    set(LIBUSB_LIBRARY "")
    add_definitions(-DSIMULATE_DM_TOOLS)
endif()

# 收集源文件 - 检查实际存在的文件
set(CORE_SOURCES)
set(MOTORS_SOURCES)
set(PROTOCOLS_SOURCES)
set(SAFETY_SOURCES)

# 检查核心源文件
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ic_can/core/ic_can.cpp")
    list(APPEND CORE_SOURCES "src/ic_can/core/ic_can.cpp")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ic_can/core/ic_can_real.cpp")
    list(APPEND CORE_SOURCES "src/ic_can/core/ic_can_real.cpp")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ic_can/core/arm_component.cpp")
    list(APPEND CORE_SOURCES "src/ic_can/core/arm_component.cpp")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ic_can/core/gripper_component.cpp")
    list(APPEND CORE_SOURCES "src/ic_can/core/gripper_component.cpp")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ic_can/core/torque_predictor.cpp")
    list(APPEND CORE_SOURCES "src/ic_can/core/torque_predictor.cpp")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ic_can/core/torque_predictor_pure_c.cpp")
    list(APPEND CORE_SOURCES "src/ic_can/core/torque_predictor_pure_c.cpp")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ic_can/core/torque_predictor_regressor.cpp")
    list(APPEND CORE_SOURCES "src/ic_can/core/torque_predictor_regressor.cpp")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ic_can/core/torque_predictor_config.cpp")
    list(APPEND CORE_SOURCES "src/ic_can/core/torque_predictor_config.cpp")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ic_can/core/torque_predictor_unified.cpp")
    list(APPEND CORE_SOURCES "src/ic_can/core/torque_predictor_unified.cpp")
endif()

# 检查电机源文件
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ic_can/motors/base_motor.cpp")
    list(APPEND MOTORS_SOURCES "src/ic_can/motors/base_motor.cpp")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ic_can/motors/dm_motor_real.cpp")
    list(APPEND MOTORS_SOURCES "src/ic_can/motors/dm_motor_real.cpp")
endif()

# 检查协议源文件
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ic_can/protocols/usb2can_real.cpp")
    list(APPEND PROTOCOLS_SOURCES "src/ic_can/protocols/usb2can_real.cpp")
endif()

# 检查安全源文件
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ic_can/safety/safety_module.cpp")
    list(APPEND SAFETY_SOURCES "src/ic_can/safety/safety_module.cpp")
endif()

set(IC_CAN_SOURCES
    ${CORE_SOURCES}
    ${MOTORS_SOURCES}
    ${PROTOCOLS_SOURCES}
    ${SAFETY_SOURCES}
)


# 创建核心库
add_library(ic_can_core SHARED ${IC_CAN_SOURCES})

# 设置库属性
set_target_properties(ic_can_core PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# 链接库
target_link_libraries(ic_can_core
    Threads::Threads
    ${LIBUSB_LIBRARY}
)

# 平台特定的链接库
if(APPLE)
    # macOS 特定库
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(IOKIT_FRAMEWORK IOKit)
    target_link_libraries(ic_can_core ${FOUNDATION_FRAMEWORK} ${IOKIT_FRAMEWORK})
elseif(WIN32)
    # Windows 特定库
    target_link_libraries(ic_can_core setupapi)
endif()

# 安装规则
install(TARGETS ic_can_core
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# 编译选项
target_compile_options(ic_can_core PRIVATE -Wno-unused-parameter)

# 添加 dm-tools SDK 头文件路径
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/dm-tools")
    target_include_directories(ic_can_core PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/dm-tools/C++/ubuntu/include
    )

    # 添加 dm-tools SDK 库路径
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/dm-tools/C++/ubuntu/lib/libu2canfd.a")
        target_link_libraries(ic_can_core
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/dm-tools/C++/ubuntu/lib/libu2canfd.a
        )
        message(STATUS "Found dm-tools SDK library: ${CMAKE_CURRENT_SOURCE_DIR}/lib/dm-tools/C++/ubuntu/lib/libu2canfd.a")
    elseif(APPLE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/dm-tools/C++/arm/lib/libu2canfd.a")
        target_link_libraries(ic_can_core
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/dm-tools/C++/arm/lib/libu2canfd.a
        )
        message(STATUS "Found dm-tools SDK library: ${CMAKE_CURRENT_SOURCE_DIR}/lib/dm-tools/C++/arm/lib/libu2canfd.a")
    endif()
endif()

# 测试选项
option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_PYTHON_BINDINGS "Build Python bindings" ON)

# 构建测试
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 构建示例
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 构建工具
option(BUILD_TOOLS "Build tools" ON)
if(BUILD_TOOLS)
    add_subdirectory(tools)
endif()

# 构建 Python 绑定
if(BUILD_PYTHON_BINDINGS)
    add_subdirectory(python)
endif()

# 添加自定义目标用于代码格式化
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT}
        -i
        ${IC_CAN_SOURCES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Formatting code with clang-format"
    )
endif()

# 打印配置信息
message(STATUS "=== IC_CAN Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Threads found: ${CMAKE_USE_PTHREADS_INIT}")
message(STATUS "libusb found: ${LIBUSB_LIBRARY}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "Build examples: ${BUILD_EXAMPLES}")
message(STATUS "Build tools: ${BUILD_TOOLS}")
message(STATUS "Build Python bindings: ${BUILD_PYTHON_BINDINGS}")
message(STATUS "=============================")

# 创建 pkg-config 文件
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ic_can.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ic_can.pc"
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ic_can.pc
    DESTINATION lib/pkgconfig
)

# 创建版本文件
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/version.h"
    @ONLY
)

# 生成配置头文件
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config.h"
    @ONLY
)

# 包含生成的头文件
target_include_directories(ic_can_core PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/include
)