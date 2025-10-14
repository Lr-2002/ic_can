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
 * @file motor_gain_demo.cpp
 * @brief Motor Gain Configuration Demo
 *
 * This demo demonstrates:
 * 1. Setting individual motor P and D gains
 * 2. Setting all motor gains at once
 * 3. Reading current motor gains
 * 4. Loading default motor gains
 * 5. Using motor-specific gains in position control
 */

#include <chrono>
#include <csignal>
#include <ic_can/core/ic_can.hpp>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

static volatile bool g_running = true;

void signal_handler(int signal) {
  std::cout << "\n⚠️  Received signal " << signal << ", stopping demo..."
            << std::endl;
  g_running = false;
}

void print_motor_gains(ic_can::IC_CAN& controller, const std::string& title) {
  std::cout << "\n" << title << std::endl;
  std::cout << std::string(50, '-') << std::endl;
  std::cout << "Motor | Kp (P Gain) | Kd (D Gain) | Type" << std::endl;
  std::cout << "------|------------|------------|-------------" << std::endl;

  for (int i = 1; i <= 9; i++) {
    double kp, kd;
    controller.get_motor_gains(i, kp, kd);

    std::string motor_type;
    if (i <= 6) {
      motor_type = "Damiao";
    } else if (i <= 8) {
      motor_type = "HT";
    } else {
      motor_type = "Servo";
    }

    std::cout << std::setw(5) << i << " | "
              << std::setw(10) << std::fixed << std::setprecision(1) << kp << " | "
              << std::setw(10) << std::setprecision(2) << kd << " | "
              << motor_type << std::endl;
  }
}

int main() {
  std::cout << "=== IC_CAN Motor Gain Configuration Demo ===" << std::endl;

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

    // Show default gains
    print_motor_gains(*controller, "🔧 Default Motor Gains (Loaded Automatically)");

    // Enable motors
    if (!controller->enable_all()) {
      std::cout << "⚠️  WARNING: Some motors failed to enable" << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::cout << "\n🚀 Starting motor gain configuration demo..." << std::endl;

    // Demo 1: Set individual motor gains
    std::cout << "\n📝 Demo 1: Setting individual motor gains" << std::endl;

    // Set higher gains for motor 1 (should be more responsive)
    controller->set_motor_gains(1, 50.0, 2.0);

    // Set softer gains for motor 2 (should be smoother)
    controller->set_motor_gains(2, 10.0, 0.8);

    // Set precise gains for servo motor (motor 9)
    controller->set_motor_gains(9, 80.0, 5.0);

    print_motor_gains(*controller, "🎯 Updated Motor Gains (Individual Changes)");

    // Demo 2: Set all motor gains at once
    std::cout << "\n📝 Demo 2: Setting all motor gains with custom values" << std::endl;

    std::vector<double> custom_kp = {
      25.0, 15.0, 20.0, 18.0, 22.0, 30.0,  // Damiao motors (1-6)
      12.0, 12.0,                           // HT motors (7-8)
      40.0                                  // Servo motor (9)
    };

    std::vector<double> custom_kd = {
      1.0, 0.6, 0.8, 0.7, 0.9, 1.2,       // Damiao motors (1-6)
      0.4, 0.4,                            // HT motors (7-8)
      2.0                                  // Servo motor (9)
    };

    controller->set_all_motor_gains(custom_kp, custom_kd);
    print_motor_gains(*controller, "🎯 Custom Motor Gains (All Motors)");

    // Demo 3: Test gains in position control
    std::cout << "\n📝 Demo 3: Testing motor gains in position control" << std::endl;
    std::cout << "Starting 100Hz control loop with different gains..." << std::endl;

    if (!controller->start_control_loop(100.0)) {
      std::cout << "❌ Failed to start control loop" << std::endl;
      return -1;
    }

    // Test positions with small movements
    std::vector<std::vector<double>> test_positions = {
      {0.1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},  // Move motor 1 slightly
      {0.1, 0.1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},  // Add motor 2
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.05},  // Move servo motor
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}   // Return to zero
    };

    std::vector<std::string> descriptions = {
      "Moving motor 1 (high gains) - should be fast/precise",
      "Adding motor 2 (soft gains) - should be smooth",
      "Moving servo motor (high gains) - should be very precise",
      "Returning to zero position"
    };

    for (size_t i = 0; i < test_positions.size() && g_running; ++i) {
      std::cout << "\n📍 " << descriptions[i] << std::endl;
      controller->set_target_positions_interpolated(test_positions[i], 0.2);

      // Monitor for 3 seconds
      auto start_time = std::chrono::steady_clock::now();
      while (g_running) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                           std::chrono::steady_clock::now() - start_time)
                           .count();
        if (elapsed >= 3) break;

        auto positions = controller->get_joint_positions();
        std::cout << "Time: " << elapsed << "s, Motor 1: " << std::fixed
                  << std::setprecision(4) << positions[0] << ", Motor 2: "
                  << positions[1] << ", Servo: " << positions[8] << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
    }

    // Stop control loop
    controller->stop_control_loop();

    // Demo 4: Reset to default gains
    std::cout << "\n📝 Demo 4: Resetting to default motor gains" << std::endl;
    controller->load_default_motor_gains();
    print_motor_gains(*controller, "🔧 Reset to Default Motor Gains");

    // Disable motors
    controller->disable_all();

    std::cout << "\n🎉 Motor Gain Configuration Demo Completed!" << std::endl;
    std::cout << "✅ Individual motor gain setting tested" << std::endl;
    std::cout << "✅ All motor gain setting tested" << std::endl;
    std::cout << "✅ Motor gain reading tested" << std::endl;
    std::cout << "✅ Position control with custom gains tested" << std::endl;
    std::cout << "✅ Default gain loading tested" << std::endl;

    return 0;

  } catch (const std::exception &e) {
    std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
    return -1;
  }
}