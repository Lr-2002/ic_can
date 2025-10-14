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
 * @file ic_can_usage_example.cpp
 * @brief Example usage of IC_CAN system
 *
 * This demonstrates how to use the IC_CAN system like the OpenArm team does.
 */

#include <chrono>
#include <csignal>
#include <ic_can/core/ic_can.hpp>
#include <iostream>
#include <thread>

static volatile bool g_running = true;

void signal_handler(int signal) {
  std::cout << "\n⚠️  Received signal " << signal << ", stopping..."
            << std::endl;
  g_running = false;
}

int main() {
  std::cout << "=== IC_CAN Usage Example ===" << std::endl;

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  try {
    // Create IC_CAN controller like OpenArm team does
    std::cout << "🔧 Creating IC_CAN controller..." << std::endl;
    auto controller = std::make_unique<ic_can::IC_CAN>(
        "F561E08C892274DB09496BCC1102DBC5", true);

    // Initialize the system
    std::cout << "🚀 Initializing IC_CAN system..." << std::endl;
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

    // Simple position control example
    std::cout << "\n🎯 Position Control Example:" << std::endl;

    // Get current positions
    controller->refresh_all();
    auto current_positions = controller->get_joint_positions();

    std::cout << "Current positions (rad): ";
    for (size_t i = 0; i < current_positions.size(); ++i) {
      std::cout << "J" << (i + 1) << "=" << current_positions[i];
      if (i < current_positions.size() - 1)
        std::cout << ", ";
    }
    std::cout << std::endl;
    /**/
    /*// Move to zero position*/
    /*std::vector<double> zero_positions(9, 0.0);*/
    /*std::cout << "Moving all joints to zero position..." << std::endl;*/
    /*if (controller->set_joint_positions(zero_positions)) {*/
    /*    std::cout << "✅ Position command sent" << std::endl;*/
    /*}*/
    /**/
    /*// Wait for movement*/
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Read new positions
    controller->refresh_all();
    auto new_positions = controller->get_joint_positions();

    std::cout << "New positions (rad): ";
    for (size_t i = 0; i < new_positions.size(); ++i) {
      std::cout << "J" << (i + 1) << "=" << new_positions[i];
      if (i < new_positions.size() - 1)
        std::cout << ", ";
    }
    std::cout << std::endl;

    // Test high-frequency control
    std::cout << "\n🔄 Testing 500Hz control for 3 seconds..." << std::endl;
    if (controller->start_high_frequency_control()) {
      std::this_thread::sleep_for(std::chrono::seconds(3));
      controller->stop_high_frequency_control();
      std::cout << "✅ High-frequency control test completed" << std::endl;
    } else {
      std::cout << "❌ Failed to start high-frequency control" << std::endl;
    }

    // Disable all motors
    std::cout << "\n🔌 Disabling all motors..." << std::endl;
    if (!controller->disable_all()) {
      std::cout << "⚠️  WARNING: Some motors failed to disable" << std::endl;
    } else {
      std::cout << "✅ All motors disabled successfully" << std::endl;
    }

    std::cout << "\n🎉 IC_CAN Usage Example Completed!" << std::endl;
    std::cout << "✅ Successfully demonstrated IC_CAN API usage" << std::endl;

    return 0;

  } catch (const std::exception &e) {
    std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
    return -1;
  }
}
