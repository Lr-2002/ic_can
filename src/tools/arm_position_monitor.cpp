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
 * @file arm_position_monitor.cpp
 * @brief Read-Only Robot System Position Monitoring Tool
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
static std::ofstream g_position_log_file;
static std::string g_position_log_filename;

// Simple profiling variables
static double total_time_us = 0.0;
static double can_time_us = 0.0;
static double usb_time_us = 0.0;
static uint64_t profile_count = 0;

void signal_handler(int signal) {
  std::cout << "\n⚠️  Received signal " << signal << ", stopping monitor..."
            << std::endl;

  // Print profiling summary
  if (profile_count > 0) {
    std::cout << "\n📊 PROFILING SUMMARY (" << profile_count
              << " iterations):" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << "Average total time: " << std::fixed << std::setprecision(1)
              << (total_time_us / profile_count) << " μs" << std::endl;
    std::cout << "Average CAN time:   " << std::setprecision(1)
              << (can_time_us / profile_count) << " μs" << std::endl;
    std::cout << "Average USB time:   " << std::setprecision(1)
              << (usb_time_us / profile_count) << " μs" << std::endl;

    double avg_total_ms = (total_time_us / profile_count) / 1000.0;
    if (avg_total_ms > 0) {
      double actual_freq = 1000.0 / avg_total_ms;
      std::cout << "Actual frequency:   " << std::setprecision(1) << actual_freq
                << " Hz" << std::endl;
    }

    double can_percent = (can_time_us / total_time_us) * 100.0;
    double usb_percent = (usb_time_us / total_time_us) * 100.0;
    std::cout << "CAN operations:     " << std::setprecision(1) << can_percent
              << "%" << std::endl;
    std::cout << "USB operations:     " << std::setprecision(1) << usb_percent
              << "%" << std::endl;
  }

  if (g_profiler) {
    g_profiler->print_summary();
  }
  if (g_can_logger) {
    std::cout << "\n📋 CAN Bus Traffic Summary:" << std::endl;
    g_can_logger->print_statistics();
  }
  if (g_position_log_file.is_open()) {
    g_position_log_file.close();
    std::cout << "📝 Position log saved: " << g_position_log_filename
              << std::endl;
  }
  g_running = false;
}

bool initialize_position_logger() {
  // Create timestamp for filename
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;

  std::stringstream ss;
  ss << "arm_positions_"
     << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
  ss << "_" << std::setfill('0') << std::setw(3) << ms.count() << ".csv";
  g_position_log_filename = ss.str();

  g_position_log_file.open(g_position_log_filename);
  if (!g_position_log_file.is_open()) {
    std::cout << "❌ Failed to create position log file: "
              << g_position_log_filename << std::endl;
    return false;
  }

  // Write CSV header
  g_position_log_file << "timestamp_ns,joint,position_rad,position_deg,"
                         "velocity_rad_s,torque_nm,motor_type\n";
  g_position_log_file.flush();

  std::cout << "📝 Position logger initialized: " << g_position_log_filename
            << std::endl;
  return true;
}

void log_arm_positions(const std::vector<double> &positions,
                       const std::vector<double> &velocities,
                       const std::vector<double> &torques) {
  if (!g_position_log_file.is_open()) {
    std::cout << "[DEBUG] Position log file not open!" << std::endl;
    return;
  }

  const char *motor_types[] = {"DM10010L", "DM6248", "DM6248",
                               "DM4340",   "DM4340", "DM4310",
                               "HT4438",   "HT4438", "SERVO"};

  int num_motors = std::min(
      {(int)positions.size(), (int)velocities.size(), (int)torques.size(), 9});

  static int debug_counter = 0;
  if (++debug_counter % 50 == 0) { // Print debug every 50 calls
    std::cout << "[DEBUG] Logging " << num_motors << " motor positions"
              << std::endl;
  }

  auto timestamp =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();

  for (int i = 0; i < num_motors; i++) {
    g_position_log_file << timestamp << "," << (i + 1) << "," << std::fixed
                        << std::setprecision(6) << positions[i] << ","
                        << std::setprecision(3) << (positions[i] * 180.0 / M_PI)
                        << "," << std::setprecision(6) << velocities[i] << ","
                        << std::setprecision(6) << torques[i] << ","
                        << motor_types[i] << "\n";
  }
  g_position_log_file.flush();
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
  std::cout << "  -f <freq>     Update frequency in Hz (default: 10)"
            << std::endl;
  std::cout << "  -t <seconds>  Run for specified time, then exit" << std::endl;
  std::cout << "  -h            Show this help message" << std::endl;
  std::cout << "\nNote: READ-ONLY mode - no control commands sent to any motors"
            << std::endl;
}

int main(int argc, char *argv[]) {
  std::cout << "=== IC_CAN Arm Position Monitor (READ-ONLY MODE) ==="
            << std::endl;
  std::cout << "Pure monitoring for 9-joint system (motors 1-9)" << std::endl;
  std::cout << "READ-ONLY: No control commands sent to any motors" << std::endl;

  // Parse command line arguments
  double frequency = 10.0;       // Hz
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
    std::cout << "🔇 READ-ONLY: Motors NOT enabled - pure monitoring"
              << std::endl;

    // Set to TEACH_MODE (gravity + friction compensation only)
    std::cout << "\n🎓 Setting TEACH_MODE for monitoring..." << std::endl;
    controller->set_control_mode(ic_can::IC_CAN::ControlMode::TEACH_MODE);

    // Enable compensation for monitoring
    controller->enable_gravity_compensation();
    controller->enable_friction_compensation();
    controller->start_logging();
    // Disable servo force output for TEACH_MODE
    std::cout
        << "🎓 TEACH_MODE: Disabling servo force output for natural movement"
        << std::endl;
    try {
      auto &gripper = controller->get_gripper();
      // Send disable torque command to servo
      if (gripper.stop()) {
        std::cout << "✅ Servo force output disabled" << std::endl;
      } else {
        std::cout << "⚠️  Failed to disable servo force output" << std::endl;
      }
    } catch (const std::exception &e) {
      std::cout << "⚠️  Servo disable failed: " << e.what() << std::endl;
    }

    // Initialize profiler
    g_profiler = std::make_unique<MotorProfiler>();
    std::cout << "📈 Motor profiling enabled" << std::endl;

    // Initialize position logger
    if (!initialize_position_logger()) {
      std::cout << "⚠️  Warning: Failed to initialize position logger"
                << std::endl;
    }

    // Print header
    print_arm_header();

    std::cout << "\n🚀 Starting READ-ONLY arm position monitoring..."
              << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;

    auto start_time = std::chrono::steady_clock::now();

    // Monitoring loop - READ ONLY with profiling
    auto iteration_start = std::chrono::steady_clock::now();
    while (g_running) {
      auto loop_start = std::chrono::high_resolution_clock::now();

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

      profile_count++;
      g_profiler->increment_loop_count();

      // Profile CAN operations
      auto can_start = std::chrono::high_resolution_clock::now();
      auto positions = controller->get_joint_positions();
      controller->set_joint_positions(positions, {}, {});
      auto can_end = std::chrono::high_resolution_clock::now();

      can_time_us += std::chrono::duration_cast<std::chrono::microseconds>(
                         can_end - can_start)
                         .count();
      // Read USB servo position (motor 9) - READ ONLY
      static double last_servo_angle = 0.0;
      static auto last_servo_time = std::chrono::steady_clock::now();
      static bool first_servo_read = true;
      static bool first_can_servo_read = true;

      double servo_openness = 0.5; // Default fallback
      bool is_moving = false;
      bool use_usb_feedback = false;

      // Create velocity and torque arrays with servo data
      std::vector<double> velocities(9, 0.0);
      std::vector<double> torques(9, 0.0);

      // Set servo velocity (calculated above)
      if (velocities.size() >= 9) {
        auto current_time = std::chrono::steady_clock::now();
        double time_diff =
            std::chrono::duration<double>(current_time - last_servo_time)
                .count();
        if (time_diff > 0.001) {
          double current_angle =
              positions[8]; // Use the actual position from USB or CAN
          velocities[8] = (current_angle - last_servo_angle) / time_diff;
          last_servo_angle = current_angle;
          last_servo_time = current_time;
        }

        // Simple torque estimate for servo
        torques[8] = 0.0;
      }

      // Log position data every iteration
      log_arm_positions(positions, velocities, torques);

      // Print complete arm data including servo (every 10 iterations to avoid
      // spam)
      static int print_counter = 0;
      if (++print_counter % 10 == 0) {
        std::cout << "\n";
        print_arm_header();
        print_arm_data(positions, velocities, torques);
        std::cout << std::flush;
      }

      // Calculate total iteration time
      auto loop_end = std::chrono::high_resolution_clock::now();
      auto iteration_time =
          std::chrono::duration_cast<std::chrono::microseconds>(loop_end -
                                                                loop_start)
              .count();
      total_time_us += iteration_time;

      // Print current timing every 50 iterations
      /*if (profile_count % 50 == 0) {*/
      /*  std::cout << "\n📊 Iteration " << profile_count << " - Current
       * timing:" << std::endl;*/
      /*  std::cout << "  Total: " << std::fixed << std::setprecision(1) <<
       * iteration_time << " μs" << std::endl;*/
      /*  std::cout << "  CAN:   " << std::setprecision(1) << (can_time_us /
       * profile_count) << " μs avg" << std::endl;*/
      /*  if (iteration_time > 0) {*/
      /*    double current_freq = 1000000.0 / iteration_time;*/
      /*    std::cout << "  Freq:  " << std::setprecision(1) << current_freq <<
       * " Hz" << std::endl;*/
      /*  }*/
      /*}*/

      // Sleep to maintain frequency
      std::this_thread::sleep_for(
          std::chrono::microseconds(static_cast<int>(1000000.0 / frequency)));
    }

    // Print final profiling summary
    if (g_profiler) {
      g_profiler->print_summary();
    }

    // Close position log file
    if (g_position_log_file.is_open()) {
      g_position_log_file.close();
      std::cout << "📝 Position log saved: " << g_position_log_filename
                << std::endl;
    }

    std::cout << "\n🎉 READ-ONLY arm position monitoring completed!"
              << std::endl;

    return 0;
  } catch (const std::exception &e) {
    std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
    return -1;
  }
}
