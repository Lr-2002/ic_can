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

# 基础控制示例
add_executable(basic_control basic_control.cpp)
target_link_libraries(basic_control ic_can_core)
if(BUILD_WITH_PINOCCHIO)
    target_link_directories(basic_control PRIVATE /opt/openrobots/lib)
    set_target_properties(basic_control PROPERTIES LINK_FLAGS "-Wl,-rpath,/opt/openrobots/lib")
endif()

# 单电机测试 - dm-tools SDK连接测试
add_executable(single_motor_test single_motor_test.cpp)
target_link_libraries(single_motor_test ic_can_core)
if(BUILD_WITH_PINOCCHIO)
    target_link_directories(single_motor_test PRIVATE /opt/openrobots/lib)
    set_target_properties(single_motor_test PROPERTIES LINK_FLAGS "-Wl,-rpath,/opt/openrobots/lib")
endif()

# dm-tools SDK直接连接测试 (不依赖IC_CAN框架) - Temporarily disabled due to C++ standard library compatibility issues
# add_executable(dm_tools_connection_test dm_tools_connection_test.cpp)
# target_include_directories(dm_tools_connection_test PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/../lib/dm-tools/C++/ubuntu/include
# )
# target_link_libraries(dm_tools_connection_test
#     ${CMAKE_CURRENT_SOURCE_DIR}/../lib/dm-tools/C++/ubuntu/lib/libu2canfd.a
#     Threads::Threads
#     ${LIBUSB_LIBRARY}
# )
# target_compile_options(dm_tools_connection_test PRIVATE -Wno-error)
# set_property(TARGET dm_tools_connection_test PROPERTY CXX_STANDARD 14)

# 简单DM4340测试
add_executable(simple_dm4340_test simple_dm4340_test.cpp)
target_link_libraries(simple_dm4340_test ic_can_core)
if(BUILD_WITH_PINOCCHIO)
    target_link_directories(simple_dm4340_test PRIVATE /opt/openrobots/lib)
    set_target_properties(simple_dm4340_test PROPERTIES LINK_FLAGS "-Wl,-rpath,/opt/openrobots/lib")
endif()

# DM4340详细测试
add_executable(test_dm4340 test_dm4340.cpp)
target_link_libraries(test_dm4340 ic_can_core)
if(BUILD_WITH_PINOCCHIO)
    target_link_directories(test_dm4340 PRIVATE /opt/openrobots/lib)
    set_target_properties(test_dm4340 PROPERTIES LINK_FLAGS "-Wl,-rpath,/opt/openrobots/lib")
endif()

# CAN ID 1测试 - removed

# 手腕和夹爪分离演示 - removed
# add_executable(wrist_gripper_demo wrist_gripper_demo.cpp)
# target_link_libraries(wrist_gripper_demo ic_can_core)
# if(BUILD_WITH_PINOCCHIO)
#     target_link_directories(wrist_gripper_demo PRIVATE /opt/openrobots/lib)
#     set_target_properties(wrist_gripper_demo PROPERTIES LINK_FLAGS "-Wl,-rpath,/opt/openrobots/lib")
# endif()

# 手腕和夹爪分离简单演示（不依赖完整框架）
add_executable(wrist_gripper_simple_demo wrist_gripper_simple_demo.cpp)

# 测试手腕和夹爪组件构建
add_executable(test_wrist_gripper_build test_wrist_gripper_build.cpp)
target_link_libraries(test_wrist_gripper_build ic_can_core)
if(BUILD_WITH_PINOCCHIO)
    target_link_directories(test_wrist_gripper_build PRIVATE /opt/openrobots/lib)
    set_target_properties(test_wrist_gripper_build PROPERTIES LINK_FLAGS "-Wl,-rpath,/opt/openrobots/lib")
endif()

# 设置示例输出目录
set_target_properties(basic_control PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)
set_target_properties(single_motor_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)
set_target_properties(simple_dm4340_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)
set_target_properties(test_dm4340 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)
# can_id1_test removed
# wrist_gripper_demo removed
set_target_properties(wrist_gripper_simple_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)
set_target_properties(test_wrist_gripper_build PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)

# 安装示例
install(TARGETS basic_control single_motor_test simple_dm4340_test test_dm4340 wrist_gripper_simple_demo test_wrist_gripper_build
    RUNTIME DESTINATION bin/examples)