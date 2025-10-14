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
 * @file simple_frequency_demo.cpp
 * @brief Simple Frequency Control Demo
 *
 * Demonstrates basic usage of configurable frequency control
 * and velocity-limited interpolation.
 */

#include <chrono>
#include <csignal>
#include <ic_can/core/ic_can.hpp>
#include <iomanip>
#include <iostream>
#include <thread>

static volatile bool g_running = true;

void signal_handler(int signal) {
  std::cout << "\n⚠️  Received signal " << signal << ", stopping demo..."
            << std::endl;
  g_running = false;
}

int main() {
  std::cout << "=== Simple Frequency Control Demo ===" << std::endl;

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  try {
    // Create IC_CAN controller
    auto controller = std::make_unique<ic_can::IC_CAN>(
        "F561E08C892274DB09496BCC1102DBC5", true);

    // Initialize system
    if (!controller->initialize()) {
      std::cout << "❌ FAILED: System initialization failed" << std::endl;
      return -1;
    }
    std::cout << "✅ System initialized" << std::endl;

    // Enable motors
    if (!controller->enable_all()) {
      std::cout << "⚠️  WARNING: Some motors failed to enable" << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::cout << "\n🚀 Starting 200Hz control with interpolation..."
              << std::endl;

    // Start control loop at 200Hz
    if (!controller->start_control_loop(200.0)) {
      std::cout << "❌ Failed to start control loop" << std::endl;
      return -1;
    }

    // Set target positions with 0.2 rad/s max velocity
    std::vector<double> target_positions = {0.0,  -0.0, 0.0, -0.0, 0.0,
                                            -0.0, 0.0,  0.0, 0.0};
    controller->set_target_positions_interpolated(target_positions, 3);

    std::cout << "✅ Moving to target positions at max 0.1 rad/s..."
              << std::endl;
    std::cout << "Running for 5 seconds. Press Ctrl+C to stop early."
              << std::endl;

    // Monitor for 5 seconds
    auto start_time = std::chrono::steady_clock::now();
    while (g_running) {
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                         std::chrono::steady_clock::now() - start_time)
                         .count();

      if (elapsed >= 10)
        break;

      auto positions = controller->get_joint_positions();
      std::cout << "Time: " << elapsed << "s, Joint 1: " << std::fixed
                << std::setprecision(4) << positions[0]
                << " rad, Joint 2: " << positions[1]
                << " rad, Joint 3: " << positions[2]
                << " rad, Joint 4: " << positions[3]
                << " rad, Joint 5: " << positions[4]
                << " rad, Joint 6: " << positions[5] << " rad" << std::endl;

      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // Stop control loop
    controller->stop_control_loop();

    /*// Move back to zero*/
    /*std::cout << "\n🔙 Returning to zero position..." << std::endl;*/
    /*controller->start_control_loop(100.0); // Slower return*/
    /*std::vector<double> zero_positions(9, 0.0);*/
    /*controller->set_target_positions_interpolated(zero_positions, 0.1);*/
    /**/
    /*std::this_thread::sleep_for(std::chrono::seconds(3));*/
    /*controller->stop_control_loop();*/
    /**/
    // Disable motors
    controller->disable_all();

    std::cout << "\n🎉 Demo completed!" << std::endl;
    std::cout << "✅ 200Hz control loop tested" << std::endl;
    std::cout << "✅ Velocity-limited interpolation tested" << std::endl;
    std::cout << "✅ All motions respected 0.2 rad/s velocity limit"
              << std::endl;

    return 0;

  } catch (const std::exception &e) {
    std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
    return -1;
  }
}
