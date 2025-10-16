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
 * @brief Arm Position Monitoring Tool
 *
 * Real-time monitoring tool for 6-joint arm positions:
 * - Continuous position monitoring for arm joints (motors 1-6)
 * - Real-time display of position, velocity, and torque
 * - Configurable update frequency
 * - Data logging capability
 */

#include <chrono>
#include <cmath>
#include <csignal>
#include <ic_can/core/ic_can.hpp>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static volatile bool g_running = true;
static std::ofstream g_log_file;

void signal_handler(int signal) {
  std::cout << "\n⚠️  Received signal " << signal << ", stopping monitor..." << std::endl;
  g_running = false;
}

void print_arm_header() {
  std::cout << "\n" << std::string(100, '=') << std::endl;
  std::cout << "Joint | Position (rad) | Position (deg) | Velocity (rad/s) | Torque (Nm) | Motor Type" << std::endl;
  std::cout << "------|---------------|----------------|-----------------|-------------|------------" << std::endl;
}

void print_arm_data(const std::vector<double>& positions,
                   const std::vector<double>& velocities,
                   const std::vector<double>& torques) {
  const char* motor_types[] = {"DM10010L", "DM6248", "DM6248", "DM4340", "DM4340", "DM4310"};

  for (int i = 0; i < 6; i++) {
    std::cout << std::setw(5) << (i + 1) << " | "
              << std::setw(13) << std::fixed << std::setprecision(4) << positions[i] << " | "
              << std::setw(14) << std::setprecision(2) << (positions[i] * 180.0 / M_PI) << " | "
              << std::setw(15) << std::setprecision(3) << velocities[i] << " | "
              << std::setw(11) << std::setprecision(3) << torques[i] << " | "
              << motor_types[i] << std::endl;
  }
}

void log_arm_data(const std::vector<double>& positions,
                  const std::vector<double>& velocities,
                  const std::vector<double>& torques,
                  double timestamp) {
  if (g_log_file.is_open()) {
    g_log_file << std::fixed << std::setprecision(6) << timestamp << ",";
    for (int i = 0; i < 6; i++) {
      g_log_file << std::setprecision(6) << positions[i] << ","
                 << std::setprecision(6) << velocities[i] << ","
                 << std::setprecision(6) << torques[i];
      if (i < 5) g_log_file << ",";
    }
    g_log_file << std::endl;
  }
}

void start_logging() {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto tm = *std::localtime(&time_t);

  char filename[100];
  strftime(filename, sizeof(filename), "arm_data_%Y%m%d_%H%M%S.csv", &tm);

  g_log_file.open(filename);
  if (g_log_file.is_open()) {
    g_log_file << "timestamp,";
    for (int i = 1; i <= 6; i++) {
      g_log_file << "joint" << i << "_pos,joint" << i << "_vel,joint" << i << "_tau";
      if (i < 6) g_log_file << ",";
    }
    g_log_file << std::endl;

    std::cout << "📝 Logging data to: " << filename << std::endl;
  }
}

void print_usage(const char* program_name) {
  std::cout << "Usage: " << program_name << " [options]" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -f <freq>     Update frequency in Hz (default: 10)" << std::endl;
  std::cout << "  -l            Enable data logging to CSV file" << std::endl;
  std::cout << "  -t <seconds>  Run for specified time, then exit" << std::endl;
  std::cout << "  -h            Show this help message" << std::endl;
  std::cout << "\nExample:" << std::endl;
  std::cout << "  " << program_name << " -f 20 -l     # Monitor at 20Hz with logging" << std::endl;
  std::cout << "  " << program_name << " -t 30        # Monitor for 30 seconds only" << std::endl;
}

int main(int argc, char* argv[]) {
  std::cout << "=== IC_CAN Arm Position Monitor ===" << std::endl;
  std::cout << "Real-time monitoring for 6-joint arm (motors 1-6)" << std::endl;

  // Parse command line arguments
  double frequency = 10.0;  // Hz
  bool enable_logging = false;
  double duration_seconds = 0.0;  // 0 = run forever

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      print_usage(argv[0]);
      return 0;
    } else if (arg == "-f" && i + 1 < argc) {
      frequency = std::atof(argv[++i]);
      if (frequency <= 0 || frequency > 1000) {
        std::cout << "❌ Invalid frequency: " << frequency << " Hz (must be 0-1000)" << std::endl;
        return -1;
      }
    } else if (arg == "-l") {
      enable_logging = true;
    } else if (arg == "-t" && i + 1 < argc) {
      duration_seconds = std::atof(argv[++i]);
      if (duration_seconds <= 0) {
        std::cout << "❌ Invalid duration: " << duration_seconds << " seconds" << std::endl;
        return -1;
      }
    } else {
      std::cout << "❌ Unknown argument: " << arg << std::endl;
      print_usage(argv[0]);
      return -1;
    }
  }

  std::cout << "📊 Monitoring frequency: " << frequency << " Hz" << std::endl;
  std::cout << "📝 Logging: " << (enable_logging ? "Enabled" : "Disabled") << std::endl;
  if (duration_seconds > 0) {
    std::cout << "⏱️  Duration: " << duration_seconds << " seconds" << std::endl;
  } else {
    std::cout << "⏱️  Duration: Continuous (Ctrl+C to stop)" << std::endl;
  }

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  try {
    // Create IC_CAN controller
    auto controller = std::make_unique<ic_can::IC_CAN>(
        "F561E08C892274DB09496BCC1102DBC5", false);  // Debug off for cleaner output

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

    // Start logging if requested
    if (enable_logging) {
      start_logging();
    }

    std::cout << "\n🚀 Starting arm position monitoring..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;

    // Print header
    print_arm_header();

    // Monitoring loop
    auto start_time = std::chrono::steady_clock::now();
    auto period = std::chrono::duration<double>(1.0 / frequency);
    int update_count = 0;

    while (g_running) {
      auto loop_start = std::chrono::steady_clock::now();

      // Check duration limit
      if (duration_seconds > 0) {
        auto elapsed = std::chrono::duration<double>(loop_start - start_time).count();
        if (elapsed >= duration_seconds) {
          std::cout << "\n⏱️  Time limit reached, stopping monitor..." << std::endl;
          break;
        }
      }

      // Refresh motor data
      controller->refresh_all();
      std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Small delay for response

      // Get arm data (motors 1-6)
      auto positions = controller->get_joint_positions();
      auto velocities = controller->get_joint_velocities();
      auto torques = controller->get_joint_torques();

      // Truncate to first 6 motors (arm joints)
      std::vector<double> arm_positions(positions.begin(), positions.begin() + 6);
      std::vector<double> arm_velocities(velocities.begin(), velocities.begin() + 6);
      std::vector<double> arm_torques(torques.begin(), torques.begin() + 6);

      // Calculate timestamp
      double timestamp = std::chrono::duration<double>(loop_start - start_time).count();

      // Print data every second (to avoid spam)
      if (update_count % static_cast<int>(frequency) == 0) {
        print_arm_header();
        print_arm_data(arm_positions, arm_velocities, arm_torques);
        std::cout << std::flush;
      }

      // Log data at full frequency
      if (enable_logging) {
        log_arm_data(arm_positions, arm_velocities, arm_torques, timestamp);
      }

      update_count++;

      // Maintain frequency
      auto elapsed = std::chrono::steady_clock::now() - loop_start;
      auto sleep_time = period - elapsed;
      if (sleep_time.count() > 0) {
        std::this_thread::sleep_for(sleep_time);
      }
    }

    // Close log file
    if (g_log_file.is_open()) {
      g_log_file.close();
      std::cout << "\n📄 Data logging completed" << std::endl;
    }

    // Disable motors
    controller->disable_all();
    std::cout << "\n🎉 Arm position monitoring completed!" << std::endl;
    std::cout << "✅ Total updates: " << update_count << std::endl;

    return 0;

  } catch (const std::exception &e) {
    std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
    return -1;
  }
}