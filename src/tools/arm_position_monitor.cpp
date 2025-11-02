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

    // Monitoring loop with detailed profiling
    auto start_time = std::chrono::steady_clock::now();
    auto period = std::chrono::duration<double>(1.0 / frequency);
    int update_count = 0;

    // Profiling variables
    double total_refresh_time = 0.0;
    double total_get_data_time = 0.0;
    double total_set_positions_time = 0.0;
    double total_loop_time = 0.0;
    int profile_count = 0;

    // Motor state change tracking
    int total_motor_updates = 0;           // Total times motor states changed
    std::vector<double> last_positions(9, 999.0);  // Previous positions for comparison
    std::vector<int> motor_change_counts(9, 0);    // Changes per motor
    std::vector<double> max_position_changes(9, 0.0); // Max change per motor

    /*controller->load_friction_params_from_file(*/
    /*    "/home/lr-2002/project/InstantCreation/ic_can/"*/
    /*    "friction_calibrated_params.json");*/
    /*controller->enable_friction_compensation();*/

    // High-frequency monitoring loop with low-frequency display
    auto last_display_time = std::chrono::steady_clock::now();
    auto display_period = std::chrono::duration<double>(1.0 / frequency);

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

      // Profile refresh_all() timing
      auto refresh_start = std::chrono::high_resolution_clock::now();
      controller->refresh_all();
      auto refresh_end = std::chrono::high_resolution_clock::now();
      auto refresh_time = std::chrono::duration<double, std::milli>(refresh_end - refresh_start).count();

      // Small delay for CAN response (removed 10ms sleep to improve frequency)

      // Profile get_*_data timing
      auto get_data_start = std::chrono::high_resolution_clock::now();
      auto positions = controller->get_joint_positions();
      auto velocities = controller->get_joint_velocities();
      auto torques = controller->get_joint_torques();
      auto get_data_end = std::chrono::high_resolution_clock::now();
      auto get_data_time = std::chrono::duration<double, std::milli>(get_data_end - get_data_start).count();

      // Profile set_joint_positions timing (this does torque compensation!)
      auto set_positions_start = std::chrono::high_resolution_clock::now();
      std::vector<double> empty = {0, 0, 0, 0, 0, 0, 0, 0, 0};
      controller->set_joint_positions(positions, empty, empty);
      auto set_positions_end = std::chrono::high_resolution_clock::now();
      auto set_positions_time = std::chrono::duration<double, std::milli>(set_positions_end - set_positions_start).count();

      // Accumulate profiling data
      total_refresh_time += refresh_time;
      total_get_data_time += get_data_time;
      total_set_positions_time += set_positions_time;
      profile_count++;

      // Track motor state changes
      bool any_motor_changed = false;
      for (int i = 0; i < 9 && i < (int)positions.size(); i++) {
        double position_change = std::abs(positions[i] - last_positions[i]);

        // Initialize first iteration
        if (last_positions[i] == 999.0) {
          last_positions[i] = positions[i];
          continue;
        }

        // Check if this motor changed (with small threshold to avoid noise)
        if (position_change > 1e-6) {  // 0.000001 rad threshold
          any_motor_changed = true;
          motor_change_counts[i]++;

          if (position_change > max_position_changes[i]) {
            max_position_changes[i] = position_change;
          }

          last_positions[i] = positions[i];
        }
      }

      if (any_motor_changed) {
        total_motor_updates++;
      }

      // Check if it's time to display (low frequency)
      auto current_time = std::chrono::steady_clock::now();
      auto time_since_last_display = std::chrono::duration<double>(current_time - last_display_time);

      if (time_since_last_display >= display_period) {
        print_arm_header();
        print_arm_data(positions, velocities, torques);

        // Show timing profile
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "⏱️  Timings (ms): refresh=" << refresh_time
                  << ", get_data=" << get_data_time
                  << ", set_positions=" << set_positions_time << std::endl;
        std::cout << "📊 Avg (ms): refresh=" << (total_refresh_time / profile_count)
                  << ", get_data=" << (total_get_data_time / profile_count)
                  << ", set_positions=" << (total_set_positions_time / profile_count) << std::endl;

        // Show motor update statistics
        std::cout << "🔄 Motor Updates: Total=" << total_motor_updates
                  << ", Rate=" << (total_motor_updates > 0 ? (1000.0 * total_motor_updates / std::chrono::duration<double>(current_time - start_time).count()) : 0.0) << " Hz" << std::endl;
        std::cout << "🔄 Loop iterations: " << profile_count << " ("
                  << (profile_count / std::chrono::duration<double>(current_time - start_time).count()) << " Hz)" << std::endl;
        std::cout << std::flush;

        last_display_time = current_time;
        update_count++;
      }

      // Note: Logging is handled automatically by the IC_CAN logging system
      // when enabled, which logs at 400Hz in the background

      // Profile total loop time
      auto loop_end = std::chrono::steady_clock::now();
      auto loop_time = std::chrono::duration<double, std::milli>(loop_end - loop_start).count();
      total_loop_time += loop_time;

      // NO ARTIFICIAL SLEEP - let the loop run at maximum speed!
      // Only limited by actual processing time
    }

    // Print final profiling summary
    double total_run_time = std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time).count();

    std::cout << "\n📈 ===== COMPREHENSIVE PROFILING SUMMARY =====" << std::endl;
    std::cout << std::fixed << std::setprecision(3);

    // Timing Summary
    std::cout << "⏱️  TIMING ANALYSIS:" << std::endl;
    std::cout << "  Total runtime:         " << total_run_time << " seconds" << std::endl;
    std::cout << "  Total loop iterations: " << profile_count << std::endl;
    std::cout << "  Actual loop frequency: " << (profile_count / total_run_time) << " Hz" << std::endl;
    std::cout << "  Avg loop time:         " << (total_loop_time / profile_count) << " ms" << std::endl;
    std::cout << "  Operation breakdown:" << std::endl;
    std::cout << "    refresh_all():       " << (total_refresh_time / profile_count) << " ms ("
              << (100.0 * total_refresh_time / total_loop_time) << "%)" << std::endl;
    std::cout << "    get_*_data():        " << (total_get_data_time / profile_count) << " ms ("
              << (100.0 * total_get_data_time / total_loop_time) << "%)" << std::endl;
    std::cout << "    set_positions():     " << (total_set_positions_time / profile_count) << " ms ("
              << (100.0 * total_set_positions_time / total_loop_time) << "%)" << std::endl;

    // Motor State Change Summary
    std::cout << "\n🔄 MOTOR STATE CHANGE ANALYSIS:" << std::endl;
    std::cout << "  Total motor updates:   " << total_motor_updates << std::endl;
    std::cout << "  Motor update rate:     " << (total_motor_updates / total_run_time) << " Hz" << std::endl;
    std::cout << "  Data freshness ratio:  " << (100.0 * total_motor_updates / profile_count) << "%" << std::endl;

    // Per-motor detailed statistics
    const char* motor_types[] = {"DM10010L", "DM6248", "DM6248", "DM4340", "DM4340", "DM4310", "HT4438", "HT4438", "SERVO"};
    std::cout << "\n🦾 PER-MOTOR CHANGE STATISTICS:" << std::endl;
    for (int i = 0; i < 9; i++) {
      std::cout << "  Motor " << (i+1) << " (" << motor_types[i] << "): "
                << motor_change_counts[i] << " changes, "
                << "max change: " << std::setprecision(6) << max_position_changes[i] << " rad ("
                << std::setprecision(2) << (max_position_changes[i] * 180.0 / M_PI) << "°), "
                << "rate: " << std::setprecision(1) << (motor_change_counts[i] / total_run_time) << " Hz" << std::endl;
    }

    // Key Insights
    std::cout << "\n💡 KEY INSIGHTS:" << std::endl;
    if (total_motor_updates == 0) {
      std::cout << "  ⚠️  NO MOTOR STATE CHANGES DETECTED!" << std::endl;
      std::cout << "  ⚠️  This explains the 'ka-ka-ka' jerky motion in replay!" << std::endl;
    } else if ((100.0 * total_motor_updates / profile_count) < 50.0) {
      std::cout << "  ⚠️  Low data freshness: Only " << (100.0 * total_motor_updates / profile_count)
                << "% of loops had new motor data" << std::endl;
      std::cout << "  ⚠️  This will cause repeated values in logged data" << std::endl;
    } else {
      std::cout << "  ✅ Good data freshness: " << (100.0 * total_motor_updates / profile_count)
                << "% of loops had new motor data" << std::endl;
    }

    std::cout << "==============================================================" << std::endl;

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
