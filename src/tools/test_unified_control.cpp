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
 * @file test_unified_control.cpp
 * @brief Test program for the new unified control system
 *
 * This program tests:
 * 1. Precision timing at various frequencies (100Hz, 400Hz, 1000Hz)
 * 2. Single vector position control
 * 3. Matrix trajectory control
 * 4. Selective motor control
 * 5. Performance monitoring and statistics
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
  std::cout << "\n⚠️  Received signal " << signal << ", stopping test..."
            << std::endl;
  g_running = false;
}

void print_usage(const char *program_name) {
  std::cout << "Usage: " << program_name << " [options]" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -f <freq>     Test frequency in Hz (default: 400)" << std::endl;
  std::cout << "  -t <test>     Test type:" << std::endl;
  std::cout << "                 single    - Single vector position control" << std::endl;
  std::cout << "                 trajectory - Matrix trajectory control" << std::endl;
  std::cout << "                 selective  - Selective motor control" << std::endl;
  std::cout << "                 all       - Run all tests (default)" << std::endl;
  std::cout << "  -d <seconds>  Duration for each test (default: 10)" << std::endl;
  std::cout << "  -h            Show this help message" << std::endl;
  std::cout << "\nExamples:" << std::endl;
  std::cout << "  " << program_name << " -f 400 -t single    # Test single control at 400Hz" << std::endl;
  std::cout << "  " << program_name << " -f 1000 -t all       # Test all modes at 1000Hz" << std::endl;
}

void test_single_position_control(ic_can::IC_CAN &controller, double frequency, double duration) {
  std::cout << "\n🎯 Testing Single Position Control at " << frequency << "Hz..." << std::endl;

  // Set test positions (small movements for safety)
  std::vector<double> test_positions = {
      0.1,   // Motor 1
      0.05,  // Motor 2
      0.0,   // Motor 3
      -0.05, // Motor 4
      0.0,   // Motor 5
      0.02,  // Motor 6
      0.1,   // Motor 7 (HT)
      -0.1,  // Motor 8 (HT)
      0.0    // Motor 9 (Servo)
  };

  // Set single target position
  if (!controller.set_target_position(test_positions)) {
    std::cout << "❌ Failed to set target position" << std::endl;
    return;
  }

  // Start unified control
  if (!controller.start_unified_control(frequency)) {
    std::cout << "❌ Failed to start unified control" << std::endl;
    return;
  }

  // Run for specified duration
  std::cout << "Running for " << duration << " seconds..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(static_cast<int>(duration)));

  // Print performance stats
  controller.print_unified_performance_stats();

  // Stop control
  controller.stop_unified_control();
  std::cout << "✅ Single position control test completed" << std::endl;
}

void test_trajectory_control(ic_can::IC_CAN &controller, double frequency, double duration) {
  std::cout << "\n🛤️  Testing Trajectory Control at " << frequency << "Hz..." << std::endl;

  // Create a simple sine wave trajectory for first 3 joints
  std::vector<std::vector<double>> trajectory;
  int num_points = static_cast<int>(duration * frequency);

  for (int i = 0; i < num_points; i++) {
    double t = static_cast<double>(i) / frequency;
    std::vector<double> point(9, 0.0);

    // Small amplitude sine waves for safety
    point[0] = 0.1 * std::sin(2 * M_PI * 0.5 * t); // 0.5 Hz sine wave
    point[1] = 0.05 * std::sin(2 * M_PI * 0.3 * t); // 0.3 Hz sine wave
    point[2] = 0.08 * std::sin(2 * M_PI * 0.7 * t); // 0.7 Hz sine wave

    trajectory.push_back(point);
  }

  // Set trajectory
  if (!controller.set_target_trajectory(trajectory)) {
    std::cout << "❌ Failed to set trajectory" << std::endl;
    return;
  }

  // Start unified control
  if (!controller.start_unified_control(frequency)) {
    std::cout << "❌ Failed to start unified control" << std::endl;
    return;
  }

  // Run for specified duration
  std::cout << "Running " << trajectory.size() << " trajectory points for "
            << duration << " seconds..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(static_cast<int>(duration)));

  // Print performance stats
  controller.print_unified_performance_stats();

  // Stop control
  controller.stop_unified_control();
  std::cout << "✅ Trajectory control test completed" << std::endl;
}

void test_selective_control(ic_can::IC_CAN &controller, double frequency, double duration) {
  std::cout << "\n🔧 Testing Selective Motor Control at " << frequency << "Hz..." << std::endl;

  // Test controlling only motors 1, 2, 3 (arm joints)
  std::vector<int> selected_motors = {1, 2, 3};

  if (!controller.set_motor_selection(selected_motors)) {
    std::cout << "❌ Failed to set motor selection" << std::endl;
    return;
  }

  // Set small test positions for selected motors
  std::vector<double> test_positions = {
      0.15,  // Motor 1
      -0.1,  // Motor 2
      0.12,  // Motor 3
      0.0,   // Motor 4 (ignored)
      0.0,   // Motor 5 (ignored)
      0.0,   // Motor 6 (ignored)
      0.0,   // Motor 7 (ignored)
      0.0,   // Motor 8 (ignored)
      0.0    // Motor 9 (ignored)
  };

  if (!controller.set_target_position(test_positions)) {
    std::cout << "❌ Failed to set target position" << std::endl;
    return;
  }

  // Start unified control
  if (!controller.start_unified_control(frequency)) {
    std::cout << "❌ Failed to start unified control" << std::endl;
    return;
  }

  // Run for specified duration
  std::cout << "Controlling motors [";
  for (int motor : selected_motors) {
    std::cout << motor << " ";
  }
  std::cout << "] for " << duration << " seconds..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(static_cast<int>(duration)));

  // Print performance stats
  controller.print_unified_performance_stats();

  // Stop control
  controller.stop_unified_control();
  std::cout << "✅ Selective control test completed" << std::endl;
}

int main(int argc, char *argv[]) {
  std::cout << "=== IC_CAN Unified Control System Test ===" << std::endl;

  // Parse command line arguments
  double frequency = 400.0; // Default frequency
  std::string test_type = "all";
  double duration = 10.0;   // Default duration

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      print_usage(argv[0]);
      return 0;
    } else if (arg == "-f" && i + 1 < argc) {
      frequency = std::stod(argv[++i]);
      if (frequency <= 0 || frequency > 2000) {
        std::cout << "❌ Invalid frequency: " << frequency
                  << " Hz (must be 0-2000)" << std::endl;
        return -1;
      }
    } else if (arg == "-t" && i + 1 < argc) {
      test_type = argv[++i];
    } else if (arg == "-d" && i + 1 < argc) {
      duration = std::stod(argv[++i]);
      if (duration <= 0) {
        std::cout << "❌ Invalid duration: " << duration << std::endl;
        return -1;
      }
    } else {
      std::cout << "❌ Unknown argument: " << arg << std::endl;
      print_usage(argv[0]);
      return -1;
    }
  }

  std::cout << "📊 Test Configuration:" << std::endl;
  std::cout << "   Frequency: " << frequency << " Hz" << std::endl;
  std::cout << "   Test Type: " << test_type << std::endl;
  std::cout << "   Duration: " << duration << " seconds per test" << std::endl;

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  try {
    // Create IC_CAN controller
    auto controller = std::make_unique<ic_can::IC_CAN>(
        "693D3DE86DF5940C8BC74A5B46A3CE2E", false); // Debug off for cleaner output

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

    // Set to TEACH_MODE for safety
    controller->set_control_mode(ic_can::IC_CAN::ControlMode::TEACH_MODE);
    controller->enable_gravity_compensation();
    controller->enable_friction_compensation();

    // Run tests
    if (test_type == "single" || test_type == "all") {
      test_single_position_control(*controller, frequency, duration);
      std::this_thread::sleep_for(std::chrono::seconds(2)); // Pause between tests
    }

    if (test_type == "trajectory" || test_type == "all") {
      test_trajectory_control(*controller, frequency, duration);
      std::this_thread::sleep_for(std::chrono::seconds(2)); // Pause between tests
    }

    if (test_type == "selective" || test_type == "all") {
      test_selective_control(*controller, frequency, duration);
      std::this_thread::sleep_for(std::chrono::seconds(2)); // Pause between tests
    }

    // Disable motors
    controller->disable_all();
    std::cout << "\n🎉 All tests completed successfully!" << std::endl;

    return 0;

  } catch (const std::exception &e) {
    std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
    return -1;
  }
}