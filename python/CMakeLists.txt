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

# 查找 pybind11
find_package(pybind11 REQUIRED)

# Python 包信息
set(PYTHON_MODULE_NAME ic_can_python)
set(PYTHON_MODULE_VERSION ${PROJECT_VERSION})

# 生成绑定代码
pybind11_add_module(${PYTHON_MODULE_NAME} MODULE
    src/ic_can_python.cpp
    EXCLUDE_FROM_ALL
)

# 链接库
target_link_libraries(${PYTHON_MODULE_NAME} PRIVATE ic_can_core)

# 设置模块属性
set_target_properties(${PYTHON_MODULE_NAME} PROPERTIES
    SUFFIX ${PYTHON_MODULE_EXTENSION}
)

# 包含目录
target_include_directories(${PYTHON_MODULE_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# 编译定义
target_compile_definitions(${PYTHON_MODULE_NAME} PRIVATE
    VERSION_INFO=${PYTHON_MODULE_VERSION}
    PY_MAJOR=${PYTHON_VERSION_MAJOR}
    PY_MINOR=${PYTHON_VERSION_MINOR}
    PY_PATCH=${PYTHON_VERSION_PATCH}
)

# 安装 Python 模块
install(TARGETS ${PYTHON_MODULE_NAME}
        COMPONENT Python
        DESTINATION ${PYTHON_SITE_PACKAGES})

# 创建 Python 包结构
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/python/__init__.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/python/__init__.py
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/python/__init__.py
        COMPONENT Python
        DESTINATION ${PYTHON_SITE_PACKAGES}/ic_can)

# 安装 Python 模块
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/python/ic_can/__init__.py
        COMPONENT Python
        DESTINATION ${PYTHON_SITE_PACKAGES}/ic_can)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/python/ic_can/core.py
        COMPONENT Python
        DESTINATION ${PYTHON_SITE_PACKAGES}/ic_can)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/python/ic_can/exceptions.py
        COMPONENT Python
        DESTINATION ${PYTHON_SITE_PACKAGES}/ic_can)

# 开发模式支持
option(DEVELOPMENT_MODE "Enable development mode for Python bindings" OFF)
if(DEVELOPMENT_MODE)
    target_compile_definitions(${PYTHON_MODULE_NAME} PRIVATE
        DEVELOPMENT_MODE=1
    )
    target_compile_options(${PYTHON_MODULE_NAME} PRIVATE -g -O0)
    message(STATUS "Python bindings development mode enabled")
endif()

# 详细的绑定错误报告 (可选)
option(PYBIND11_DETAILED_ERROR_MESSAGES "Enable detailed pybind11 error messages" ON)
if(PYBIND11_DETAILED_ERROR_MESSAGES)
    target_compile_definitions(${PYTHON_MODULE_NAME} PRIVATE
        PYBIND11_DETAILED_ERROR_MESSAGES=1
    )
endif()

# 调试符号 (可选)
option(ENABLE_PYTHON_DEBUG_SYMBOLS "Enable debug symbols for Python module" OFF)
if(ENABLE_PYTHON_DEBUG_SYMBOLS)
    target_compile_options(${PYTHON_MODULE_NAME} PRIVATE -g)
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        target_compile_options(${PYTHON_MODULE_NAME} PRIVATE -O0)
    endif()
    message(STATUS "Python debug symbols enabled")
endif()

# 打印配置信息
message(STATUS "=== Python Bindings Configuration ===")
message(STATUS "Python module name: ${PYTHON_MODULE_NAME}")
message(STATUS "Python version: ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}.${PYTHON_VERSION_PATCH}")
message(STATUS "pybind11 version: ${pybind11_VERSION}")
message(STATUS "Development mode: ${DEVELOPMENT_MODE}")
message(STATUS "=====================================")