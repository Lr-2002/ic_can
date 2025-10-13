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
 * @file can_id1_test.cpp
 * @brief 测试CAN ID 1发送和接收ID 0x11的enable/disable功能
 *
 * 专门测试：
 * - CAN发送ID = 1
 * - CAN接收ID = 0x11
 * - 电机enable/disable命令
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

// 手动发送CAN帧测试
void test_manual_can_frames() {
    std::cout << "\n=== Manual CAN Frame Test ===" << std::endl;

    // 创建一个USB2CAN适配器直接测试
    try {
        auto adapter = std::make_unique<ic_can::USB2CANAdapter>("TEST", true);

        std::cout << "1. Connecting to CAN interface..." << std::endl;
        if (!adapter->connect()) {
            std::cerr << "Failed to connect to CAN interface" << std::endl;
            return;
        }
        std::cout << "   ✓ Connected to CAN interface" << std::endl;

        // 发送enable命令到CAN ID 1
        std::cout << "2. Sending ENABLE command to CAN ID 1..." << std::endl;
        std::vector<uint8_t> enable_cmd = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC}; // Enable command
        if (adapter->send_frame(1, enable_cmd)) {
            std::cout << "   ✓ Enable command sent to ID 1" << std::endl;
        } else {
            std::cerr << "   ✗ Failed to send enable command" << std::endl;
        }

        // 等待响应
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 检查是否有来自ID 0x11的响应
        std::cout << "3. Checking for response from ID 0x11..." << std::endl;
        uint32_t recv_id;
        std::vector<uint8_t> recv_data;
        if (adapter->receive_frame(recv_id, recv_data, 100)) { // 100ms timeout
            std::cout << "   ✓ Received response from ID 0x" << std::hex << recv_id << std::dec << std::endl;
            std::cout << "   Data: ";
            for (uint8_t byte : recv_data) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
            }
            std::cout << std::dec << std::endl;
        } else {
            std::cout << "   No response from ID 0x11" << std::endl;
        }

        // 等待5秒
        std::cout << "4. Waiting 5 seconds..." << std::endl;
        for (int i = 5; i > 0; --i) {
            std::cout << "   " << i << " seconds\r" << std::flush;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::cout << std::endl;

        // 发送disable命令
        std::cout << "5. Sending DISABLE command to CAN ID 1..." << std::endl;
        std::vector<uint8_t> disable_cmd = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD}; // Disable command
        if (adapter->send_frame(1, disable_cmd)) {
            std::cout << "   ✓ Disable command sent to ID 1" << std::endl;
        } else {
            std::cerr << "   ✗ Failed to send disable command" << std::endl;
        }

        // 检查响应
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (adapter->receive_frame(recv_id, recv_data, 100)) {
            std::cout << "   ✓ Received response from ID 0x" << std::hex << recv_id << std::dec << std::endl;
        } else {
            std::cout << "   No response from ID 0x11" << std::endl;
        }

        adapter->disconnect();
        std::cout << "6. ✓ CAN interface disconnected" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Manual CAN test error: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "=== CAN ID 1 / 0x11 Enable/Disable Test ===" << std::endl;
    std::cout << "Testing CAN communication:" << std::endl;
    std::cout << "  Send ID: 1" << std::endl;
    std::cout << "  Receive ID: 0x11" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl << std::endl;

    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        // 1. 手动CAN帧测试
        test_manual_can_frames();

        std::cout << "\n=== IC_CAN System Test ===" << std::endl;

        // 创建控制器
        controller = std::make_unique<ic_can::IC_CAN>("TEST_DEVICE", true);

        // 初始化系统
        std::cout << "A. Initializing IC_CAN system..." << std::endl;
        if (!controller->initialize()) {
            std::cerr << "Failed to initialize IC_CAN system" << std::endl;
            return -1;
        }
        std::cout << "   ✓ IC_CAN system initialized" << std::endl;

        // 启用电机（只测试电机1，使用CAN ID 1/0x11）
        std::cout << "B. Enabling motor 1 (CAN ID 1/0x11)..." << std::endl;
        if (!controller->enable_all()) {
            std::cerr << "Failed to enable motors" << std::endl;
            return -1;
        }
        std::cout << "   ✓ Motor 1 enabled" << std::endl;

        // 等待10秒监控状态
        std::cout << "C. Monitoring for 10 seconds..." << std::endl;
        for (int i = 0; i < 10 && running; ++i) {
            std::cout << "   Monitoring: " << (10-i) << "s remaining\r" << std::flush;

            // 更新状态
            controller->update_all_states();

            // 获取电机1的状态
            auto positions = controller->get_joint_positions();
            if (positions.size() > 0) {
                std::cout << "  Motor 1 position: " << std::fixed << std::setprecision(3) << positions[0] << " rad" << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::cout << std::endl;

        // 禁用电机
        std::cout << "D. Disabling motor 1..." << std::endl;
        if (!controller->disable_all()) {
            std::cerr << "Failed to disable motors" << std::endl;
            return -1;
        }
        std::cout << "   ✓ Motor 1 disabled" << std::endl;

        std::cout << "\n=== Test Completed ===" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }

    // 清理
    if (controller) {
        controller->disable_all();
        controller->stop_high_frequency_control();
    }

    std::cout << "CAN ID 1/0x11 test completed" << std::endl;
    return 0;
}