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

#include "motor_profiler.hpp"
#include "ic_can/core/can_bus_logger.hpp"
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

/**
 * @brief Parse CAN frame ID from feedback message
 * @param feedback_line The feedback line from console output
 * @return CAN ID if found, 0 otherwise
 */
uint32_t parse_can_id_from_feedback(const std::string &feedback_line) {
  // Look for patterns like:
  // "🔥 FEEDBACK: Received DM Motor 2 feedback, DLC=8 Data: 12 6f 43 7f f8 00
  // 1d 1a" "🔥 FEEDBACK: Received HT Motor 7 feedback, DLC=8 Data: 27 01 52 03
  // 03 00 ff ff"

  // Extract the hex data from the feedback line
  size_t data_pos = feedback_line.find("Data:");
  if (data_pos == std::string::npos) {
    return 0;
  }

  // Get the first hex byte which should be the CAN ID in our format
  size_t first_byte_pos = data_pos + 5; // Skip "Data:"
  while (first_byte_pos < feedback_line.length() &&
         (feedback_line[first_byte_pos] == ' ' ||
          feedback_line[first_byte_pos] == '\t')) {
    first_byte_pos++;
  }

  // Extract first hex byte (CAN ID)
  if (first_byte_pos + 1 < feedback_line.length()) {
    std::string hex_byte = feedback_line.substr(first_byte_pos, 2);
    try {
      uint32_t can_id = std::stoul(hex_byte, nullptr, 16);
      return can_id;
    } catch (...) {
      return 0;
    }
  }

  return 0;
}

/**
 * @brief Log CAN frame send operation
 */
void log_can_frame_send(uint32_t can_id, bool extended_id, const std::vector<uint8_t>& data) {
    if (g_can_logger && g_can_logger->is_logging()) {
        g_can_logger->log_sent_frame(can_id, extended_id, static_cast<uint8_t>(data.size()), data);
    }
}

/**
 * @brief Log CAN frame receive operation
 */
void log_can_frame_receive(uint32_t can_id, bool extended_id, const std::vector<uint8_t>& data) {
    if (g_can_logger && g_can_logger->is_logging()) {
        g_can_logger->log_received_frame(can_id, extended_id, static_cast<uint8_t>(data.size()), data);
    }

    // Also update motor profiler with received frame
    if (g_profiler) {
        g_profiler->record_can_frame(can_id);
    }
}

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
  std::cout << "\nCommunication Backend:" << std::endl;
  std::cout << "  🚀 ZLG CAN FD (64-byte frames, 5 Mbps) - DEFAULT"
            << std::endl;
  std::cout << "  🔄 DM Tools (8-byte frames, 1 Mbps) - Automatic fallback"
            << std::endl;
  std::cout << "  💻 Simulation - Testing without hardware" << std::endl;
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
                                         true); // Debug off for cleaner output

    // Initialize system
    if (!controller->initialize()) {
      std::cout << "❌ FAILED: System initialization failed" << std::endl;
      return -1;
    }
    std::cout << "✅ System initialized" << std::endl;

    // Show which backend is being used
    std::cout << "🔌 Active communication backend: "
              << controller->get_current_backend() << std::endl;

    // Show channel status if ZLG is being used
    if (controller->get_current_backend() == "zlg") {
      std::cout << "📊 ZLG CAN FD Configuration:" << std::endl;
      controller->print_channel_status();
    }

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

    // Initialize standalone motor profiler
    g_profiler = std::make_unique<MotorProfiler>();
    std::cout << "📈 Motor profiling enabled" << std::endl;

    // Initialize CAN bus logger
    g_can_logger = std::make_unique<ic_can::CANBusLogger>("can_bus_analysis");
    if (g_can_logger->start_logging()) {
        std::cout << "📋 CAN bus logging enabled" << std::endl;
    } else {
        std::cout << "❌ Failed to start CAN bus logging" << std::endl;
    }

    // Print header
    print_arm_header();

    // Simple monitoring loop - only get positions and set them
    std::cout << "\n🚀 Starting simple position monitoring..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;

    auto start_time = std::chrono::steady_clock::now();
    /*std::cout << "start? " << std::endl;*/
    /*int a = 0;*/
    /*std::cin >> a;*/
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

      // Profile get_joint_positions
      auto get_start = std::chrono::high_resolution_clock::now();
      auto positions = controller->get_joint_positions();
      auto get_end = std::chrono::high_resolution_clock::now();

      double get_duration =
          std::chrono::duration_cast<std::chrono::microseconds>(get_end -
                                                                get_start)
              .count();

      // DEBUG: Print actual positions being read (reduced frequency)
      if (g_profiler->get_feedback_count(1) % 50 == 0 ||
          g_profiler->get_feedback_count(1) == 0) {
        std::cout << "🔍 DEBUG: Current positions read from motors: ";
        for (int i = 0; i < positions.size(); i++) {
          std::cout << "M" << (i + 1) << "=" << std::fixed
                    << std::setprecision(3) << positions[i] << "rad ("
                    << (positions[i] * 180.0 / M_PI) << "°)";
          if (i < positions.size() - 1)
            std::cout << ", ";
        }
        std::cout << std::endl;
      }

      // Profile set_joint_positions - track individual motor performance
      auto set_start = std::chrono::high_resolution_clock::now();
      std::vector<double> empty = {0, 0, 0, 0, 0, 0, 0, 0, 0};

      // Record timing for each motor
      for (int i = 0; i < positions.size(); i++) {
        g_profiler->record_send_attempt(i + 1); // Motor IDs are 1-9
      }

      controller->set_joint_positions(positions, empty, empty);
      auto set_end = std::chrono::high_resolution_clock::now();

      double set_duration =
          std::chrono::duration_cast<std::chrono::microseconds>(set_end -
                                                                set_start)
              .count();

      // Record timing for all motors
      for (int i = 0; i < positions.size(); i++) {
        g_profiler->record_send_timing(i + 1, set_duration);
      }

      // Monitor feedback based on positions (temporary solution)
      /*monitor_feedback_from_positions(positions);*/

      // Simple timeout detection - if operation takes too long
      if (set_duration > 10000) { // If send takes more than 10ms
        for (int i = 0; i < positions.size(); i++) {
          g_profiler->record_timeout(i + 1);
        }
      }

      // Print profiling results every 50 iterations
      if (g_profiler->get_feedback_count(1) % 50 == 0 &&
          g_profiler->get_feedback_count(1) > 0) {
        std::cout << "📊 PROFILE: Loop " << g_profiler->get_feedback_count(1)
                  << " | get_positions: " << std::fixed << std::setprecision(0)
                  << "μs"
                  << " | set_positions: " << set_duration << "μs"
                  << " | total: " << (get_duration + set_duration) << "μs"
                  << " | timeouts: " << g_profiler->get_timeout_count(1)
                  << std::endl;
      }
    }

    // Print final profiling summary
    if (g_profiler) {
      g_profiler->print_summary();
    }

    // Disable motors
    controller->disable_all();
    std::cout << "\n🎉 Arm position monitoring completed!" << std::endl;

    return 0;

  } catch (const std::exception &e) {
    std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
    return -1;
  }
}
