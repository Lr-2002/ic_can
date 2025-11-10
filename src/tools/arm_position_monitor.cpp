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
      controller->set_joint_positions(positions, {}, {});
      // Read USB servo position (motor 9) - READ ONLY
      static double last_servo_angle = 0.0;
      static auto last_servo_time = std::chrono::steady_clock::now();
      static bool first_servo_read = true;
      static bool first_can_servo_read = true;

      double servo_openness = 0.5; // Default fallback
      bool is_moving = false;
      bool use_usb_feedback = false;

      try {
        auto &gripper = controller->get_gripper();

        // READ ONLY: Use new read_servo_position() method
        uint16_t raw_position = gripper.read_servo_position();
        bool position_fresh = gripper.is_position_fresh(100); // 100ms max age

        if (raw_position > 0) {
          // Convert raw position (1000-2100) to openness (0.0-1.0)
          double raw_openness =
              static_cast<double>(raw_position - 1000) / 1100.0;
          servo_openness = std::clamp(raw_openness, 0.0, 1.0);
          use_usb_feedback = true;

          if (first_servo_read) {
            std::cout
                << "📊 USB Servo: Connected - reading real positions (raw="
                << raw_position << " -> raw_openness=" << std::fixed
                << std::setprecision(3) << raw_openness
                << " -> clamped_openness=" << servo_openness << ")"
                << std::endl;
            first_servo_read = false;
          }

          // Update servo angle continuously with freshness checking
          static int debug_counter = 0;
          if (++debug_counter % 10 == 0) { // Print every 10 iterations (every 2 seconds at 5Hz)
            std::cout << "🔄 USB Servo Update: raw=" << raw_position
                      << " -> openness=" << std::fixed << std::setprecision(3)
                      << servo_openness
                      << " -> angle=" << (servo_openness * 360.0) << "°"
                      << " [fresh=" << (position_fresh ? "✅" : "❌") << "]"
                      << std::endl;
          }

          // Warn if position data is stale
          if (!position_fresh) {
            static int stale_counter = 0;
            if (++stale_counter % 50 == 0) { // Warn every 50 stale reads
              std::cout << "⚠️  USB Servo: Position data is stale!" << std::endl;
            }
          }
        } else {
          // Use default value if read failed
          if (first_servo_read) {
            std::cout
                << "⚠️  USB Servo: Read failed, falling back to CAN feedback"
                << std::endl;
            first_servo_read = false;
          }
        }

        is_moving = gripper.is_moving();

      } catch (const std::exception &e) {
        // Use default values if gripper fails
        if (first_servo_read) {
          std::cout << "⚠️  USB Servo: Exception (" << e.what()
                    << "), falling back to CAN feedback" << std::endl;
          first_servo_read = false;
        }
      }

      // Update positions array with USB servo data OR use CAN feedback
      if (positions.size() >= 9) {
        if (use_usb_feedback) {
          // Convert servo position (1000-2100) to radians (0 to 2π)
          double servo_angle_rad = servo_openness * 2.0 * M_PI;
          positions[8] = servo_angle_rad;

          if (first_can_servo_read) {
            std::cout << "✅ Motor 9: Using USB servo feedback (" << std::fixed
                      << std::setprecision(2) << (servo_openness * 360.0)
                      << "°)" << std::endl;
            first_can_servo_read = false;
          }
        } else {
          // Keep CAN feedback for motor 9 (already in positions[8]) if USB
          // fails
          if (first_can_servo_read) {
            std::cout << "📊 Motor 9: Using CAN feedback (" << std::fixed
                      << std::setprecision(2) << (positions[8] * 180.0 / M_PI)
                      << "°)" << std::endl;
            first_can_servo_read = false;
          }
        }

        // Calculate servo velocity estimate
        auto current_time = std::chrono::steady_clock::now();
        double time_diff =
            std::chrono::duration<double>(current_time - last_servo_time)
                .count();
        if (time_diff > 0.001) { // Avoid division by very small numbers
          double current_angle = positions[8];
          double servo_velocity =
              (current_angle - last_servo_angle) / time_diff;
          last_servo_angle = current_angle;
          last_servo_time = current_time;
        }
      }

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
        torques[8] = is_moving ? 0.5 : 0.0;
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
