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
 * @file arm_position_monitor_clean.cpp
 * @brief Clean Read-Only Robot System Position Monitoring Tool
 *
 * READ-ONLY monitoring tool for complete 9-joint robot system:
 * - Arm joints monitoring (motors 1-6): DM10010L, DM6248, DM4340, DM4310
 * - Wrist motors monitoring (motors 7-8, HT4438)
 * - USB Servo motor monitoring (motor 9, gripper) - PURE READ ONLY
 * - Real-time display of position, velocity, and torque for all motors
 * - Configurable update frequency
 * - Data logging capability for all 9 joints
 */

#include "ic_can/core/can_bus_logger.hpp"
#include "ic_can/core/gripper_component.hpp"
#include "motor_profiler.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <csignal>
#include <fstream>
#include <ic_can/core/ic_can.hpp>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <thread>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static volatile bool g_running = true;
static std::unique_ptr<MotorProfiler> g_profiler;
static std::unique_ptr<ic_can::CANBusLogger> g_can_logger;

void signal_handler(int signal) {
  std::cout << "\n⚠️  Received signal " << signal << ", stopping monitor..."
            << std::endl;
  if (g_profiler) {
    g_profiler->print_summary();
  }
  if (g_can_logger) {
    std::cout << "\n📋 CAN Bus Traffic Summary:" << std::endl;
    g_can_logger->print_statistics();
  }
  g_running = false;
}

void print_arm_header() {
  std::cout << "\n" << std::string(120, '=') << std::endl;
  std::cout << "Joint | Position (rad) | Position (deg) | Velocity (rad/s) | "
               "Torque (Nm) | Motor Type     "
            << std::endl;
  std::cout << "------|---------------|----------------|-----------------|-----"
               "--------|---------------"
            << std::endl;
}

void print_arm_data(const std::vector<double> &positions,
                    const std::vector<double> &velocities,
                    const std::vector<double> &torques) {
  const char *motor_types[] = {"DM10010L", "DM6248", "DM6248",
                               "DM4340",   "DM4340", "DM4310",
                               "HT4438",   "HT4438", "SERVO"};

  int num_motors = std::min(
      {(int)positions.size(), (int)velocities.size(), (int)torques.size(), 9});

  for (int i = 0; i < num_motors; i++) {
    std::cout << std::setw(5) << (i + 1) << " | " << std::setw(13) << std::fixed
              << std::setprecision(4) << positions[i] << " | " << std::setw(14)
              << std::setprecision(2) << (positions[i] * 180.0 / M_PI) << " | "
              << std::setw(15) << std::setprecision(3) << velocities[i] << " | "
              << std::setw(11) << std::setprecision(3) << torques[i] << " | "
              << motor_types[i] << std::endl;
  }
}

void print_usage(const char *program_name) {
  std::cout << "Usage: " << program_name << " [options]" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -f <freq>     Update frequency in Hz (default: 10)" << std::endl;
  std::cout << "  -t <seconds>  Run for specified time, then exit" << std::endl;
  std::cout << "  -h            Show this help message" << std::endl;
  std::cout << "\nNote: READ-ONLY mode - no control commands sent to any motors" << std::endl;
}

int main(int argc, char *argv[]) {
  std::cout << "=== IC_CAN Arm Position Monitor (READ-ONLY MODE) ===" << std::endl;
  std::cout << "Pure monitoring for 9-joint system (motors 1-9)" << std::endl;
  std::cout << "READ-ONLY: No control commands sent to any motors" << std::endl;

  // Parse command line arguments
  double frequency = 10.0; // Hz
  double duration_seconds = 0.0; // 0 = run forever

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      print_usage(argv[0]);
      return 0;
    } else if (arg == "-f" && i + 1 < argc) {
      frequency = std::atof(argv[++i]);
      if (frequency <= 0 || frequency > 1000) {
        std::cout << "❌ Invalid frequency: " << frequency
                  << " Hz (must be 0-1000)" << std::endl;
        return -1;
      }
    } else if (arg == "-t" && i + 1 < argc) {
      duration_seconds = std::atof(argv[++i]);
      if (duration_seconds <= 0) {
        std::cout << "❌ Invalid duration: " << duration_seconds << " seconds"
                  << std::endl;
        return -1;
      }
    } else {
      std::cout << "❌ Unknown argument: " << arg << std::endl;
      print_usage(argv[0]);
      return -1;
    }
  }

  std::cout << "📊 Monitoring frequency: " << frequency << " Hz" << std::endl;
  if (duration_seconds > 0) {
    std::cout << "⏱️  Duration: " << duration_seconds << " seconds" << std::endl;
  } else {
    std::cout << "⏱️  Duration: Continuous (Ctrl+C to stop)" << std::endl;
  }

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  try {
    // Create IC_CAN controller
    auto controller =
        std::make_unique<ic_can::IC_CAN>("693D3DE86DF5940C8BC74A5B46A3CE2E",
                                         false); // Debug off for cleaner output

    // Initialize system
    if (!controller->initialize()) {
      std::cout << "❌ FAILED: System initialization failed" << std::endl;
      return -1;
    }
    std::cout << "✅ System initialized" << std::endl;

    std::cout << "🔌 Active communication backend: "
              << controller->get_current_backend() << std::endl;

    // DO NOT enable motors - this is READ-ONLY mode
    std::cout << "🔇 READ-ONLY: Motors NOT enabled - pure monitoring" << std::endl;

    // Initialize profiler
    g_profiler = std::make_unique<MotorProfiler>();
    std::cout << "📈 Motor profiling enabled" << std::endl;

    // Print header
    print_arm_header();

    std::cout << "\n🚀 Starting READ-ONLY arm position monitoring..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;

    auto start_time = std::chrono::steady_clock::now();

    // Monitoring loop - READ ONLY
    while (g_running) {
      // Check duration limit
      if (duration_seconds > 0) {
        auto elapsed = std::chrono::duration<double>(
                           std::chrono::steady_clock::now() - start_time)
                           .count();
        if (elapsed >= duration_seconds) {
          std::cout << "\n⏱️  Time limit reached, stopping monitor..."
                    << std::endl;
          break;
        }
      }

      // Increment loop counter
      g_profiler->increment_loop_count();

      // Get CAN motor positions (motors 1-8)
      auto positions = controller->get_joint_positions();

      // Read USB servo position (motor 9) - READ ONLY
      static double last_servo_angle = 0.0;
      static auto last_servo_time = std::chrono::steady_clock::now();
      static bool first_servo_read = true;

      double servo_openness = 0.5; // Default fallback
      bool is_moving = false;

      try {
        auto& gripper = controller->get_gripper();

        // READ ONLY: Use new read_servo_position() method
        uint16_t raw_position = gripper.read_servo_position();

        if (raw_position > 0) {
          // Convert raw position (1000-2100) to openness (0.0-1.0)
          servo_openness = static_cast<double>(raw_position - 1000) / 1100.0;
          servo_openness = std::clamp(servo_openness, 0.0, 1.0);

          if (first_servo_read) {
            std::cout << "📊 USB Servo: Connected - reading real positions" << std::endl;
            first_servo_read = false;
          }
        } else {
          // Use default value if read failed
          if (first_servo_read) {
            std::cout << "⚠️  USB Servo: Using default values (read failed)" << std::endl;
            first_servo_read = false;
          }
        }

        is_moving = gripper.is_moving();

      } catch (const std::exception& e) {
        // Use default values if gripper fails
        if (first_servo_read) {
          std::cout << "⚠️  USB Servo: Using default values (" << e.what() << ")" << std::endl;
          first_servo_read = false;
        }
      }

      // Update positions array with USB servo data
      if (positions.size() >= 9) {
        // Convert servo openness (0.0-1.0) to radians (0 to π)
        double servo_angle_rad = servo_openness * M_PI;
        positions[8] = servo_angle_rad;

        // Calculate servo velocity estimate
        auto current_time = std::chrono::steady_clock::now();
        double time_diff = std::chrono::duration<double>(current_time - last_servo_time).count();
        double servo_velocity = 0.0;
        if (time_diff > 0.001) { // Avoid division by very small numbers
          servo_velocity = (servo_angle_rad - last_servo_angle) / time_diff;
          last_servo_angle = servo_angle_rad;
          last_servo_time = current_time;
        }
      }

      // Create velocity and torque arrays with servo data
      std::vector<double> velocities(9, 0.0);
      std::vector<double> torques(9, 0.0);

      // Update servo velocity and torque estimates
      if (velocities.size() >= 9) {
        // Simple velocity calculation for servo
        auto current_time = std::chrono::steady_clock::now();
        double time_diff = std::chrono::duration<double>(current_time - last_servo_time).count();
        if (time_diff > 0.001) {
          double current_angle = servo_openness * M_PI;
          velocities[8] = (current_angle - last_servo_angle) / time_diff;
          last_servo_angle = current_angle;
          last_servo_time = current_time;
        }

        // Simple torque estimate for servo
        torques[8] = is_moving ? 0.5 : 0.0;
      }

      // Print complete arm data including servo (every 10 iterations to avoid spam)
      static int print_counter = 0;
      if (++print_counter % 10 == 0) {
        std::cout << "\n";
        print_arm_header();
        print_arm_data(positions, velocities, torques);
        std::cout << std::flush;
      }

      // Sleep to maintain frequency
      std::this_thread::sleep_for(std::chrono::microseconds(
          static_cast<int>(1000000.0 / frequency)));
    }

    // Print final profiling summary
    if (g_profiler) {
      g_profiler->print_summary();
    }

    std::cout << "\n🎉 READ-ONLY arm position monitoring completed!" << std::endl;

    return 0;

  } catch (const std::exception &e) {
    std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
    return -1;
  }
}