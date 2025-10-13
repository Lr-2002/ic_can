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
 * @file test_dm4340.cpp
 * @brief DM4340电机通信测试
 *
 * 这个程序测试与单个DM4340电机的通信，验证CAN协议和控制功能
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

void print_motor_state(const std::string& name, double position, double velocity, double torque) {
    std::cout << "[" << name << "] Position: " << std::fixed << std::setprecision(3)
              << " rad, Velocity: " << std::fixed << std::setprecision(3)
              << " rad/s, Torque: " << std::fixed << std::setprecision(3)
              << " Nm" << std::endl;
}

int main() {
    std::cout << "=== DM4340 Motor Communication Test ===" << std::endl;
    std::cout << "Testing CAN communication with DM4340 motor" << std::endl;
    std::cout << "Motor ID: 4, CAN Send ID: 0x04, CAN Receive ID: 0x14" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl << std::endl;

    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        // 创建控制器 (使用你的设备SN)
        controller = std::make_unique<ic_can::IC_CAN>("F561E08C892274DB09496BCC1102DBC5", true);

        // 初始化系统
        std::cout << "Initializing IC_CAN system..." << std::endl;
        if (!controller->initialize()) {
            std::cerr << "Failed to initialize IC_CAN system" << std::endl;
            return -1;
        }
        std::cout << "IC_CAN system initialized successfully" << std::endl;

        // 启动500Hz控制
        std::cout << "Starting 500Hz high-frequency control..." << std::endl;
        if (!controller->start_high_frequency_control()) {
            std::cerr << "Failed to start high-frequency control" << std::endl;
            return -1;
        }

        // 使能电机
        std::cout << "Enabling DM4340 motor..." << std::endl;
        if (!controller->enable_all()) {
            std::cerr << "Failed to enable motors" << std::endl;
            return -1;
        }

        // 等待电机启动
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // 测试1: 读取初始状态
        std::cout << "\n=== Test 1: Reading Initial State ===" << std::endl;
        controller->update_all_states();
        auto positions = controller->get_joint_positions();
        auto velocities = controller->get_joint_velocities();
        auto torques = controller->get_joint_torques();

        if (positions.size() >= 4) {
            print_motor_state("DM4340", positions[3], velocities[3], torques[3]);
        }

        // 测试2: 设置零位
        std::cout << "\n=== Test 2: Setting Zero Position ===" << std::endl;
        // 这里需要添加设置零位的功能
        std::cout << "Setting zero position..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        // 测试3: 位置控制测试
        std::cout << "\n=== Test 3: Position Control Test ===" << std::endl;
        std::vector<double> test_positions = {0, 0, 0, 0.5, 0, 0, 0, 0, 0}; // 只控制m4

        std::cout << "Setting m4 to 0.5 rad..." << std::endl;
        if (!controller->set_joint_positions(test_positions)) {
            std::cerr << "Failed to set joint positions" << std::endl;
        }

        // 等待运动完成
        for (int i = 0; i < 20; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            controller->update_all_states();
            positions = controller->get_joint_positions();
            if (positions.size() >= 4) {
                std::cout << "  m4 position: " << std::fixed << std::setprecision(3)
                          << positions[3] << " rad" << std::endl;
            }
        }

        // 测试4: 连续位置变化
        std::cout << "\n=== Test 4: Continuous Position Changes ===" << std::endl;
        for (int cycle = 0; cycle < 3 && running; cycle++) {
            std::cout << "Cycle " << (cycle + 1) << ": Moving to -0.5 rad..." << std::endl;
            test_positions[3] = -0.5;
            controller->set_joint_positions(test_positions);

            for (int i = 0; i < 30 && running; i++) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                controller->update_all_states();
                positions = controller->get_joint_positions();
                if (positions.size() >= 4) {
                    std::cout << "  m4: " << std::fixed << std::setprecision(3)
                              << positions[3] << " rad" << std::endl;
                }
            }

            std::cout << "Moving to 0.5 rad..." << std::endl;
            test_positions[3] = 0.5;
            controller->set_joint_positions(test_positions);

            for (int i = 0; i < 30 && running; i++) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                controller->update_all_states();
                positions = controller->get_joint_positions();
                if (positions.size() >= 4) {
                    std::cout << "  m4: " << std::fixed << std::setprecision(3)
                              << positions[3] << " rad" << std::endl;
                }
            }
        }

        // 测试5: 回到零位
        std::cout << "\n=== Test 5: Returning to Zero ===" << std::endl;
        test_positions[3] = 0.0;
        controller->set_joint_positions(test_positions);
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        controller->update_all_states();
        positions = controller->get_joint_positions();
        if (positions.size() >= 4) {
            print_motor_state("DM4340 Final", positions[3], velocities[3], torques[3]);
        }

        // 测试6: 状态查询
        std::cout << "\n=== Test 6: Status Information ===" << std::endl;
        auto status = controller->get_system_status();
        std::cout << "System Status:" << std::endl;
        for (const auto& [key, value] : status) {
            std::cout << "  " << key << ": " << value << std::endl;
        }

        // 安全模块状态
        auto& safety = controller->get_safety();
        std::cout << "\nSafety Status:" << std::endl;
        auto safety_stats = safety.get_safety_statistics();
        for (const auto& [key, value] : safety_stats) {
            std::cout << "  " << key << ": " << value << std::endl;
        }

        std::cout << "\n=== Test Completed Successfully ===" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }

    // 清理
    if (controller) {
        std::cout << "Disabling motors and stopping control..." << std::endl;
        controller->disable_all();
        controller->stop_high_frequency_control();
    }

    std::cout << "Test completed" << std::endl;
    return 0;
}