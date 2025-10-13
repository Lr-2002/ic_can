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

# 设置示例输出目录
set_target_properties(basic_control PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)

# 安装示例
install(TARGETS basic_control RUNTIME DESTINATION bin/examples)