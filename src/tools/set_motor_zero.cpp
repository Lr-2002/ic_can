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
 * @file set_motor_zero.cpp
 * @brief Motor Zero Setting Tool
 *
 * Interactive tool to:
 * 1. Display all motor positions
 * 2. Allow user to select a motor ID
 * 3. Set selected motor to zero position
 * 4. Monitor the result
 */

#include <chrono>
#include <cmath>
#include <ic_can/core/ic_can.hpp>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void print_all_positions(ic_can::IC_CAN& controller) {
  auto positions = controller.get_joint_positions();

  std::cout << "\n📊 Current Motor Positions:" << std::endl;
  std::cout << std::string(70, '=') << std::endl;
  std::cout << "Motor | Position (rad) | Position (deg) | Type      | Status" << std::endl;
  std::cout << "------|---------------|----------------|-----------|--------" << std::endl;

  for (int i = 0; i < 9; i++) {
    std::string motor_type;
    if (i < 6) {
      motor_type = "Damiao";
    } else if (i < 8) {
      motor_type = "HT";
    } else {
      motor_type = "Servo";
    }

    std::string status;
    double pos = positions[i];
    if (std::abs(pos) < 0.01) {
      status = "ZERO";
    } else if (std::abs(pos) < 0.1) {
      status = "NEAR ZERO";
    } else {
      status = "OFFSET";
    }

    std::cout << std::setw(5) << (i + 1) << " | "
              << std::setw(13) << std::fixed << std::setprecision(4) << pos << " | "
              << std::setw(14) << std::setprecision(2) << (pos * 180.0 / M_PI) << " | "
              << std::setw(9) << motor_type << " | "
              << status << std::endl;
  }
  std::cout << std::string(70, '=') << std::endl;
}

void set_motor_to_zero(ic_can::IC_CAN& controller, int motor_id) {
  if (motor_id < 1 || motor_id > 9) {
    std::cout << "❌ Invalid motor ID: " << motor_id << " (must be 1-9)" << std::endl;
    return;
  }

  std::cout << "\n🎯 Setting Motor " << motor_id << " zero position calibration..." << std::endl;

  // Get current positions
  auto current_positions = controller.get_joint_positions();
  std::cout << "Current position: " << std::fixed << std::setprecision(4)
            << current_positions[motor_id - 1] << " rad ("
            << (current_positions[motor_id - 1] * 180.0 / M_PI) << "°)" << std::endl;

  std::cout << "⚠️  This will set the CURRENT position as the new ZERO calibration" << std::endl;
  std::cout << "📝 After calibration, this position will be reported as 0.0 rad" << std::endl;

  // Ask for confirmation
  std::cout << "\n🤔 Do you want to proceed? (y/N): ";
  std::string confirm;
  std::getline(std::cin, confirm);

  if (confirm != "y" && confirm != "Y" && confirm != "yes" && confirm != "YES") {
    std::cout << "❌ Cancelled zero calibration for Motor " << motor_id << std::endl;
    return;
  }

  // Send zero position calibration command using the new IC_CAN method
  bool success = controller.set_motor_zero_calibration(motor_id);

  if (success) {
    std::cout << "\n✅ Zero calibration command sent to Motor " << motor_id << std::endl;
    std::cout << "⏳ Waiting for motor to process calibration..." << std::endl;

    // Wait for motor to process
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Refresh positions to see the result
    controller.refresh_all();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Show the new position reading
    auto new_positions = controller.get_joint_positions();
    std::cout << "📊 New reading: " << std::fixed << std::setprecision(4)
              << new_positions[motor_id - 1] << " rad ("
              << (new_positions[motor_id - 1] * 180.0 / M_PI) << "°)" << std::endl;

    if (std::abs(new_positions[motor_id - 1]) < 0.1) {
      std::cout << "✅ SUCCESS: Motor " << motor_id << " zero calibration completed!" << std::endl;
    } else {
      std::cout << "⚠️  WARNING: Motor position may still need time to settle" << std::endl;
      std::cout << "💡 Try refreshing positions again in a few seconds" << std::endl;
    }
  } else {
    std::cout << "❌ Failed to send zero calibration command to Motor " << motor_id << std::endl;
  }

  std::cout << "\n📖 Note: Zero position calibration sets the current position as 0.0" << std::endl;
  std::cout << "   This is different from moving the motor to position 0.0" << std::endl;
  std::cout << "   After calibration, the motor will treat this position as its new origin" << std::endl;
}

void print_usage() {
  std::cout << "\n📖 Usage:" << std::endl;
  std::cout << "  - Type 'p' or 'pos' to display all positions" << std::endl;
  std::cout << "  - Type '1'-'9' to set that motor's ZERO CALIBRATION" << std::endl;
  std::cout << "  - Type 'a' or 'all' to set all motors to zero position (move)" << std::endl;
  std::cout << "  - Type 'r' or 'refresh' to refresh positions" << std::endl;
  std::cout << "  - Type 'q' or 'quit' to exit" << std::endl;
  std::cout << "  - Type 'h' or 'help' to show this help" << std::endl;
  std::cout << "\n📌 NOTE: Setting zero calibration means setting CURRENT position as 0.0" << std::endl;
  std::cout << "   This is NOT moving the motor to position 0.0!" << std::endl;
}

int main() {
  std::cout << "=== IC_CAN Motor Zero Setting Tool ===" << std::endl;
  std::cout << "Interactive tool to set individual motors to zero position" << std::endl;

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

    // Show initial positions
    print_all_positions(*controller);
    print_usage();

    // Interactive loop
    std::string input;
    while (true) {
      std::cout << "\n> ";
      std::getline(std::cin, input);

      // Convert to lowercase
      for (char& c : input) {
        c = std::tolower(c);
      }

      if (input == "q" || input == "quit") {
        std::cout << "👋 Goodbye!" << std::endl;
        break;
      } else if (input == "p" || input == "pos") {
        print_all_positions(*controller);
      } else if (input == "r" || input == "refresh") {
        controller->refresh_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        print_all_positions(*controller);
      } else if (input == "a" || input == "all") {
        std::cout << "\n🎯 Setting ALL motors to zero position..." << std::endl;
        controller->set_zero_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        print_all_positions(*controller);
      } else if (input == "h" || input == "help") {
        print_usage();
      } else if (input.length() == 1 && input[0] >= '1' && input[0] <= '9') {
        int motor_id = input[0] - '0';
        set_motor_to_zero(*controller, motor_id);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        print_all_positions(*controller);
      } else if (!input.empty()) {
        std::cout << "❌ Unknown command: " << input << std::endl;
        print_usage();
      }
    }

    // Disable motors
    controller->disable_all();
    std::cout << "\n✅ Motors disabled" << std::endl;

  } catch (const std::exception &e) {
    std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
    return -1;
  }

  return 0;
}