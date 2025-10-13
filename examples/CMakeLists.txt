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

# 单电机测试 - dm-tools SDK连接测试
add_executable(single_motor_test single_motor_test.cpp)
target_link_libraries(single_motor_test ic_can_core)

# dm-tools SDK直接连接测试 (不依赖IC_CAN框架)
add_executable(dm_tools_connection_test dm_tools_connection_test.cpp)
target_include_directories(dm_tools_connection_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../dm-tools/USB2FDCAN/SDK/C++/arm/include
)
target_link_libraries(dm_tools_connection_test
    ${CMAKE_CURRENT_SOURCE_DIR}/../dm-tools/USB2FDCAN/SDK/C++/arm/lib/libu2canfd.a
    Threads::Threads
    ${LIBUSB_LIBRARY}
)

# 简单DM4340测试
add_executable(simple_dm4340_test simple_dm4340_test.cpp)
target_link_libraries(simple_dm4340_test ic_can_core)

# DM4340详细测试
add_executable(test_dm4340 test_dm4340.cpp)
target_link_libraries(test_dm4340 ic_can_core)

# CAN ID 1测试
add_executable(can_id1_test can_id1_test.cpp)
target_link_libraries(can_id1_test ic_can_core)

# 设置示例输出目录
set_target_properties(basic_control PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)
set_target_properties(single_motor_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)
set_target_properties(simple_dm4340_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)
set_target_properties(test_dm4340 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)
set_target_properties(can_id1_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)

# 安装示例
install(TARGETS basic_control single_motor_test simple_dm4340_test test_dm4340 can_id1_test
    RUNTIME DESTINATION bin/examples)