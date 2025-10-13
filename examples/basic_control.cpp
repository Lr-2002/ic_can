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
 * @file basic_example.cpp
 * @brief Basic usage example for IC_CAN library
 */

#include <ic_can/core/ic_can.hpp>
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "=== IC_CAN Basic Example ===" << std::endl;

    try {
        // Create IC_CAN controller with USB2CAN device
        ic_can::IC_CAN controller("F561E08C892274DB09496BCC1102DBC5", true);

        // Initialize the system
        std::cout << "Initializing IC_CAN system..." << std::endl;
        if (!controller.initialize()) {
            std::cerr << "Failed to initialize IC_CAN system" << std::endl;
            return -1;
        }
        std::cout << "IC_CAN system initialized successfully" << std::endl;

        // Print system information
        controller.print_system_info();

        // Enable all motors
        std::cout << "Enabling all motors..." << std::endl;
        if (!controller.enable_all()) {
            std::cerr << "Failed to enable motors" << std::endl;
            return -1;
        }
        std::cout << "All motors enabled" << std::endl;

        // Wait for motors to be ready
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // Get current joint positions
        auto positions = controller.get_joint_positions();
        std::cout << "Current joint positions: ";
        for (size_t i = 0; i < positions.size(); ++i) {
            std::cout << "j" << (i+1) << "=" << positions[i];
            if (i < positions.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;

        // Set joint positions (small movement)
        std::vector<double> target_positions = {0.1, -0.1, 0.1, -0.1, 0.1, -0.1, 0.0, 0.0, 0.0};
        std::cout << "Setting target positions..." << std::endl;
        if (!controller.set_joint_positions(target_positions)) {
            std::cerr << "Failed to set joint positions" << std::endl;
        }

        // Wait for movement
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        // Update and read positions again
        controller.update_all_states();
        positions = controller.get_joint_positions();
        std::cout << "New joint positions: ";
        for (size_t i = 0; i < positions.size(); ++i) {
            std::cout << "j" << (i+1) << "=" << positions[i];
            if (i < positions.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;

        // Start high-frequency control (500Hz)
        std::cout << "Starting high-frequency control..." << std::endl;
        if (!controller.start_high_frequency_control()) {
            std::cerr << "Failed to start high-frequency control" << std::endl;
        } else {
            std::cout << "High-frequency control started" << std::endl;

            // Run control for a few seconds
            std::this_thread::sleep_for(std::chrono::seconds(3));

            // Stop high-frequency control
            controller.stop_high_frequency_control();
            std::cout << "High-frequency control stopped" << std::endl;
        }

        // Print safety status
        auto& safety = controller.get_safety();
        safety.print_safety_status_report();

        // Disable all motors
        std::cout << "Disabling all motors..." << std::endl;
        controller.disable_all();
        std::cout << "All motors disabled" << std::endl;

        std::cout << "Example completed successfully!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}