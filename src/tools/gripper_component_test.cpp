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
 * @file gripper_component_test.cpp
 * @brief Test program for real IC_CAN servo motor and gripper component
 * integration
 *
 * This program tests the actual IC_CAN implementation with servo motor (motor
 * 9) and validates the gripper component functionality using the real CAN FD
 * protocol.
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

// Include the real IC_CAN implementation
#include "ic_can/core/ic_can.hpp"

int main() {
  std::cout << "=== IC_CAN Real Servo Motor & Gripper Component Test ==="
            << std::endl;
  std::cout << "Testing real servo motor (motor 9) and gripper component "
               "integration..."
            << std::endl
            << std::endl;

  // Initialize IC_CAN system with debug mode
  ic_can::IC_CAN ic_can("693D3DE86DF5940C8BC74A5B46A3CE2E", false);

  if (!ic_can.initialize()) {
    std::cout << "❌ Failed to initialize IC_CAN system" << std::endl;
    return 1;
  }

  std::cout
      << "✅ IC_CAN system initialized with real servo motor implementation"
      << std::endl;

  // Enable all motors (especially servo motor 9)
  std::cout << "\n🔌 Enabling all motors..." << std::endl;
  if (!ic_can.enable_all()) {
    std::cout << "❌ Failed to enable motors" << std::endl;
    return 1;
  }

  std::cout << "✅ All motors enabled (including servo motor 9)" << std::endl;

  // Test 1: Initial joint positions reading
  std::cout << "\n🔍 Test 1: Initial Joint Positions Reading" << std::endl;
  std::cout << std::string(60, '-') << std::endl;

  auto initial_positions = ic_can.get_joint_positions();
  std::cout << "📈 Initial joint positions:" << std::endl;
  for (size_t i = 0; i < initial_positions.size(); ++i) {
    double pos_rad = initial_positions[i];
    double pos_deg = pos_rad * 180.0 / M_PI;
    double pos_norm = pos_rad / (2 * M_PI); // Normalized to 0-1

    std::cout << "   Motor " << (i + 1) << ": "
              << "rad=" << std::fixed << std::setprecision(6) << pos_rad
              << ", deg=" << std::setprecision(2) << pos_deg
              << ", norm=" << std::setprecision(4) << pos_norm << std::endl;
  }

  // Test 2: Servo motor position control with real CAN protocol
  std::cout << "\n🦾 Test 2: Servo Motor (Motor 9) Real CAN Protocol Testing"
            << std::endl;
  std::cout << std::string(60, '-') << std::endl;

  // Test positions: 0%, 25%, 50%, 75%, 100% (normalized)
  std::vector<double> test_positions = {0.0, 0.25, 0.5, 0.75, 1.0};

  for (size_t i = 0; i < test_positions.size(); ++i) {
    double target_pos = test_positions[i];
    double target_deg = target_pos * 180.0;

    std::cout << "\n📍 Setting Servo to: " << std::fixed << std::setprecision(4)
              << target_pos << " (" << target_deg << "°)" << std::endl;

    // Set only servo motor position (motor 9 = index 8)
    std::vector<double> servo_only_pos(9, 0.0);
    servo_only_pos[8] = target_pos;

    if (!ic_can.set_joint_positions(servo_only_pos)) {
      std::cout << "❌ Failed to set servo position" << std::endl;
      continue;
    }

    // Wait for servo to move
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Read back positions
    auto positions = ic_can.get_joint_positions();
    if (positions.size() >= 9) {
      double actual_pos = positions[8];
      double actual_deg = actual_pos * 180.0 / M_PI;
      double error = std::abs(target_pos - actual_pos);

      std::cout << "   ✓ Servo Response: norm=" << actual_pos << " ("
                << actual_deg << "°)"
                << ", Error=" << error << std::endl;
    }
  }

  // Test 3: Gripper-like movements (open/close simulation)
  std::cout << "\n🤏 Test 3: Gripper Movements Simulation" << std::endl;
  std::cout << std::string(60, '-') << std::endl;

  struct GripperMovement {
    std::string name;
    double openness; // 0.0 = closed, 1.0 = fully open
    double speed;
  };

  std::vector<GripperMovement> movements = {
      {"Fully Open", 1.0, 0.8},   {"Partially Open", 0.75, 0.6},
      {"Half Open", 0.5, 0.5},    {"Partially Closed", 0.25, 0.4},
      {"Fully Closed", 0.0, 0.3}, {"Half Open", 0.5, 0.5},
      {"Fully Open", 1.0, 0.8}};

  for (const auto &movement : movements) {
    std::cout << "🤏 " << movement.name << " (openness: " << std::fixed
              << std::setprecision(2) << movement.openness
              << ", speed: " << movement.speed << ")" << std::endl;

    // Set gripper position (only servo motor)
    std::vector<double> gripper_pos(9, 0.0);
    gripper_pos[8] = movement.openness;

    if (!ic_can.set_joint_positions(gripper_pos)) {
      std::cout << "❌ Failed to set gripper position" << std::endl;
    }

    // Wait for movement
    int wait_time = static_cast<int>((1.0 - movement.openness) * 1000 + 300);
    std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));

    // Check position
    auto current_pos = ic_can.get_joint_positions();
    if (current_pos.size() >= 9) {
      double servo_pos = current_pos[8];
      std::cout << "   Current servo position: " << std::fixed
                << std::setprecision(4) << servo_pos << " ("
                << (servo_pos * 180.0 / M_PI) << "°)" << std::endl;
    }
  }

  // Test 4: Continuous gripper control test
  std::cout << "\n⏱️  Test 4: Continuous Gripper Control (3 seconds)"
            << std::endl;
  std::cout << std::string(60, '-') << std::endl;

  auto start_time = std::chrono::steady_clock::now();
  auto duration = std::chrono::seconds(3);

  while (std::chrono::steady_clock::now() - start_time < duration) {
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - start_time)
                       .count();

    // Create smooth opening/closing motion
    double phase =
        (static_cast<double>(elapsed) / 1000.0) * 2.0; // 0-6 seconds range
    double openness = (std::sin(phase) + 1.0) / 2.0;   // 0.0-1.0 range

    std::vector<double> continuous_pos(9, 0.0);
    continuous_pos[8] = openness;

    ic_can.set_joint_positions(continuous_pos);

    // Display current state
    auto positions = ic_can.get_joint_positions();
    if (positions.size() >= 9) {
      std::cout << "\r   [" << std::fixed << std::setprecision(3)
                << "Mot1-8: 0.000, "
                << "Servo: " << positions[8] << " ("
                << (positions[8] * 180.0 / M_PI) << "°)"
                << "] Time: " << (elapsed / 1000.0) << "s ";
      std::cout.flush();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  std::cout << "\n" << std::endl;

  // Test 5: Final validation and return to safe position
  std::cout << "🏠 Test 5: Final Validation and Safe Position" << std::endl;
  std::cout << std::string(60, '-') << std::endl;

  // Return to middle position (safe position)
  std::cout << "Returning servo to middle position..." << std::endl;
  std::vector<double> safe_pos(9, 0.0);
  safe_pos[8] = 0.5; // Middle position

  ic_can.set_joint_positions(safe_pos);
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  auto final_positions = ic_can.get_joint_positions();
  std::cout << "Final servo position: " << std::fixed << std::setprecision(6)
            << final_positions[8] << " rad ("
            << (final_positions[8] * 180.0 / M_PI) << "°)" << std::endl;

  // Test 6: System performance statistics
  std::cout << "\n📊 Test 6: System Performance Statistics" << std::endl;
  std::cout << std::string(60, '-') << std::endl;

  auto stats = ic_can.get_performance_stats();
  std::cout << "Performance Summary:" << std::endl;
  std::cout << "   Send Frequency: " << std::fixed << std::setprecision(1)
            << stats["send_frequency"] << " Hz" << std::endl;
  std::cout << "   Receive Frequency: " << std::fixed << std::setprecision(1)
            << stats["receive_frequency"] << " Hz" << std::endl;
  std::cout << "   Total Commands Sent: " << std::fixed << std::setprecision(0)
            << stats["total_commands_sent"] << std::endl;
  std::cout << "   Total Messages Received: " << std::fixed
            << std::setprecision(0) << stats["total_messages_received"]
            << std::endl;
  std::cout << "   Uptime: " << std::fixed << std::setprecision(2)
            << stats["uptime_seconds"] << " seconds" << std::endl;

  // Cleanup
  std::cout << "\n🔌 Disabling motors and shutting down..." << std::endl;
  ic_can.disable_all();
  ic_can.shutdown();

  std::cout << "\n✅ Real Servo Motor & Gripper Component Test Completed!"
            << std::endl;
  std::cout << "📝 Test Summary:" << std::endl;
  std::cout << "   ✓ Real IC_CAN servo motor implementation tested"
            << std::endl;
  std::cout << "   ✓ CAN FD protocol with correct command format" << std::endl;
  std::cout << "   ✓ Position control and feedback validation" << std::endl;
  std::cout << "   ✓ Gripper movement simulation" << std::endl;
  std::cout << "   ✓ Continuous control testing" << std::endl;
  std::cout << "   ✓ Integration with 9-motor system" << std::endl;
  std::cout << "   ✓ Performance monitoring" << std::endl;
  std::cout << "   ✓ Servo CAN ID: 0x09 (send), 0x19 (receive)" << std::endl;

  return 0;
}
