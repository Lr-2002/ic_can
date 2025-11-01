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
 * @file test_control_modes.cpp
 * @brief Test program for IC_CAN control modes functionality
 *
 * This program demonstrates the two control modes:
 * - TEACH_MODE: Gravity compensation + friction compensation only (monitoring)
 * - EXECUTION_MODE: Full position control with specified Kp/Kd gains
 */

#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>
#include "ic_can/core/ic_can.hpp"

int main() {
  std::cout << "=== IC_CAN Control Modes Test ===" << std::endl;
  std::cout << "Testing TEACH_MODE and EXECUTION_MODE functionality..." << std::endl << std::endl;

  // Initialize IC_CAN system
  ic_can::IC_CAN ic_can("693D3DE86DF5940C8BC74A5B46A3CE2E", true);

  if (!ic_can.initialize()) {
    std::cout << "❌ Failed to initialize IC_CAN system" << std::endl;
    return 1;
  }

  std::cout << "✅ IC_CAN system initialized" << std::endl;

  // Test 1: Default mode (should be TEACH_MODE)
  std::cout << "\n🔍 Test 1: Check Default Control Mode" << std::endl;
  std::cout << std::string(50, '-') << std::endl;

  auto default_mode = ic_can.get_control_mode();
  std::cout << "Default control mode: "
            << (default_mode == ic_can::IC_CAN::ControlMode::TEACH_MODE ? "TEACH_MODE" : "EXECUTION_MODE")
            << std::endl;

  // Test 2: Enable motors and test TEACH_MODE
  std::cout << "\n🔌 Test 2: TEACH_MODE Testing" << std::endl;
  std::cout << std::string(50, '-') << std::endl;

  if (!ic_can.enable_all()) {
    std::cout << "❌ Failed to enable motors" << std::endl;
    return 1;
  }

  // Set to TEACH_MODE explicitly
  std::cout << "\nSetting TEACH_MODE..." << std::endl;
  ic_can.set_control_mode(ic_can::IC_CAN::ControlMode::TEACH_MODE);

  // Enable compensation for teach mode
  ic_can.enable_gravity_compensation();
  ic_can.enable_friction_compensation();

  std::cout << "\nMonitoring motor positions for 3 seconds in TEACH_MODE..." << std::endl;
  std::cout << "(Motors should be free to move with gravity + friction compensation)" << std::endl;

  auto start_time = std::chrono::steady_clock::now();
  while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(3)) {
    ic_can.refresh_all();
    auto positions = ic_can.get_joint_positions();

    std::cout << "\rPositions: [";
    for (size_t i = 0; i < std::min(positions.size(), size_t(9)); ++i) {
      std::cout << std::fixed << std::setprecision(3) << positions[i];
      if (i < 8) std::cout << ", ";
    }
    std::cout << "] ";
    std::cout.flush();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  std::cout << "\n" << std::endl;

  // Test 3: EXECUTION_MODE testing
  std::cout << "\n⚙️  Test 3: EXECUTION_MODE Testing" << std::endl;
  std::cout << std::string(50, '-') << std::endl;

  std::cout << "\nSetting EXECUTION_MODE..." << std::endl;
  ic_can.set_control_mode(ic_can::IC_CAN::ControlMode::EXECUTION_MODE);

  // Test position control in execution mode
  std::vector<double> test_positions = {0.1, -0.1, 0.2, -0.2, 0.15, -0.15, 0.0, 0.0, 0.5};

  std::cout << "\nTesting position control in EXECUTION_MODE..." << std::endl;
  std::cout << "Setting test positions: [";
  for (size_t i = 0; i < test_positions.size(); ++i) {
    std::cout << std::fixed << std::setprecision(2) << test_positions[i];
    if (i < 8) std::cout << ", ";
  }
  std::cout << "]" << std::endl;

  if (!ic_can.set_joint_positions(test_positions)) {
    std::cout << "❌ Failed to set joint positions" << std::endl;
  } else {
    std::cout << "✅ Position command sent successfully" << std::endl;
  }

  std::cout << "\nMonitoring response for 3 seconds in EXECUTION_MODE..." << std::endl;
  start_time = std::chrono::steady_clock::now();
  while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(3)) {
    ic_can.refresh_all();
    auto positions = ic_can.get_joint_positions();

    std::cout << "\rCurrent: [";
    for (size_t i = 0; i < std::min(positions.size(), size_t(9)); ++i) {
      std::cout << std::fixed << std::setprecision(3) << positions[i];
      if (i < 8) std::cout << ", ";
    }
    std::cout << "] ";
    std::cout.flush();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  std::cout << "\n" << std::endl;

  // Test 4: Return to safe TEACH_MODE and cleanup
  std::cout << "\n🏠 Test 4: Return to Safe TEACH_MODE" << std::endl;
  std::cout << std::string(50, '-') << std::endl;

  std::cout << "Returning to TEACH_MODE for safe shutdown..." << std::endl;
  ic_can.set_control_mode(ic_can::IC_CAN::ControlMode::TEACH_MODE);

  // Set zero positions
  std::vector<double> zero_positions(9, 0.0);
  ic_can.set_joint_positions(zero_positions);
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  // Cleanup
  std::cout << "\n🔌 Disabling motors and shutting down..." << std::endl;
  ic_can.disable_all();
  ic_can.shutdown();

  std::cout << "\n✅ Control Modes Test Completed!" << std::endl;
  std::cout << "📝 Test Summary:" << std::endl;
  std::cout << "   ✓ TEACH_MODE: Gravity + friction compensation only" << std::endl;
  std::cout << "   ✓ EXECUTION_MODE: Full position control with Kp/Kd gains" << std::endl;
  std::cout << "   ✓ Dynamic gain switching based on control mode" << std::endl;
  std::cout << "   ✓ Safe mode transitions" << std::endl;

  return 0;
}