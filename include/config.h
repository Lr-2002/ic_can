// Copyright 2025 IC_CAN Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

// 构建配置
#define IC_CAN_BUILD_TYPE "Release"
#define IC_CAN_INSTALL_PREFIX "/usr/local"

// 功能开关
#define BUILD_TESTS
#define BUILD_EXAMPLES
#define BUILD_PYTHON_BINDINGS

// 调试选项
/* #undef DEBUG */

// 仿真选项
/* #undef SIMULATE_DM_TOOLS */

// 平台检测
#if defined(_WIN32) || defined(_WIN64)
#define IC_CAN_PLATFORM_WINDOWS
#elif defined(__linux__)
#define IC_CAN_PLATFORM_LINUX
#elif defined(__APPLE__)
#define IC_CAN_PLATFORM_MACOS
#endif

// 编译器特性
#if defined(__GNUC__)
#define IC_CAN_COMPILER_GCC
#define IC_CAN_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#elif defined(__clang__)
#define IC_CAN_COMPILER_CLANG
#define IC_CAN_COMPILER_VERSION (__clang__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif defined(_MSC_VER)
#define IC_CAN_COMPILER_MSVC
#define IC_CAN_COMPILER_VERSION _MSC_VER
#endif

// 默认配置
#ifndef IC_CAN_DEFAULT_CONTROL_FREQUENCY
#define IC_CAN_DEFAULT_CONTROL_FREQUENCY 500.0
#endif

#ifndef IC_CAN_DEFAULT_TIMEOUT_MS
#define IC_CAN_DEFAULT_TIMEOUT_MS 5000
#endif

#ifndef IC_CAN_MAX_MOTORS
#define IC_CAN_MAX_MOTORS 9
#endif

#ifndef IC_CAN_ARM_MOTORS
#define IC_CAN_ARM_MOTORS 6
#endif

#ifndef IC_CAN_GRIPPER_MOTORS
#define IC_CAN_GRIPPER_MOTORS 3
#endif
