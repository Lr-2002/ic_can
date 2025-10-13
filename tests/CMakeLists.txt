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

# 查找测试框架
find_package(GTest REQUIRED)

# 启用测试
enable_testing()

# 基础功能测试
add_executable(test_basic test_basic.cpp)
target_link_libraries(test_basic ic_can_core GTest::GTest GTest::Main)

# 电机控制测试
add_executable(test_motors test_motors.cpp)
target_link_libraries(test_motors ic_can_core GTest::GTest GTest::Main)

# 安全模块测试
add_executable(test_safety test_safety.cpp)
target_link_libraries(test_safety ic_can_core GTest::GTest GTest::Main)

# 集成测试
add_executable(test_integration test_integration.cpp)
target_link_libraries(test_integration ic_can_core GTest::GTest GTest::Main)

# 性能测试
add_executable(test_performance test_performance.cpp)
target_link_libraries(test_performance ic_can_core GTest::GTest GTest::Main)

# 通信测试
add_executable(test_communication test_communication.cpp)
target_link_libraries(test_communication ic_can_core GTest::GTest GTest::Main)

# 设置测试属性
set_target_properties(test_basic PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test_results
)

set_target_properties(test_motors PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test_results
)

set_target_properties(test_safety PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test_results
)

set_target_properties(test_integration PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test_results
)

set_target_properties(test_performance PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test_results
)

set_target_properties(test_communication PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test_results
)

# 注册测试
add_test(NAME BasicTests COMMAND test_basic)
add_test(NAME MotorTests COMMAND test_motors)
add_test(NAME SafetyTests COMMAND test_safety)
add_test(NAME IntegrationTests COMMAND test_integration)
add_test(NAME PerformanceTests COMMAND test_performance)
add_test(NAME CommunicationTests COMMAND test_communication)

# 测试覆盖率 (可选)
option(ENABLE_COVERAGE "Enable test coverage" OFF)
if(ENABLE_COVERAGE)
    find_program(GCOV gcov)
    if(GCOV)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")

        # 为每个测试目标添加覆盖率支持
        target_link_libraries(test_basic gcov)
        target_link_libraries(test_motors gcov)
        target_link_libraries(test_safety gcov)
        target_link_libraries(test_integration gcov)
        target_link_libraries(test_performance gcov)
        target_link_libraries(test_communication gcov)

        message(STATUS "Test coverage enabled")
    endif()
endif()

# 内存检查 (可选)
option(ENABLE_SANITIZER "Enable address sanitizer" OFF)
if(ENABLE_SANITIZER)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    message(STATUS "Address sanitizer enabled")
endif()