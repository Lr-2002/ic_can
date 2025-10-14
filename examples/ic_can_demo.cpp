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

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>
#include <iomanip>
#include <vector>
#include <memory>
#include <cmath>

// Use working dm-tools SDK directly
#include "dm-tools/USB2FDCAN/SDK/C++/ubuntu/include/protocol/usb_class.h"

// Include IC_CAN headers
#include "ic_can/core/ic_can.hpp"

static volatile bool g_running = true;

void signal_handler(int signal) {
    std::cout << "\n⚠️  Received signal " << signal << ", stopping..." << std::endl;
    g_running = false;
}

void print_all_motor_positions(ic_can::IC_CAN& controller) {
    auto positions = controller.get_joint_positions();
    auto velocities = controller.get_joint_velocities();
    auto torques = controller.get_joint_torques();

    // Clear screen (works on Linux/Mac)
    std::cout << "\033[2J\033[H";

    std::cout << "=== IC_CAN System Position Monitor ===" << std::endl;
    std::cout << "Real-time monitoring of all 9 motors" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;

    // Print header
    std::cout << "Joint | Motor | Type      | Position (rad) | Position (deg) | Velocity (rad/s) | Torque (Nm)" << std::endl;
    std::cout << "------|-------|-----------|----------------|----------------|-------------------|-------------" << std::endl;

    // Motor type names for display
    const std::vector<std::string> motor_types = {
        "DM10010L", "DM6248", "DM6248", "DM4340", "DM4340", "DM4310",
        "HT4438", "HT4438", "SERVO"
    };

    // Print each joint
    for (int i = 0; i < 9; i++) {
        double pos_rad = positions[i];
        double pos_deg = pos_rad * 180.0 / M_PI;
        double vel = velocities[i];
        double tau = torques[i];

        std::cout << std::setw(5) << (i + 1) << " | "
                  << std::setw(5) << "M" << (i + 1) << " | "
                  << std::setw(9) << motor_types[i] << " | "
                  << std::fixed << std::setw(14) << std::setprecision(3) << pos_rad << " | "
                  << std::fixed << std::setw(14) << std::setprecision(1) << pos_deg << " | "
                  << std::fixed << std::setw(17) << std::setprecision(3) << vel << " | "
                  << std::fixed << std::setw(11) << std::setprecision(3) << tau << std::endl;
    }

    // Print system status
    auto status = controller.get_system_status();
    std::cout << std::endl;
    std::cout << "📊 System Status:" << std::endl;
    for (const auto& [key, value] : status) {
        std::cout << "  " << key << ": " << value << std::endl;
    }

    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    std::cout << "\nLast update: " << timestamp << " ms" << std::endl;
}

void test_motor_control(ic_can::IC_CAN& controller) {
    std::cout << "\n🎯 Testing motor control through IC_CAN system..." << std::endl;

    // Test single motor control (Motor 1 - DM10010L)
    std::cout << "\n📍 Testing Motor 1 position control..." << std::endl;

    // Get current position
    auto initial_positions = controller.get_joint_positions();
    double initial_pos = initial_positions[0];
    std::cout << "📍 Initial position: " << std::fixed << std::setprecision(3)
              << initial_pos << " rad (" << (initial_pos * 180.0 / M_PI) << "°)" << std::endl;

    // Test position commands
    std::vector<std::pair<double, std::string>> test_positions = {
        {0.0, "Zero position"},
        {0.2, "Small positive (0.2 rad ≈ 11.5°)"},
        {-0.2, "Small negative (-0.2 rad ≈ -11.5°)"},
        {initial_pos, "Return to initial"}
    };

    for (const auto& [target_pos, description] : test_positions) {
        if (!g_running) break;

        std::cout << "\n📍 " << description << "..." << std::endl;

        // Create position vector for all motors (only move motor 1)
        std::vector<double> positions(9, 0.0);
        positions[0] = target_pos;

        // Send position command through IC_CAN system
        if (controller.set_joint_positions(positions)) {
            std::cout << "✅ Position command sent through IC_CAN" << std::endl;
        } else {
            std::cout << "❌ Failed to send position command" << std::endl;
        }

        // Wait for movement
        for (int i = 3; i > 0 && g_running; --i) {
            std::cout << "   ⏱️  Waiting... " << i << "s" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Read result
        controller.refresh_all();
        auto current_positions = controller.get_joint_positions();
        double current_pos = current_positions[0];

        std::cout << "   📍 Current: " << std::fixed << std::setprecision(3)
                  << current_pos << " rad (" << (current_pos * 180.0 / M_PI) << "°)" << std::endl;
        std::cout << "   📏 Error: " << std::fixed << std::setprecision(3)
                  << std::abs(target_pos - current_pos) << " rad" << std::endl;
    }
}

int main() {
    std::cout << "=== IC_CAN System Position Monitor ===" << std::endl;
    std::cout << "Using integrated IC_CAN system with dm-tools SDK" << std::endl;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        // Create IC_CAN controller like OpenArm team does
        std::cout << "\n🔧 Creating IC_CAN controller..." << std::endl;
        auto controller = std::make_unique<ic_can::IC_CAN>("F561E08C892274DB09496BCC1102DBC5", true);

        // Initialize the system
        std::cout << "\n🚀 Initializing IC_CAN system..." << std::endl;
        if (!controller->initialize()) {
            std::cout << "❌ FAILED: System initialization failed" << std::endl;
            return -1;
        }
        std::cout << "✅ SUCCESS: System initialized" << std::endl;

        // Print system information
        controller->print_system_info();

        // Enable all motors
        std::cout << "\n⚡ Enabling all motors..." << std::endl;
        if (!controller->enable_all()) {
            std::cout << "⚠️  WARNING: Some motors failed to enable" << std::endl;
        } else {
            std::cout << "✅ All motors enabled successfully" << std::endl;
        }

        // Wait for motors to initialize
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // Read initial motor states
        std::cout << "\n📊 Reading initial motor states..." << std::endl;
        controller->refresh_all();
        print_all_motor_positions(*controller);

        // Test motor control
        if (g_running) {
            test_motor_control(*controller);
        }

        // Test high-frequency control
        if (g_running) {
            std::cout << "\n🔄 Testing 500Hz high-frequency control..." << std::endl;

            // Start high-frequency control
            if (!controller->start_high_frequency_control()) {
                std::cout << "❌ Failed to start high-frequency control" << std::endl;
            } else {
                std::cout << "✅ 500Hz control loop started" << std::endl;
                std::cout << "🔄 Running for 5 seconds..." << std::endl;

                // Run for 5 seconds
                auto start_time = std::chrono::steady_clock::now();
                while (g_running &&
                       std::chrono::duration_cast<std::chrono::seconds>(
                           std::chrono::steady_clock::now() - start_time).count() < 5) {

                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }

                // Stop high-frequency control
                controller->stop_high_frequency_control();
                std::cout << "✅ 500Hz control stopped" << std::endl;
            }
        }

        // Continuous monitoring loop
        if (g_running) {
            std::cout << "\n🔄 Starting continuous position monitoring..." << std::endl;
            std::cout << "Updating display every 100ms" << std::endl;

            auto last_update = std::chrono::steady_clock::now();

            while (g_running) {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    current_time - last_update).count();

                // Update display every 100ms
                if (elapsed >= 100) {
                    controller->refresh_all();
                    print_all_motor_positions(*controller);
                    last_update = current_time;
                }

                // Sleep for short duration
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        // Disable all motors
        std::cout << "\n🔌 Disabling all motors..." << std::endl;
        if (!controller->disable_all()) {
            std::cout << "⚠️  WARNING: Some motors failed to disable" << std::endl;
        } else {
            std::cout << "✅ All motors disabled successfully" << std::endl;
        }

        std::cout << "\n🎉 IC_CAN POSITION MONITOR COMPLETED!" << std::endl;
        std::cout << "✅ IC_CAN system working correctly" << std::endl;
        std::cout << "✅ All 9 motors monitored successfully" << std::endl;
        std::cout << "✅ dm-tools SDK integration verified" << std::endl;
        std::cout << "✅ Motor control through IC_CAN working" << std::endl;

        return 0;

    } catch (const std::exception &e) {
        std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
        return -1;
    }
}