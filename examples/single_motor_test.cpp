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
 * @file single_motor_test.cpp
 * @brief Single Motor Connection and Control Test for dm-tools SDK
 *
 * This test specifically focuses on verifying:
 * 1. dm-tools SDK connection to USB2CAN device
 * 2. Basic CAN communication with a single motor
 * 3. Motor enable/disable functionality
 * 4. Simple position control
 * 5. Real-time feedback reading
 *
 * Usage:
 *   ./single_motor_test [device_serial] [motor_id]
 *
 * Default: device_serial="F561E08C892274DB09496BCC1102DBC5", motor_id=4
 */

#include <chrono>
#include <cmath>
#include <csignal>
#include <ic_can/core/ic_can.hpp>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

// Global variables for signal handling
std::unique_ptr<ic_can::IC_CAN> controller;
volatile bool running = true;
volatile bool test_passed = true;

void signal_handler(int signal) {
  std::cout << "\n⚠️  Received signal " << signal << ", stopping test..."
            << std::endl;
  running = false;
  if (controller) {
    controller->disable_all();
    controller->stop_high_frequency_control();
  }
}

void print_test_header(const std::string &device_sn, int motor_id) {
  std::cout << "\n" << std::string(60, '=') << std::endl;
  std::cout << "🔧 SINGLE MOTOR DM-TOOLS SDK TEST" << std::endl;
  std::cout << std::string(60, '=') << std::endl;
  std::cout << "Device Serial: " << device_sn << std::endl;
  std::cout << "Target Motor ID: " << motor_id << std::endl;
  std::cout << "Test Duration: ~30 seconds" << std::endl;
  std::cout << "Press Ctrl+C to abort" << std::endl;
  std::cout << std::string(60, '=') << std::endl;
}

bool test_connection(const std::string &device_sn) {
  std::cout << "\n🔗 STEP 1: Testing USB2CAN Connection" << std::endl;
  std::cout << std::string(40, '-') << std::endl;

  controller = std::make_unique<ic_can::IC_CAN>(device_sn, true);

  std::cout << "📡 Connecting to device..." << std::endl;
  if (!controller->initialize()) {
    std::cout << "❌ FAILED: Could not initialize IC_CAN system" << std::endl;
    return false;
  }

  std::cout << "✅ SUCCESS: IC_CAN system initialized" << std::endl;

  // Print system status
  auto status = controller->get_system_status();
  std::cout << "\n📊 System Status:" << std::endl;
  for (const auto &[key, value] : status) {
    std::cout << "   " << key << ": " << value << std::endl;
  }

  return true;
}

bool test_motor_enable_disable(int motor_id) {
  std::cout << "\n⚡ STEP 2: Testing Motor Enable/Disable" << std::endl;
  std::cout << std::string(40, '-') << std::endl;

  std::cout << "🔋 Enabling all motors..." << std::endl;
  if (!controller->enable_all()) {
    std::cout << "❌ FAILED: Could not enable motors" << std::endl;
    return false;
  }
  std::cout << "✅ SUCCESS: Motors enabled" << std::endl;

  // Wait for motors to stabilize
  std::this_thread::sleep_for(std::chrono::seconds(2));

  // Read initial positions
  auto positions = controller->get_joint_positions();
  if (positions.empty()) {
    std::cout << "⚠️  WARNING: Could not read motor positions" << std::endl;
  } else {
    std::cout << "📍 Initial Motor " << motor_id << " Position: ";
    if (motor_id <= static_cast<int>(positions.size())) {
      std::cout << std::fixed << std::setprecision(3) << positions[motor_id - 1]
                << " rad" << std::endl;
    }
  }

  return true;
}

bool test_position_control(int motor_id) {
  std::cout << "\n🎯 STEP 3: Testing Position Control" << std::endl;
  std::cout << std::string(40, '-') << std::endl;

  // Test positions (small safe movements)
  std::vector<double> test_positions = {0.1, -0.1, 0.0}; // radians

  for (size_t i = 0; i < test_positions.size() && running; ++i) {
    double target_pos = test_positions[i];

    std::cout << "🎯 Moving motor " << motor_id
              << " to position: " << std::fixed << std::setprecision(3)
              << target_pos << " rad" << std::endl;

    // Create position command vector (9 motors, only target motor gets command)
    std::vector<double> positions(9, 0.0);
    positions[motor_id - 1] = target_pos;

    if (!controller->set_joint_positions(positions)) {
      std::cout << "❌ FAILED: Could not set position" << std::endl;
      return false;
    }

    std::cout << "✅ Command sent, waiting 3 seconds..." << std::endl;

    // Monitor position for 3 seconds
    for (int sec = 3; sec > 0 && running; --sec) {
      std::this_thread::sleep_for(std::chrono::seconds(1));

      auto current_pos = controller->get_joint_positions();
      if (!current_pos.empty() &&
          motor_id <= static_cast<int>(current_pos.size())) {
        double pos = current_pos[motor_id - 1];
        std::cout << "   Current position: " << std::fixed
                  << std::setprecision(3) << pos << " rad (remaining " << sec
                  << "s)" << std::endl;
      }
    }
  }

  return true;
}

bool test_realtime_feedback(int motor_id) {
  std::cout << "\n📡 STEP 4: Testing Real-time Feedback" << std::endl;
  std::cout << std::string(40, '-') << std::endl;

  std::cout << "🔄 Starting high-frequency control (500Hz)..." << std::endl;
  if (!controller->start_high_frequency_control()) {
    std::cout << "❌ FAILED: Could not start high-frequency control"
              << std::endl;
    return false;
  }
  std::cout << "✅ SUCCESS: High-frequency control started" << std::endl;

  // Monitor for 5 seconds
  std::cout << "📊 Monitoring real-time feedback for 5 seconds..." << std::endl;
  auto start_time = std::chrono::steady_clock::now();
  int update_count = 0;

  while (running && std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::steady_clock::now() - start_time)
                            .count() < 5) {

    auto positions = controller->get_joint_positions();
    auto velocities = controller->get_joint_velocities();

    if (!positions.empty() && motor_id <= static_cast<int>(positions.size())) {
      if (update_count % 10 == 0) { // Print every 10th update
        std::cout << "   Update " << std::setw(3) << update_count
                  << " | Pos: " << std::fixed << std::setprecision(3)
                  << positions[motor_id - 1] << " rad";

        if (!velocities.empty() &&
            motor_id <= static_cast<int>(velocities.size())) {
          std::cout << " | Vel: " << std::fixed << std::setprecision(3)
                    << velocities[motor_id - 1] << " rad/s";
        }
        std::cout << std::endl;
      }
      update_count++;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  std::cout << "✅ SUCCESS: Received " << update_count << " updates"
            << std::endl;

  controller->stop_high_frequency_control();
  std::cout << "🛑 High-frequency control stopped" << std::endl;

  return true;
}

bool test_motor_disable() {
  std::cout << "\n🔌 STEP 5: Testing Motor Disable" << std::endl;
  std::cout << std::string(40, '-') << std::endl;

  std::cout << "🔌 Disabling all motors..." << std::endl;
  if (!controller->disable_all()) {
    std::cout << "❌ FAILED: Could not disable motors" << std::endl;
    return false;
  }
  std::cout << "✅ SUCCESS: Motors disabled" << std::endl;

  return true;
}

void print_test_result(bool success) {
  std::cout << "\n" << std::string(60, '=') << std::endl;
  if (success) {
    std::cout << "🎉 TEST COMPLETED SUCCESSFULLY!" << std::endl;
    std::cout << "✅ dm-tools SDK integration is working" << std::endl;
    std::cout << "✅ USB2CAN connection is functional" << std::endl;
    std::cout << "✅ Motor control is operational" << std::endl;
  } else {
    std::cout << "❌ TEST FAILED!" << std::endl;
    std::cout << "⚠️  Check dm-tools SDK integration" << std::endl;
    std::cout << "⚠️  Verify USB2CAN device connection" << std::endl;
    std::cout << "⚠️  Check motor wiring and configuration" << std::endl;
  }
  std::cout << std::string(60, '=') << std::endl;
}

int main(int argc, char *argv[]) {
  // Parse arguments
  std::string device_sn = "F561E08C892274DB09496BCC1102DBC5";
  int motor_id = 4;

  if (argc > 1) {
    device_sn = argv[1];
  }
  if (argc > 2) {
    motor_id = std::stoi(argv[2]);
    if (motor_id < 1 || motor_id > 9) {
      std::cerr << "❌ Invalid motor ID: " << motor_id << ". Must be 1-9."
                << std::endl;
      return -1;
    }
  }

  print_test_header(device_sn, motor_id);

  // Set up signal handling
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  bool all_tests_passed = true;

  try {
    // Step 1: Test connection
    if (!test_connection(device_sn)) {
      all_tests_passed = false;
      goto cleanup;
    }

    // Step 2: Test motor enable/disable
    if (!test_motor_enable_disable(motor_id)) {
      all_tests_passed = false;
      goto cleanup;
    }

    // Step 3: Test position control
    if (!test_position_control(motor_id)) {
      all_tests_passed = false;
      goto cleanup;
    }

    // Step 4: Test real-time feedback
    if (!test_realtime_feedback(motor_id)) {
      all_tests_passed = false;
      goto cleanup;
    }

    // Step 5: Test motor disable
    if (!test_motor_disable()) {
      all_tests_passed = false;
    }

  } catch (const std::exception &e) {
    std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
    all_tests_passed = false;
  }

cleanup:
  // Clean up
  if (controller) {
    std::cout << "\n🧹 Cleaning up resources..." << std::endl;
    controller->disable_all();
    controller->stop_high_frequency_control();
  }

  print_test_result(all_tests_passed && running);

  return (all_tests_passed && running) ? 0 : -1;
}
