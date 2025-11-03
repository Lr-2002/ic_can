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
 * @file demo_unified_control_features.cpp
 * @brief Demo program showing advanced unified control features
 *
 * This program demonstrates:
 * 1. Starting control at a frequency, then updating positions
 * 2. Dynamic position updates while control is running
 * 3. Changing frequency during trajectory execution
 * 4. Real-time performance monitoring
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
  std::cout << "\n⚠️  Received signal " << signal << ", stopping demo..."
            << std::endl;
  g_running = false;
}

void print_usage(const char *program_name) {
  std::cout << "Usage: " << program_name << " [options]" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -h            Show this help message" << std::endl;
  std::cout << "\nDemo shows:" << std::endl;
  std::cout << "1. Start control at 100Hz" << std::endl;
  std::cout << "2. Update positions dynamically" << std::endl;
  std::cout << "3. Change frequency to 400Hz" << std::endl;
  std::cout << "4. Change frequency to 1000Hz" << std::endl;
  std::cout << "5. Dynamic trajectory execution" << std::endl;
}

void demo_frequency_and_position_updates(ic_can::IC_CAN &controller) {
  std::cout << "\n🚀 Demo 1: Frequency + Position Updates" << std::endl;
  std::cout << std::string(50, '=') << std::endl;

  // Start with initial positions at 100Hz
  std::vector<double> initial_positions = {
      0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
  };

  std::cout << "1️⃣ Starting control at 100Hz with zero positions..." << std::endl;
  controller.set_target_position(initial_positions);
  controller.start_unified_control(100.0);

  std::this_thread::sleep_for(std::chrono::seconds(3));

  // Update positions while control is running
  std::vector<double> new_positions = {
      0.1, 0.05, -0.05, 0.02, 0.0, 0.03, 0.05, -0.03, 0.0
  };

  std::cout << "\n2️⃣ Updating positions dynamically (control still running)..." << std::endl;
  controller.set_target_position(new_positions);

  std::this_thread::sleep_for(std::chrono::seconds(3));

  // Change frequency to 400Hz
  std::cout << "\n3️⃣ Changing frequency to 400Hz (no restart)..." << std::endl;
  controller.change_unified_frequency(400.0);

  std::this_thread::sleep_for(std::chrono::seconds(3));

  // Change frequency to 1000Hz
  std::cout << "\n4️⃣ Changing frequency to 1000Hz (no restart)..." << std::endl;
  controller.change_unified_frequency(1000.0);

  std::this_thread::sleep_for(std::chrono::seconds(3));

  std::cout << "\n📊 Performance after frequency changes:" << std::endl;
  controller.print_unified_performance_stats();

  controller.stop_unified_control();
  std::cout << "✅ Demo 1 completed" << std::endl;
}

void demo_trajectory_with_frequency_changes(ic_can::IC_CAN &controller) {
  std::cout << "\n🛤️  Demo 2: Trajectory + Frequency Changes" << std::endl;
  std::cout << std::string(50, '=') << std::endl;

  // Create a longer trajectory (30 seconds at 200Hz = 6000 points)
  std::vector<std::vector<double>> trajectory;
  int num_points = 6000; // 30 seconds at 200Hz
  double base_frequency = 200.0;

  for (int i = 0; i < num_points; i++) {
    double t = static_cast<double>(i) / base_frequency;
    std::vector<double> point(9, 0.0);

    // Different frequencies for different joints
    point[0] = 0.15 * std::sin(2 * M_PI * 0.2 * t); // 0.2 Hz - slow
    point[1] = 0.10 * std::sin(2 * M_PI * 0.5 * t); // 0.5 Hz - medium
    point[2] = 0.08 * std::sin(2 * M_PI * 1.0 * t); // 1.0 Hz - fast

    trajectory.push_back(point);
  }

  std::cout << "1️⃣ Starting trajectory at 200Hz (" << trajectory.size() << " points)..." << std::endl;
  controller.set_target_trajectory(trajectory);
  controller.start_unified_control(200.0);

  // Let it run for 10 seconds
  std::this_thread::sleep_for(std::chrono::seconds(10));

  // Change frequency to 500Hz mid-trajectory
  std::cout << "\n2️⃣ Changing to 500Hz mid-trajectory (trajectory continues)..." << std::endl;
  controller.change_unified_frequency(500.0);

  // Let it run for 10 more seconds
  std::this_thread::sleep_for(std::chrono::seconds(10));

  // Change frequency to 1000Hz
  std::cout << "\n3️⃣ Changing to 1000Hz mid-trajectory..." << std::endl;
  controller.change_unified_frequency(1000.0);

  // Let it run until trajectory completes
  std::this_thread::sleep_for(std::chrono::seconds(15));

  std::cout << "\n📊 Final performance:" << std::endl;
  controller.print_unified_performance_stats();

  controller.stop_unified_control();
  std::cout << "✅ Demo 2 completed" << std::endl;
}

void demo_real_time_updates(ic_can::IC_CAN &controller) {
  std::cout << "\n⚡ Demo 3: Real-time Position Updates" << std::endl;
  std::cout << std::string(50, '=') << std::endl;

  std::cout << "1️⃣ Starting control at 800Hz..." << std::endl;
  controller.start_unified_control(800.0);

  // Update positions every 2 seconds with different patterns
  std::vector<std::vector<double>> position_patterns = {
      {0.1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},      // Motor 1 only
      {0.0, 0.1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},      // Motor 2 only
      {0.0, 0.0, 0.1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},      // Motor 3 only
      {0.05, 0.05, 0.05, 0.05, 0.05, 0.05, 0.0, 0.0, 0.0}, // First 6 motors
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.1, 0.1, 0.0},    // Wrist motors
      {0.02, -0.02, 0.02, -0.02, 0.02, -0.02, 0.05, -0.05, 0.01} // All motors
  };

  for (size_t i = 0; i < position_patterns.size() && g_running; i++) {
    std::cout << "\n2️⃣ Updating to pattern " << (i + 1) << ": ";
    for (int j = 0; j < 9; j++) {
      if (position_patterns[i][j] != 0.0) {
        std::cout << "M" << (j + 1) << "=" << std::fixed << std::setprecision(3)
                  << position_patterns[i][j] << " ";
      }
    }
    std::cout << std::endl;

    controller.set_target_position(position_patterns[i]);

    if (g_running) {
      std::this_thread::sleep_for(std::chrono::seconds(3));
    }
  }

  std::cout << "\n📊 Performance with real-time updates:" << std::endl;
  controller.print_unified_performance_stats();

  controller.stop_unified_control();
  std::cout << "✅ Demo 3 completed" << std::endl;
}

int main(int argc, char *argv[]) {
  std::cout << "=== IC_CAN Unified Control Advanced Features Demo ===" << std::endl;

  if (argc > 1 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
    print_usage(argv[0]);
    return 0;
  }

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

    // Run demos
    if (g_running) {
      demo_frequency_and_position_updates(*controller);
      std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    if (g_running) {
      demo_trajectory_with_frequency_changes(*controller);
      std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    if (g_running) {
      demo_real_time_updates(*controller);
      std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    // Disable motors
    controller->disable_all();
    std::cout << "\n🎉 All demos completed successfully!" << std::endl;

    return 0;

  } catch (const std::exception &e) {
    std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
    return -1;
  }
}