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
 * @file home_to_zero.cpp
 * @brief Simple Home to Zero Tool
 *
 * Simple tool to move all motors from current positions to zero position:
 * - Read current positions
 * - Move all motors smoothly to zero using interpolation
 * - Monitor progress and completion
 * - Simple command-line interface
 */

#include <chrono>
#include <cmath>
#include <csignal>
#include <ic_can/core/ic_can.hpp>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static volatile bool g_running = true;

void signal_handler(int signal) {
  std::cout << "\n⚠️  Received signal " << signal << ", stopping..."
            << std::endl;
  g_running = false;
}

void print_positions(const std::vector<double> &positions,
                     const std::string &title) {
  std::cout << "\n" << title << std::endl;
  std::cout << std::string(80, '-') << std::endl;
  std::cout << "Motor | Position (rad) | Position (deg) | Status" << std::endl;
  std::cout << "------|---------------|----------------|--------" << std::endl;

  for (int i = 0; i < 6; i++) { // Only show first 6 (arm motors)
    std::string status;
    double pos = positions[i];
    if (std::abs(pos) < 0.01) {
      status = "ZERO";
    } else if (std::abs(pos) < 0.1) {
      status = "NEAR ZERO";
    } else {
      status = "OFFSET";
    }

    std::cout << std::setw(5) << (i + 1) << " | " << std::setw(13) << std::fixed
              << std::setprecision(4) << pos << " | " << std::setw(14)
              << std::setprecision(2) << (pos * 180.0 / M_PI) << " | " << status
              << std::endl;
  }
  std::cout << std::string(80, '-') << std::endl;
}

void print_usage(const char *program_name) {
  std::cout << "Usage: " << program_name << " [options]" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout
      << "  -v <velocity>  Maximum velocity for movement (rad/s, default: 0.5)"
      << std::endl;
  std::cout << "  -f <freq>      Control frequency in Hz (default: 100)"
            << std::endl;
  std::cout << "  -h             Show this help message" << std::endl;
  std::cout << "\nExample:" << std::endl;
  std::cout << "  " << program_name << "           # Default settings"
            << std::endl;
  std::cout << "  " << program_name << " -v 0.2    # Slower movement"
            << std::endl;
  std::cout << "  " << program_name
            << " -v 1.0 -f 200  # Fast movement at 200Hz" << std::endl;
}

int main(int argc, char *argv[]) {
  std::cout << "=== IC_CAN Home to Zero Tool ===" << std::endl;
  std::cout << "Move all motors smoothly from current positions to zero"
            << std::endl;

  // Parse command line arguments
  double max_velocity = 0.5; // rad/s
  double frequency = 100.0;  // Hz

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      print_usage(argv[0]);
      return 0;
    } else if (arg == "-v" && i + 1 < argc) {
      max_velocity = std::atof(argv[++i]);
      if (max_velocity <= 0 || max_velocity > 10.0) {
        std::cout << "❌ Invalid velocity: " << max_velocity
                  << " rad/s (must be 0-10)" << std::endl;
        return -1;
      }
    } else if (arg == "-f" && i + 1 < argc) {
      frequency = std::atof(argv[++i]);
      if (frequency <= 0 || frequency > 1000) {
        std::cout << "❌ Invalid frequency: " << frequency
                  << " Hz (must be 0-1000)" << std::endl;
        return -1;
      }
    } else {
      std::cout << "❌ Unknown argument: " << arg << std::endl;
      print_usage(argv[0]);
      return -1;
    }
  }

  std::cout << "📊 Configuration:" << std::endl;
  std::cout << "   Max velocity: " << max_velocity << " rad/s" << std::endl;
  std::cout << "   Control frequency: " << frequency << " Hz" << std::endl;
  std::cout << "   Movement time estimate: ~" << (1.0 / max_velocity)
            << " seconds per radian" << std::endl;

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

    // Read initial positions
    controller->refresh_all();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    auto initial_positions = controller->get_joint_positions();

    // Show initial positions
    print_positions(initial_positions,
                    "📍 Initial Positions (Arm Motors 1-6):");

    // Calculate maximum distance to zero
    double max_distance = 0.0;
    for (int i = 0; i < 6; i++) {
      max_distance = std::max(max_distance, std::abs(initial_positions[i]));
    }

    if (max_distance < 0.01) {
      std::cout << "\n✅ All motors are already at zero position!" << std::endl;
      controller->disable_all();
      return 0;
    }

    // Estimate time to complete
    double estimated_time = max_distance / max_velocity;
    std::cout << "\n🕐 Estimated time to zero: " << std::fixed
              << std::setprecision(1) << estimated_time << " seconds"
              << std::endl;

    // Set motor gains if they are zero
    for (int i = 1; i <= 6; i++) {
      double kp, kd;
      controller->get_motor_gains(i, kp, kd);
      std::cout << "motor " << i << " " << kp << " " << kd << std::endl;
      if (kp == 0.0) {
        std::cout << "⚠️  Motor " << i << " has kp=0, setting temporary gains..."
                  << std::endl;
        controller->set_motor_gains(i, 20.0, 0.5);
      }
    }

    // Ask for confirmation
    std::cout
        << "\n🤔 Do you want to proceed moving all motors to zero? (y/N): ";
    std::string confirm;
    std::getline(std::cin, confirm);

    if (confirm != "y" && confirm != "Y" && confirm != "yes" &&
        confirm != "YES") {
      std::cout << "❌ Cancelled operation" << std::endl;
      controller->disable_all();
      return 0;
    }

    std::cout << "\n🚀 Starting movement to zero position..." << std::endl;
    std::cout << "Press Ctrl+C to stop early" << std::endl;

    // Set target positions to zero with interpolation BEFORE starting control loop
    std::vector<double> target_positions(9, 0.0);
    std::cout << "🎯 Setting target positions to zero..." << std::endl;
    std::cout << "🎯 Target vector size: " << target_positions.size() << std::endl;
    controller->set_target_positions_interpolated(target_positions, max_velocity);
    std::cout << "🎯 Target positions set, about to start control loop..." << std::endl;

    // Start control loop AFTER setting targets
    std::cout << "🔄 Starting control loop..." << std::endl;
    if (!controller->start_control_loop(frequency)) {
      std::cout << "❌ Failed to start control loop" << std::endl;
      controller->disable_all();
      return -1;
    }

    // Monitor progress
    auto start_time = std::chrono::steady_clock::now();
    bool all_at_zero = false;
    int consecutive_zero_checks = 0;

    while (g_running && !all_at_zero) {
      auto current_time = std::chrono::steady_clock::now();
      auto elapsed =
          std::chrono::duration<double>(current_time - start_time).count();

      // Get current positions
      auto current_positions = controller->get_joint_positions();

      // Check if all arm motors are at zero
      bool all_zero = true;
      double max_error = 0.0;
      for (int i = 0; i < 6; i++) {
        double error = std::abs(current_positions[i]);
        max_error = std::max(max_error, error);
        if (error > 0.01) { // 0.01 rad tolerance
          all_zero = false;
        }
      }

      if (all_zero) {
        consecutive_zero_checks++;
        if (consecutive_zero_checks >= 3) { // 3 consecutive checks
          all_at_zero = true;
        }
      } else {
        consecutive_zero_checks = 0;
      }

      // Print progress every second
      static int last_progress_time = -1;
      int current_time_int = static_cast<int>(elapsed);
      if (current_time_int != last_progress_time) {
        std::cout << "⏱️  Time: " << std::setw(3) << current_time_int << "s | "
                  << "Max error: " << std::fixed << std::setprecision(4)
                  << max_error << " rad | "
                  << "Status: " << (all_at_zero ? "✅ AT ZERO" : "🔄 MOVING")
                  << std::endl;
        last_progress_time = current_time_int;
      }

      // Check timeout (2x estimated time)
      if (elapsed > estimated_time * 2.0) {
        std::cout << "\n⚠️  WARNING: Movement taking longer than expected"
                  << std::endl;
        std::cout << "💡 Motors may be stuck or gains may be too low"
                  << std::endl;
        break;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Stop control loop
    controller->stop_control_loop();

    // Show final positions
    auto final_positions = controller->get_joint_positions();
    print_positions(final_positions, "📍 Final Positions (Arm Motors 1-6):");

    // Check success
    bool success = true;
    for (int i = 0; i < 6; i++) {
      if (std::abs(final_positions[i]) > 0.05) { // 0.05 rad tolerance
        success = false;
        break;
      }
    }

    if (success) {
      std::cout << "\n✅ SUCCESS: All arm motors moved to zero position!"
                << std::endl;
    } else {
      std::cout
          << "\n⚠️  WARNING: Some motors may not have reached zero position"
          << std::endl;
      std::cout << "💡 Consider checking motor gains or mechanical constraints"
                << std::endl;
    }

    // Disable motors
    controller->disable_all();
    std::cout << "\n🎉 Home to zero operation completed!" << std::endl;

    return success ? 0 : 1;

  } catch (const std::exception &e) {
    std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
    return -1;
  }
}
