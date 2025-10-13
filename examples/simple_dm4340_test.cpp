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

/**
 * @file simple_dm4340_test.cpp
 * @brief 最简单的DM4340电机测试 - enable/disable
 *
 * 这个程序只做最基本的功能测试：
 * 1. 初始化系统
 * 2. 启用DM4340电机（ID=4）
 * 3. 等待20秒
 * 4. 禁用电机
 * 5. 退出
 */

#include <ic_can/core/ic_can.hpp>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <csignal>

// 全局变量用于信号处理
std::unique_ptr<ic_can::IC_CAN> controller;
volatile bool running = true;

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", stopping..." << std::endl;
    running = false;
    if (controller) {
        controller->disable_all();
        controller->stop_high_frequency_control();
    }
}

int main() {
    std::cout << "=== Simple DM4340 Enable/Disable Test ===" << std::endl;
    std::cout << "Testing basic enable/disable functionality" << std::endl;
    std::cout << "Motor ID: 4 (DM4340)" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl << std::endl;

    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        // 创建控制器
        controller = std::make_unique<ic_can::IC_CAN>("F561E08C892274DB09496BCC1102DBC5", true);

        // 初始化系统
        std::cout << "1. Initializing IC_CAN system..." << std::endl;
        if (!controller->initialize()) {
            std::cerr << "Failed to initialize IC_CAN system" << std::endl;
            return -1;
        }
        std::cout << "   ✓ IC_CAN system initialized successfully" << std::endl;

        // 启用电机
        std::cout << "2. Enabling DM4340 motor (ID=4)..." << std::endl;
        if (!controller->enable_all()) {
            std::cerr << "Failed to enable motors" << std::endl;
            return -1;
        }
        std::cout << "   ✓ DM4340 motor enabled successfully" << std::endl;

        // 等待20秒
        std::cout << "3. Waiting for 20 seconds..." << std::endl;
        for (int i = 20; i > 0 && running; --i) {
            std::cout << "   Time remaining: " << std::setw(2) << i << " seconds\r" << std::flush;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::cout << std::endl << "   ✓ Wait completed" << std::endl;

        // 禁用电机
        std::cout << "4. Disabling DM4340 motor..." << std::endl;
        if (!controller->disable_all()) {
            std::cerr << "Failed to disable motors" << std::endl;
            return -1;
        }
        std::cout << "   ✓ DM4340 motor disabled successfully" << std::endl;

        std::cout << "\n=== Test Completed Successfully ===" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }

    // 清理
    if (controller) {
        std::cout << "Cleaning up resources..." << std::endl;
        controller->disable_all();
        controller->stop_high_frequency_control();
    }

    std::cout << "Simple test completed" << std::endl;
    return 0;
}