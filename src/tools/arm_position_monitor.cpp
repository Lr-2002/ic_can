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
 * @brief Complete Robot System Position Monitoring Tool
 *
 * Real-time monitoring tool for complete 9-joint robot system:
 * - Arm joints monitoring (motors 1-6): DM10010L, DM6248, DM4340, DM4310
 * - Wrist motors monitoring (motors 7-8, HT4438)
 * - Servo motor monitoring (motor 9, gripper)
 * - Real-time display of position, velocity, and torque for all motors
 * - Configurable update frequency
 * - Data logging capability for all 9 joints
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <csignal>
#include <fstream>
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
  std::cout << "\n⚠️  Received signal " << signal << ", stopping monitor..."
            << std::endl;
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

void start_logging(ic_can::IC_CAN &controller) {
  std::cout << "📝 Starting comprehensive data logging..." << std::endl;

  // Create log directory with timestamp
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto tm = *std::localtime(&time_t);

  char log_dir[100];
  strftime(log_dir, sizeof(log_dir), "arm_monitor_%Y%m%d_%H%M%S", &tm);

  // Start the comprehensive logging system
  if (controller.start_logging(std::string(log_dir))) {
    std::cout << "📁 Logging directory created: " << log_dir << std::endl;
    std::cout << "📄 Files being created:" << std::endl;
    std::cout
        << "   - motor_states.csv (actual motor positions/velocities/torques)"
        << std::endl;
    std::cout << "   - joint_commands.csv (commands sent to motors)"
              << std::endl;
  } else {
    std::cout << "❌ Failed to start logging system" << std::endl;
  }
}

void print_usage(const char *program_name) {
  std::cout << "Usage: " << program_name << " [options]" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -f <freq>     Update frequency in Hz (default: 10)"
            << std::endl;
  std::cout << "  -l            Enable comprehensive data logging to "
               "timestamped directory"
            << std::endl;
  std::cout << "  -t <seconds>  Run for specified time, then exit" << std::endl;
  std::cout << "  -h            Show this help message" << std::endl;
  std::cout << "\nExample:" << std::endl;
  std::cout << "  " << program_name
            << " -f 20 -l     # Monitor at 20Hz with comprehensive logging"
            << std::endl;
  std::cout << "  " << program_name
            << " -t 30        # Monitor for 30 seconds only" << std::endl;
  std::cout << "\nLogging creates timestamped directory with:" << std::endl;
  std::cout
      << "  - motor_states.csv (actual positions/velocities/torques at 400Hz)"
      << std::endl;
  std::cout << "  - joint_commands.csv (commands sent to motors)" << std::endl;
}

int main(int argc, char *argv[]) {
  std::cout << "=== IC_CAN Arm Position Monitor (TEACH_MODE) ===" << std::endl;
  std::cout << "Real-time monitoring for 9-joint system (motors 1-9)"
            << std::endl;
  std::cout << "TEACH_MODE: Gravity + friction compensation only" << std::endl;

  // Parse command line arguments
  double frequency = 10.0; // Hz
  bool enable_logging = true;
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
    } else if (arg == "-l") {
      enable_logging = true;
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
  std::cout << "📝 Logging: " << (enable_logging ? "Enabled" : "Disabled")
            << std::endl;
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

    // Enable motors
    if (!controller->enable_all()) {
      std::cout << "⚠️  WARNING: Some motors failed to enable" << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Set to TEACH_MODE for monitoring (gravity + friction compensation only)
    std::cout << "\n🎓 Setting TEACH_MODE for monitoring..." << std::endl;
    controller->set_control_mode(ic_can::IC_CAN::ControlMode::TEACH_MODE);

    // Enable compensation for monitoring
    controller->enable_gravity_compensation();
    controller->enable_friction_compensation();

    // Start logging if requested
    if (enable_logging) {
      start_logging(*controller);
    }

    std::cout << "\n🚀 Starting arm position monitoring..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;

    // Print header
    print_arm_header();

    // Monitoring loop
    auto start_time = std::chrono::steady_clock::now();
    auto period = std::chrono::duration<double>(1.0 / frequency);
    int update_count = 0;
    /*controller->load_friction_params_from_file(*/
    /*    "/home/lr-2002/project/InstantCreation/ic_can/"*/
    /*    "friction_calibrated_params.json");*/
    /*controller->enable_friction_compensation();*/
    while (g_running) {
      auto loop_start = std::chrono::steady_clock::now();

      // Check duration limit
      if (duration_seconds > 0) {
        auto elapsed =
            std::chrono::duration<double>(loop_start - start_time).count();
        if (elapsed >= duration_seconds) {
          std::cout << "\n⏱️  Time limit reached, stopping monitor..."
                    << std::endl;
          break;
        }
      }

      // Refresh motor data
      controller->refresh_all();
      std::this_thread::sleep_for(
          std::chrono::milliseconds(10)); // Small delay for response

      // Get all motor data (motors 1-9)
      auto positions = controller->get_joint_positions();
      auto velocities = controller->get_joint_velocities();
      auto torques = controller->get_joint_torques();
      std::vector<double> empty = {0, 0, 0, 0, 0, 0, 0, 0, 0};
      controller->set_joint_positions(positions, empty, empty);

      // Print data every second (to avoid spam)
      if (update_count % static_cast<int>(frequency) == 0) {
        print_arm_header();
        print_arm_data(positions, velocities, torques);
        std::cout << std::flush;
      }

      // Note: Logging is handled automatically by the IC_CAN logging system
      // when enabled, which logs at 400Hz in the background

      update_count++;

      // Maintain frequency
      auto elapsed = std::chrono::steady_clock::now() - loop_start;
      auto sleep_time = period - elapsed;
      if (sleep_time.count() > 0) {
        std::this_thread::sleep_for(sleep_time);
      }
    }

    // Stop logging if it was started
    if (enable_logging) {
      controller->stop_logging();
      std::cout << "\n📄 Comprehensive data logging completed" << std::endl;
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
