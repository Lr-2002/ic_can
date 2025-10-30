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

#include "ic_can/core/ic_can.hpp"
// #include "ic_can/core/torque_predictor_unified.h"  // Temporarily disabled
#include "ic_can/core/gripper_component.hpp"
#include "ic_can/core/wrist_component.hpp"
// #include "ic_can/core/can_frame_dispatcher.hpp"  // Temporarily disabled
// #include "ic_can/core/usb2can_communication_adapter.hpp"  // Temporarily
// disabled
#include "ic_can/core/arm_component.hpp"

// Include stub implementations
#include "ic_can/core/torque_predictor_unified.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <mutex>
#include <nlohmann/json.hpp>
#include <sstream>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

// Use working dm-tools SDK directly
#include "protocol/usb_class.h"

namespace ic_can {

using json = nlohmann::json;

// Friction compensation data structure
struct FrictionParams {
  double Fc = 0.0;                 // Coulomb friction coefficient
  double Fv = 0.0;                 // Viscous friction coefficient
  double velocity_threshold = 0.1; // Velocity threshold for friction activation
};

// ArmComponent is now defined in arm_component.hpp

// Use the actual component implementations

class SafetyModule {
public:
  void print_info() { std::cout << "Safety Module (placeholder)" << std::endl; }
};

class IC_CAN::Impl {
  friend class IC_CAN; // Allow IC_CAN to access private members
public:
  Impl(const std::string &device_sn, bool debug)
      : device_sn_(device_sn), debug_enabled_(debug), connected_(false),
        hf_control_running_(false), control_running_(false),
        logging_running_(false), performance_monitoring_(false),
        gravity_compensation_enabled_(true),
        friction_compensation_enabled_(false), velocity_damping_(0.1),
        smooth_transition_(true), sgn_threshold_(0.01),
        wrist_monitor_running_(false), wrist_monitor_frequency_(50.0) {
    // Initialize motor gains with default values
    load_default_motor_gains();

    // Initialize friction parameters with default values
    load_default_friction_params();

    // Initialize performance counters
    send_count_ = 0;
    receive_count_ = 0;
    total_bytes_sent_ = 0;
    total_bytes_received_ = 0;
    performance_start_time_ = std::chrono::high_resolution_clock::now();

    // Initialize wrist monitoring data
    last_wrist_positions_ = {0.0, 0.0};
    last_wrist_velocities_ = {0.0, 0.0};
    last_wrist_torques_ = {0.0, 0.0};
    last_wrist_update_time_ = std::chrono::steady_clock::now();

    // Initialize unified torque predictor - using stub implementation
    std::cout << "🔧 Initializing torque predictor with stub implementation..."
              << std::endl;
    torque_predictor_ = std::make_unique<TorquePredictorUnified>();
    std::cout << "✅ Torque predictor initialized (stub)" << std::endl;

    // Initialize components
    wrist_component_ = std::make_unique<WristComponent>();
    gripper_component_ = std::make_unique<GripperComponent>();
    arm_component_ = std::make_unique<ArmComponent>();

    std::cout << "✅ Arm, Wrist, and Gripper components initialized"
              << std::endl;
  }

  ~Impl() { shutdown(); }

  bool initialize() {
    if (connected_)
      return true;

    try {
      std::cout << "🔧 Connecting to USB2CAN device..." << std::endl;
      std::cout << "Device SN: " << device_sn_ << std::endl;

      device_ = std::make_unique<usb_class>(1000000, 5000000, device_sn_);
      if (!device_) {
        std::cout << "❌ FAILED: Could not create usb_class" << std::endl;
        return false;
      }

      std::cout << "✅ SUCCESS: Device connected" << std::endl;

      // Set up callback for motor feedback
      std::cout << "📡 Setting up motor feedback callback..." << std::endl;
      device_->setFrameCallback(
          [this](can_value_type &frame) { handle_can_frame(frame); });
      std::cout << "✅ Callback configured" << std::endl;

      // Start data capture
      std::cout << "🔄 Starting data capture..." << std::endl;
      uint8_t capture_result = device_->USB_CMD_START_CAP();
      if (capture_result != 0) {
        std::cout << "❌ FAILED: Data capture start returned "
                  << (int)capture_result << std::endl;
        return false;
      }

      // Test if callback is working by sending a refresh command
      std::cout << "🧪 Testing CAN communication..." << std::endl;
      bool test_success = refresh_all();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      if (receive_count_.load() == 0) {
        std::cout << "❌ WARNING: No CAN frames received during test"
                  << std::endl;
        std::cout << "   - Check device connection" << std::endl;
        std::cout << "   - Verify CAN bus termination" << std::endl;
        std::cout << "   - Check motor power supply" << std::endl;
      } else {
        std::cout << "✅ SUCCESS: Received " << receive_count_.load()
                  << " CAN frames during test" << std::endl;
      }

      connected_ = true;
      return true;

    } catch (const std::exception &e) {
      std::cout << "❌ Connection failed: " << e.what() << std::endl;
      return false;
    }
  }

  void shutdown() {
    if (connected_ && device_) {
      stop_high_frequency_control();
      stop_control_loop();
      stop_wrist_position_monitoring();
      stop_logging();
      disable_frequency_monitoring();
      device_->USB_CMD_STOP_CAP();
      connected_ = false;
    }
  }
  void load_default_motor_gains() {
    std::lock_guard<std::mutex> lock(motor_gains_mutex_);
    /**/
    /*motor_kp_gains_ = {480, 120, 120, 80, 150, 30, 8, 8, 0};*/
    /*motor_kd_gains_ = {4, 2, 2, 1.8, 2.2, 1, 1.2, 1.2, 0};*/

    motor_kp_gains_ = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    motor_kd_gains_ = {0, 0, 0, 0, 0, 0, 0.0, 0.0, 0};

    if (debug_enabled_) {
      std::cout << "✅ Loaded default motor gains" << std::endl;
      for (auto i = 0; i <= 8; i++)
        std::cout << "Motor " << i << " kp: " << motor_kp_gains_[i]
                  << " kd: " << motor_kd_gains_[i] << std::endl;
    }
  }

  void load_default_friction_params() {
    std::lock_guard<std::mutex> lock(friction_params_mutex_);

    // Initialize with conservative default friction parameters
    friction_params_ = {{
        {0.5, 1.0, 0.1},   // Joint 0
        {0.3, 0.8, 0.1},   // Joint 1
        {0.4, 0.6, 0.1},   // Joint 2
        {0.3, 0.5, 0.08},  // Joint 3
        {0.2, 0.4, 0.06},  // Joint 4
        {0.15, 0.2, 0.05}, // Joint 5
        {0.1, 0.1, 0.03},  // Joint 6 (gripper)
        {0.1, 0.1, 0.03},  // Joint 7 (gripper)
        {0.05, 0.05, 0.02} // Joint 8 (gripper)
    }};

    if (debug_enabled_) {
      std::cout << "✅ Loaded default friction parameters" << std::endl;
      for (int i = 0; i < 9; i++) {
        std::cout << "Joint " << i << " Fc: " << friction_params_[i].Fc
                  << " Fv: " << friction_params_[i].Fv
                  << " threshold: " << friction_params_[i].velocity_threshold
                  << std::endl;
      }
    }
  }

  bool load_friction_params_from_file(const std::string &filename) {
    std::lock_guard<std::mutex> lock(friction_params_mutex_);

    std::ifstream file(filename);
    if (!file.is_open()) {
      std::cout << "❌ Cannot open friction parameters file: " << filename
                << std::endl;
      return false;
    }

    try {
      json j;
      file >> j;

      // Load global friction compensation settings
      if (j.contains("enable_friction")) {
        friction_compensation_enabled_ = j["enable_friction"].get<bool>();
      }
      if (j.contains("velocity_damping")) {
        velocity_damping_ = j["velocity_damping"].get<double>();
      }
      if (j.contains("smooth_transition")) {
        smooth_transition_ = j["smooth_transition"].get<bool>();
      }
      if (j.contains("sgn_threshold")) {
        sgn_threshold_ = j["sgn_threshold"].get<double>();
      }

      // Load joint-specific friction parameters
      if (j.contains("friction_params") && j["friction_params"].is_array()) {
        auto friction_array = j["friction_params"];
        size_t num_joints =
            std::min(friction_array.size(), friction_params_.size());

        for (size_t i = 0; i < num_joints; i++) {
          const auto &joint_params = friction_array[i];
          if (joint_params.contains("joint") && joint_params.contains("Fc") &&
              joint_params.contains("Fv")) {
            int joint_id = joint_params["joint"].get<int>();
            if (joint_id >= 0 && joint_id < 9) {
              friction_params_[joint_id].Fc = joint_params["Fc"].get<double>();
              friction_params_[joint_id].Fv = joint_params["Fv"].get<double>();
              if (joint_params.contains("velocity_threshold")) {
                friction_params_[joint_id].velocity_threshold =
                    joint_params["velocity_threshold"].get<double>();
              }
            }
          }
        }
      }

      std::cout << "✅ Successfully loaded friction parameters from: "
                << filename << std::endl;
      std::cout << "   Friction compensation: "
                << (friction_compensation_enabled_ ? "enabled" : "disabled")
                << std::endl;

      if (debug_enabled_) {
        std::cout << "📋 Loaded friction parameters:" << std::endl;
        for (int i = 0; i < 9; i++) {
          std::cout << "   Joint " << i << ": Fc=" << friction_params_[i].Fc
                    << " Fv=" << friction_params_[i].Fv
                    << " threshold=" << friction_params_[i].velocity_threshold
                    << std::endl;
        }
      }

      return true;

    } catch (const json::exception &e) {
      std::cout << "❌ JSON parsing error in friction parameters file: "
                << e.what() << std::endl;
      return false;
    } catch (const std::exception &e) {
      std::cout << "❌ Error loading friction parameters file: " << e.what()
                << std::endl;
      return false;
    }
  }

  // Frequency monitoring and logging implementation
  bool start_logging(const std::string &log_directory) {
    if (logging_running_) {
      std::cout << "⚠️ Logging already running, stopping first..." << std::endl;
      stop_logging();
    }

    // Validate log directory path
    if (log_directory.empty()) {
      std::cout << "❌ Invalid log directory: empty path" << std::endl;
      return false;
    }

    // Create timestamped subdirectory
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    ss << "_" << std::setfill('0') << std::setw(3) << ms.count();

    std::string timestamped_dir = log_directory + "/ic_can_log_" + ss.str();

    // Test directory creation/writability
    if (std::system(("mkdir -p " + timestamped_dir).c_str()) != 0) {
      std::cout << "❌ Cannot create or access directory: " << timestamped_dir
                << std::endl;
      return false;
    }

    // Test file creation
    std::string test_file = timestamped_dir + "/.test_write";
    std::ofstream test_stream(test_file);
    if (!test_stream.is_open()) {
      std::cout << "❌ Cannot write to directory: " << timestamped_dir
                << std::endl;
      return false;
    }
    test_stream << "test";
    test_stream.close();
    std::remove(test_file.c_str());

    logging_directory_ = timestamped_dir;
    logging_running_ = true;

    // Reset logging counters for new trajectory session
    {
      std::lock_guard<std::mutex> lock(logging_mutex_);
      logging_start_time_ = std::chrono::high_resolution_clock::now();
      last_sent_positions_.clear();
      last_sent_positions_.resize(9, 0.0);
    }

    // Start logger thread
    logger_thread_ = std::thread([this]() { logger_thread_function(); });

    // Start performance monitoring if not already running
    if (!performance_monitoring_) {
      enable_frequency_monitoring();
    }

    std::cout << "✅ Started trajectory logging to: " << timestamped_dir
              << std::endl;
    return true;
  }

  bool start_trajectory_logging(const std::string &log_directory) {
    // Alias for start_logging with clearer naming
    return start_logging(log_directory);
  }

  void stop_logging() {
    logging_running_ = false;
    if (logger_thread_.joinable()) {
      logger_thread_.join();
    }
    std::cout << "✅ Logging stopped" << std::endl;
  }

  void enable_frequency_monitoring() {
    if (performance_monitoring_) {
      return;
    }

    performance_monitoring_ = true;
    performance_thread_ =
        std::thread([this]() { performance_monitor_thread_function(); });

    std::cout << "✅ Frequency monitoring enabled" << std::endl;
  }

  void disable_frequency_monitoring() {
    performance_monitoring_ = false;
    if (performance_thread_.joinable()) {
      performance_thread_.join();
    }
    std::cout << "✅ Frequency monitoring disabled" << std::endl;
  }

  std::map<std::string, double> get_performance_stats() {
    std::lock_guard<std::mutex> lock(performance_mutex_);

    auto now = std::chrono::high_resolution_clock::now();
    double elapsed_sec =
        std::chrono::duration<double>(now - performance_start_time_).count();

    if (elapsed_sec < 0.1) {
      return {{"send_frequency", 0.0},
              {"receive_frequency", 0.0},
              {"total_commands_sent", 0.0},
              {"total_messages_received", 0.0},
              {"uptime_seconds", elapsed_sec}};
    }

    double send_freq = (send_count_ / elapsed_sec);
    double recv_freq = (receive_count_ / elapsed_sec);

    return {{"send_frequency", send_freq},
            {"receive_frequency", recv_freq},
            {"total_commands_sent", static_cast<double>(send_count_)},
            {"total_messages_received", static_cast<double>(receive_count_)},
            {"uptime_seconds", elapsed_sec},
            {"send_rate_kbps", (total_bytes_sent_ * 8) / 1024.0 / elapsed_sec},
            {"receive_rate_kbps",
             (total_bytes_received_ * 8) / 1024.0 / elapsed_sec}};
  }

  void print_performance_stats() {
    auto stats = get_performance_stats();

    std::cout << "\n📊 Performance Statistics:" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    std::cout << "Send Frequency: " << std::fixed << std::setprecision(1)
              << stats["send_frequency"] << " Hz" << std::endl;
    std::cout << "Receive Frequency: " << std::fixed << std::setprecision(1)
              << stats["receive_frequency"] << " Hz" << std::endl;
    std::cout << "Total Commands Sent: " << std::fixed << std::setprecision(0)
              << stats["total_commands_sent"] << std::endl;
    std::cout << "Total Messages Received: " << std::fixed
              << std::setprecision(0) << stats["total_messages_received"]
              << std::endl;
    std::cout << "Send Rate: " << std::fixed << std::setprecision(2)
              << stats["send_rate_kbps"] << " kB/s" << std::endl;
    std::cout << "Receive Rate: " << std::fixed << std::setprecision(2)
              << stats["receive_rate_kbps"] << " kB/s" << std::endl;
    std::cout << "Uptime: " << std::fixed << std::setprecision(1)
              << stats["uptime_seconds"] << " s" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
  }

  bool enable_all() {
    if (!connected_)
      return false;

    std::cout << "\n⚡ Enabling all motors..." << std::endl;

    // Enable motors 1-9 using DM protocol
    std::vector<uint8_t> enable_cmd = {0xFF, 0xFF, 0xFF, 0xFF,
                                       0xFF, 0xFF, 0xFF, 0xFC};

    bool all_success = true;
    for (int motor_id = 1; motor_id <= 9; motor_id++) {
      send_can_frame(motor_id, enable_cmd,
                     false); // Standard frame for all motors
      std::cout << "   ✅ Motor " << motor_id << " enabled" << std::endl;
      usleep(100000); // 100ms between enables
    }

    std::cout << "✅ All motors enabled" << std::endl;
    return true;
  }

  bool disable_all() {
    if (!connected_)
      return false;

    std::cout << "\n🔌 Disabling all motors..." << std::endl;

    // Disable motors 1-9
    std::vector<uint8_t> disable_cmd = {0xFF, 0xFF, 0xFF, 0xFF,
                                        0xFF, 0xFF, 0xFF, 0xFD};

    for (int motor_id = 1; motor_id <= 9; motor_id++) {
      send_can_frame(motor_id, disable_cmd,
                     false); // Standard frame for all motors
    }

    std::cout << "✅ All motors disabled" << std::endl;
    return true;
  }

  bool set_zero_all() {
    if (!connected_)
      return false;

    std::cout << "\n🎯 Setting all motors to zero position..." << std::endl;

    // Create zero position vector for all 9 motors
    std::vector<double> zero_positions(9, 0.0);

    // Send zero position command to all motors
    bool success = set_joint_positions(zero_positions, {}, {});

    if (success) {
      std::cout << "✅ Zero position command sent to all motors" << std::endl;
    } else {
      std::cout << "❌ Failed to send zero position command" << std::endl;
    }

    return success;
  }

  bool set_motor_zero_calibration(int motor_id) {
    if (!connected_)
      return false;

    if (motor_id < 1 || motor_id > 9) {
      std::cout << "❌ Invalid motor ID: " << motor_id << " (must be 1-9)"
                << std::endl;
      return false;
    }

    std::cout << "🎯 Setting Motor " << motor_id
              << " zero position calibration..." << std::endl;

    // Send zero position calibration command (0xFE command)
    // Based on Python damiao.py: set_zero_position method
    std::vector<uint8_t> zero_cmd = {0xFF, 0xFF, 0xFF, 0xFF,
                                     0xFF, 0xFF, 0xFF, 0xFE};

    // Determine CAN ID based on motor type and mode
    // For Damiao motors 1-6 and servo motor 9, use motor ID directly
    // For HT motors 7-8, we may need different approach
    int can_id = motor_id;

    std::cout << "🔧 Sending zero calibration command to Motor " << motor_id
              << " (CAN ID: " << std::hex << "0x" << can_id << std::dec << ")"
              << std::endl;

    // Send the raw CAN command using universal function
    send_can_frame(can_id, zero_cmd, false); // Standard frame

    std::cout << "✅ Zero calibration command sent to Motor " << motor_id
              << std::endl;
    std::cout << "📝 The current position is now set as the new zero (0.0 rad)"
              << std::endl;

    // Give motor time to process the command
    usleep(5000); // 5ms

    return true;
  }

  bool refresh_all() {
    if (!connected_ || !device_) {
      std::cout << "❌ Cannot refresh - not connected" << std::endl;
      return false;
    }

    // Send status request to trigger position feedback for all motor types
    bool success = true;

    try {
      // Refresh Damiao motors 1-6 with individual status requests
      for (int motor_id = 1; motor_id <= 6; motor_id++) {
        std::vector<uint8_t> status_cmd = {uint8_t(motor_id), 0x00, 0xCC, 0x00};
        send_can_frame(motor_id, status_cmd,
                       false); // Standard frame for Damiao
        if (debug_enabled_) {
          std::cout << "📤 Sent status request to DM motor " << motor_id
                    << std::endl;
        }
        usleep(500); // Small delay between requests
      }

      // Refresh HT motors 7-8 with their specific protocol
      send_ht_read_state();

      // Refresh servo motor 9
      std::vector<uint8_t> servo_status_cmd = {0x09, 0x00, 0xCC, 0x00};
      send_can_frame(9, servo_status_cmd, false); // Standard frame for servo

      if (debug_enabled_) {
        std::cout << "📤 Sent status request to servo motor 9" << std::endl;
      }

    } catch (const std::exception &e) {
      std::cout << "❌ Error during refresh: " << e.what() << std::endl;
      success = false;
    }

    return success;
  }

  std::vector<double> get_joint_positions() {
    std::lock_guard<std::mutex> lock(positions_mutex_);
    std::vector<double> positions(9, 0.0);

    for (int i = 0; i < 9; i++) {
      positions[i] = positions_[i].load();
    }

    return positions;
  }

  std::vector<double> get_joint_velocities() {
    std::lock_guard<std::mutex> lock(velocities_mutex_);
    std::vector<double> velocities(9, 0.0);

    for (int i = 0; i < 9; i++) {
      velocities[i] = velocities_[i].load();
    }

    return velocities;
  }

  std::vector<double> get_joint_torques() {
    std::lock_guard<std::mutex> lock(torques_mutex_);
    std::vector<double> torques(9, 0.0);

    for (int i = 0; i < 9; i++) {
      torques[i] = torques_[i].load();
    }

    return torques;
  }

  bool set_joint_positions(const std::vector<double> &positions,
                           const std::vector<double> &velocities,
                           const std::vector<double> &torques) {
    /*std::cout << " debug: running set joint positions " << std::endl;*/
    /*std::cout << " debug: connected=" << connected_*/
    /*          << ", positions.size()=" << positions.size() << std::endl;*/
    send_count_++;
    if (!connected_) {
      std::cout << " debug: FAILED - not connected" << std::endl;
      return false;
    }
    if (positions.size() < 9) {
      std::cout << " debug: FAILED - positions size < 9" << std::endl;
      return false;
    }

    // Get motor-specific gains
    std::array<double, 9> kp_values, kd_values;
    {
      std::lock_guard<std::mutex> lock(motor_gains_mutex_);
      for (int i = 0; i < 9; i++) {
        kp_values[i] = motor_kp_gains_[i];
        kd_values[i] = motor_kd_gains_[i];
      }
    }

    // Get gravity compensation torques if enabled
    std::vector<double> gravity_torques(9, 0.0);
    if (gravity_compensation_enabled_) {
      gravity_torques = get_gravity_compensation_torques();
    }

    // Get friction compensation torques if enabled
    std::vector<double> friction_torques(9, 0.0);
    if (friction_compensation_enabled_) {
      // Use actual velocities if available, otherwise use commanded velocities
      std::vector<double> actual_velocities;
      if (velocities.size() >= 9) {
        actual_velocities = velocities;
      } else {
        actual_velocities = get_joint_velocities();
      }
      friction_torques = get_friction_compensation_torques(actual_velocities);
    }

    for (int i = 0; i < 9; i++) {
      double pos = positions[i];
      double vel = (velocities.size() > i) ? velocities[i] : 0.0;
      double tau = (torques.size() > i) ? torques[i] : 0.0;
      double kp = kp_values[i];
      double kd = kd_values[i];

      // Add gravity compensation to torque feedforward
      /*std::cout << " the gravity compensation is "*/
      /*          << gravity_compensation_enabled_ << std::endl;*/
      if (gravity_compensation_enabled_ && i < 6) {
        tau += gravity_torques[i];
        /*std::cout << "motor id is " << i << ", torque is " <<
         * gravity_torques[i]*/
        /*          << std::endl;*/
      }

      // Add friction compensation to torque feedforward
      if (friction_compensation_enabled_) {
        tau += friction_torques[i];
        if (debug_enabled_ && i < 6) {
          std::cout << "🔧 Joint " << i << " compensation - gravity: "
                    << (gravity_compensation_enabled_ ? gravity_torques[i]
                                                      : 0.0)
                    << ", friction: " << friction_torques[i] << std::endl;
        }
      }

      if (i < 6) {
        // Damiao motors 1-6: use DM MIT protocol
        send_dm_mit_command(i + 1, pos, vel, tau, kp, kd);
      }
      /*} else if (i < 8) {*/
      /*  // HT motors 7-8: use HT MIT protocol*/
      /*  send_ht_mit_command(pos, vel, tau, kp, kd);*/
      /*} else {*/
      /*  // Servo motor 9: use DM protocol as placeholder*/
      /*  send_dm_mit_command(i + 1, pos, vel, tau, kp, kd);*/
      /*}*/
    }

    // Record the actually sent positions, velocities, and torques for logging
    if (logging_running_) {
      std::lock_guard<std::mutex> lock(logging_mutex_);
      last_sent_positions_ = positions;
      last_sent_velocities_ = velocities;
      last_sent_torques_ = torques;
    }

    return true;
  }

  bool start_high_frequency_control() {
    if (hf_control_running_) {
      std::cout << "⚠️ High-frequency control already running" << std::endl;
      return true;
    }

    if (!connected_ || !device_) {
      std::cout << "❌ Cannot start high-frequency control - not connected"
                << std::endl;
      return false;
    }

    std::cout << "🚀 Starting high-frequency control at 500Hz..." << std::endl;

    hf_control_running_ = true;
    hf_control_thread_ = std::thread([this]() {
      auto target_period = std::chrono::microseconds(2000); // 500Hz = 2ms
      auto last_time = std::chrono::steady_clock::now();
      uint64_t iteration_count = 0;
      auto performance_report_time = last_time + std::chrono::seconds(5);

      while (hf_control_running_) {
        auto start_time = std::chrono::steady_clock::now();

        // Send refresh commands to all motors
        bool refresh_success = refresh_all();

        // Track performance
        if (!refresh_success) {
          std::cout << "⚠️ Refresh failed at iteration " << iteration_count
                    << std::endl;
        }

        iteration_count++;

        // Performance monitoring and reporting
        auto current_time = std::chrono::steady_clock::now();
        if (current_time >= performance_report_time) {
          auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                             current_time - last_time)
                             .count();
          double actual_frequency =
              static_cast<double>(iteration_count) / elapsed;

          if (debug_enabled_) {
            std::cout << "📊 HF Control Performance: " << std::fixed
                      << std::setprecision(1) << actual_frequency
                      << " Hz (target: 500Hz)" << std::endl;
          }

          // Reset counters
          iteration_count = 0;
          last_time = current_time;
          performance_report_time = current_time + std::chrono::seconds(5);
        }

        // Calculate precise sleep time to maintain 500Hz
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        auto sleep_time = target_period - elapsed;

        if (sleep_time.count() > 0) {
          std::this_thread::sleep_for(sleep_time);
        } else if (sleep_time.count() < -1000) { // More than 1ms overtime
          std::cout << "⚠️ HF control overtime: " << -sleep_time.count()
                    << " microseconds" << std::endl;
        }
      }
    });

    std::cout << "✅ High-frequency control thread started" << std::endl;
    return true;
  }

  void stop_high_frequency_control() {
    std::cout << "🛑 Stopping high-frequency control..." << std::endl;
    hf_control_running_ = false;
    if (hf_control_thread_.joinable()) {
      hf_control_thread_.join();
    }
    std::cout << "✅ High-frequency control stopped" << std::endl;
  }

  bool is_hf_control_running() const { return hf_control_running_; }

  // Configurable control loop implementation with offline trajectory
  // generation
  bool start_control_loop(double frequency) {
    if (control_running_) {
      std::cout << "⚠️ Control loop already running" << std::endl;
      return true;
    }

    if (!connected_ || !device_) {
      std::cout << "❌ Cannot start control loop - not connected" << std::endl;
      return false;
    }

    if (frequency <= 0 || frequency > 1000) {
      std::cout << "❌ Invalid frequency: " << frequency
                << " Hz (must be 0-1000 Hz)" << std::endl;
      return false;
    }

    control_frequency_ = frequency;
    control_running_ = true;

    // Reset performance counters for accurate loop timing measurement
    {
      std::lock_guard<std::mutex> lock(performance_mutex_);
      send_count_ = 0;
      receive_count_ = 0;
      total_bytes_sent_ = 0;
      total_bytes_received_ = 0;
      performance_start_time_ = std::chrono::high_resolution_clock::now();
    }

    // Initialize interpolation state and pre-compute trajectory
    {
      std::lock_guard<std::mutex> lock(interpolation_mutex_);

      // Get current and target positions
      current_positions_ = get_joint_positions();

      if (target_positions_.empty()) {
        std::cout << "⚠️ No target positions set, using current positions"
                  << std::endl;
        target_positions_ = current_positions_;
      }

      // Pre-compute entire trajectory offline
      std::cout << "📊 Pre-computing trajectory at " << frequency << "Hz..."
                << std::endl;
      std::cout << "📊 Target positions size: " << target_positions_.size()
                << std::endl;

      trajectory_points_ =
          generate_trajectory_offline(current_positions_, target_positions_,
                                      1.0 / control_frequency_, max_velocity_);
      current_trajectory_index_ = 0;

      std::cout << "✅ Generated " << trajectory_points_.size()
                << " trajectory points" << std::endl;
    }

    control_thread_ = std::thread([this, frequency]() {
      auto target_period =
          std::chrono::duration<double>(1.0 / control_frequency_);
      auto last_time = std::chrono::steady_clock::now();
      uint64_t loop_iterations = 0;
      auto performance_report_time = last_time + std::chrono::seconds(5);

      std::cout << "🎮 Control loop started with period: "
                << target_period.count() * 1000 << " ms" << std::endl;

      while (control_running_) {
        auto start_time = std::chrono::steady_clock::now();

        // Get next pre-computed position from trajectory
        std::vector<double> next_position;
        bool trajectory_complete = false;

        {
          std::lock_guard<std::mutex> lock(interpolation_mutex_);

          if (current_trajectory_index_ < trajectory_points_.size()) {
            next_position = trajectory_points_[current_trajectory_index_];
            current_trajectory_index_++;

            if (debug_enabled_ && (loop_iterations % 100 == 0)) {
              std::cout << "🎯 Step " << current_trajectory_index_ << "/"
                        << trajectory_points_.size() << ": ";
              for (int i = 0; i < std::min(9, (int)next_position.size()); i++) {
                std::cout << std::fixed << std::setprecision(3)
                          << next_position[i] << " ";
              }
              std::cout << std::endl;
            }
          } else {
            // Trajectory completed, hold final position
            next_position = trajectory_points_.back();
            trajectory_complete = true;

            if (loop_iterations % 500 == 0) {
              std::cout << "🏁 Trajectory completed, holding position"
                        << std::endl;
            }
          }
        }

        // Send position to motors with error handling
        bool command_success = set_joint_positions(next_position, {}, {});
        if (!command_success) {
          std::cout << "⚠️ Failed to send position command at iteration "
                    << loop_iterations << std::endl;
        }

        // Optionally request status updates (can be disabled for performance)
        if (control_frequency_ <= 100) { // Only refresh at lower frequencies
          refresh_all();
        }

        loop_iterations++;

        // Performance monitoring
        auto current_time = std::chrono::steady_clock::now();
        if (current_time >= performance_report_time) {
          auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                             current_time - last_time)
                             .count();
          double actual_frequency =
              static_cast<double>(loop_iterations) / elapsed;

          std::cout << "📊 Control Loop Performance: " << std::fixed
                    << std::setprecision(1) << actual_frequency
                    << " Hz (target: " << control_frequency_ << "Hz)"
                    << std::endl;
          if (trajectory_complete) {
            std::cout << "📍 Status: Holding final position" << std::endl;
          }

          // Reset counters
          loop_iterations = 0;
          last_time = current_time;
          performance_report_time = current_time + std::chrono::seconds(5);
        }

        // Calculate precise sleep time to maintain frequency
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        auto sleep_time = target_period - elapsed;

        if (sleep_time.count() > 0) {
          std::this_thread::sleep_for(sleep_time);
        } else if (sleep_time.count() <
                   -target_period.count() * 0.5) { // More than 50% overtime
          std::cout << "⚠️ Control loop significant overtime: "
                    << -sleep_time.count() * 1000 << " ms" << std::endl;
        }
      }
    });

    std::cout << "✅ Started control loop at " << frequency << " Hz"
              << std::endl;
    return true;
  }

  void stop_control_loop() {
    std::cout << "🛑 Stopping control loop..." << std::endl;
    control_running_ = false;
    if (control_thread_.joinable()) {
      control_thread_.join();
    }
    std::cout << "✅ Control loop stopped" << std::endl;
  }

  // Emergency stop method for safety
  bool emergency_stop() {
    std::cout << "🚨 EMERGENCY STOP ACTIVATED!" << std::endl;

    // Stop all control loops immediately
    hf_control_running_ = false;
    control_running_ = false;

    // Wait for threads to finish
    if (hf_control_thread_.joinable()) {
      hf_control_thread_.join();
    }
    if (control_thread_.joinable()) {
      control_thread_.join();
    }

    // Send zero torque commands to all motors
    bool success = true;
    try {
      std::vector<double> zero_torques(9, 0.0);
      std::vector<double> dummy_positions(9, 0.0);
      std::vector<double> dummy_velocities(9, 0.0);
      success =
          set_joint_positions(dummy_positions, dummy_velocities, zero_torques);

      // Hold current positions with minimal gains
      auto current_pos = get_joint_positions();
      std::vector<double> min_gains(9, 0.1);
      std::vector<double> min_dgains(9, 0.01);
      set_all_motor_gains(min_gains, min_dgains);
      set_joint_positions(current_pos, {}, {});

      std::cout << "🛑 Emergency stop completed - motors in safe hold mode"
                << std::endl;
    } catch (const std::exception &e) {
      std::cout << "❌ Emergency stop error: " << e.what() << std::endl;
      success = false;
    }

    return success;
  }

  // Safety check method
  bool check_system_safety() {
    if (!connected_ || !device_) {
      std::cout << "❌ Safety check failed: Not connected" << std::endl;
      return false;
    }

    // Check motor feedback freshness
    auto now = std::chrono::steady_clock::now();
    auto time_since_last_feedback =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - std::chrono::steady_clock::time_point(
                      performance_start_time_.time_since_epoch()))
            .count();

    if (time_since_last_feedback > 1000) { // No feedback for more than 1 second
      std::cout << "⚠️ Safety warning: No motor feedback for "
                << time_since_last_feedback << " ms" << std::endl;
      return false;
    }

    // Check motor positions are within reasonable bounds
    auto positions = get_joint_positions();
    for (int i = 0; i < 6; i++) {                 // Check arm motors
      if (std::abs(positions[i]) > 3.14159 * 2) { // More than 360 degrees
        std::cout << "⚠️ Safety warning: Motor " << (i + 1)
                  << " position out of bounds: " << positions[i] << " rad"
                  << std::endl;
        return false;
      }
    }

    return true;
  }

  // Wrist position monitoring methods
  bool start_wrist_position_monitoring(double frequency = 50.0) {
    if (wrist_monitor_running_) {
      std::cout << "⚠️ Wrist position monitoring already running" << std::endl;
      return true;
    }

    if (!connected_ || !device_) {
      std::cout << "❌ Cannot start wrist monitoring - not connected"
                << std::endl;
      return false;
    }

    if (frequency <= 0 || frequency > 1000) {
      std::cout << "❌ Invalid wrist monitoring frequency: " << frequency
                << " Hz (must be 0-1000 Hz)" << std::endl;
      return false;
    }

    std::cout << "🦾 Starting wrist position monitoring at " << frequency
              << "Hz..." << std::endl;

    wrist_monitor_running_ = true;
    wrist_monitor_frequency_ = frequency;
    wrist_monitor_thread_ = std::thread([this]() {
      auto target_period =
          std::chrono::duration<double>(1.0 / wrist_monitor_frequency_);
      auto last_time = std::chrono::steady_clock::now();
      uint64_t monitor_iterations = 0;
      auto report_time = last_time + std::chrono::seconds(5);

      while (wrist_monitor_running_) {
        auto start_time = std::chrono::steady_clock::now();

        // Refresh wrist motors only (motors 7 and 8)
        refresh_wrist_motors_only();

        // Get wrist positions
        auto wrist_positions = get_wrist_positions();
        auto wrist_velocities = get_wrist_velocities();
        auto wrist_torques = get_wrist_torques();

        // Log wrist data at reduced rate
        if (debug_enabled_ && (monitor_iterations % 25 == 0)) {
          std::cout << "🦾 Wrist [" << monitor_iterations << "]: ";
          std::cout << "Pos[7]=" << std::fixed << std::setprecision(3)
                    << wrist_positions[0] << " ("
                    << (wrist_positions[0] * 180.0 / M_PI) << "°), ";
          std::cout << "Pos[8]=" << wrist_positions[1] << " ("
                    << (wrist_positions[1] * 180.0 / M_PI) << "°)";
          std::cout << std::endl;
        }

        // Store wrist monitoring data
        {
          std::lock_guard<std::mutex> lock(wrist_monitor_mutex_);
          last_wrist_positions_ = wrist_positions;
          last_wrist_velocities_ = wrist_velocities;
          last_wrist_torques_ = wrist_torques;
          last_wrist_update_time_ = std::chrono::steady_clock::now();
        }

        monitor_iterations++;

        // Performance reporting
        auto current_time = std::chrono::steady_clock::now();
        if (current_time >= report_time) {
          auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                             current_time - last_time)
                             .count();
          double actual_frequency =
              static_cast<double>(monitor_iterations) / elapsed;

          std::cout << "🦾 Wrist Monitor Performance: " << std::fixed
                    << std::setprecision(1) << actual_frequency
                    << " Hz (target: " << wrist_monitor_frequency_ << "Hz)"
                    << std::endl;

          monitor_iterations = 0;
          last_time = current_time;
          report_time = current_time + std::chrono::seconds(5);
        }

        // Sleep to maintain frequency
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        auto sleep_time = target_period - elapsed;

        if (sleep_time.count() > 0) {
          std::this_thread::sleep_for(sleep_time);
        }
      }
    });

    std::cout << "✅ Wrist position monitoring started" << std::endl;
    return true;
  }

  void stop_wrist_position_monitoring() {
    std::cout << "🛑 Stopping wrist position monitoring..." << std::endl;
    wrist_monitor_running_ = false;
    if (wrist_monitor_thread_.joinable()) {
      wrist_monitor_thread_.join();
    }
    std::cout << "✅ Wrist position monitoring stopped" << std::endl;
  }

  bool refresh_wrist_motors_only() {
    if (!connected_ || !device_) {
      return false;
    }

    try {
      // Send HT read state command to refresh wrist motors 7 and 8
      send_ht_read_state();

      if (debug_enabled_) {
        std::cout << "📤 Refreshed wrist motors (7, 8)" << std::endl;
      }

      return true;
    } catch (const std::exception &e) {
      std::cout << "❌ Error refreshing wrist motors: " << e.what()
                << std::endl;
      return false;
    }
  }

  std::vector<double> get_wrist_positions() {
    auto all_positions = get_joint_positions();
    return {all_positions[6],
            all_positions[7]}; // Motors 7 and 8 (0-indexed as 6, 7)
  }

  std::vector<double> get_wrist_velocities() {
    auto all_velocities = get_joint_velocities();
    return {all_velocities[6], all_velocities[7]};
  }

  std::vector<double> get_wrist_torques() {
    auto all_torques = get_joint_torques();
    return {all_torques[6], all_torques[7]};
  }

  std::map<std::string, std::vector<double>> get_wrist_monitoring_data() {
    std::lock_guard<std::mutex> lock(wrist_monitor_mutex_);

    auto now = std::chrono::steady_clock::now();
    auto time_since_update =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_wrist_update_time_)
            .count();

    return {{"positions", last_wrist_positions_},
            {"velocities", last_wrist_velocities_},
            {"torques", last_wrist_torques_},
            {"update_age_ms", {static_cast<double>(time_since_update)}}};
  }

  void print_wrist_status() {
    auto wrist_data = get_wrist_monitoring_data();
    auto positions = wrist_data["positions"];
    auto velocities = wrist_data["velocities"];
    auto torques = wrist_data["torques"];
    auto update_age = wrist_data["update_age_ms"][0];

    std::cout << "\n=== Wrist Status Monitor ===" << std::endl;
    std::cout << "Motor 7 (Pitch): " << std::fixed << std::setprecision(3)
              << positions[0] << " rad (" << (positions[0] * 180.0 / M_PI)
              << "°)" << std::endl;
    std::cout << "Motor 8 (Roll):  " << std::fixed << std::setprecision(3)
              << positions[1] << " rad (" << (positions[1] * 180.0 / M_PI)
              << "°)" << std::endl;
    std::cout << "Velocities: [" << velocities[0] << ", " << velocities[1]
              << "] rad/s" << std::endl;
    std::cout << "Torques: [" << torques[0] << ", " << torques[1] << "] Nm"
              << std::endl;
    std::cout << "Last update: " << update_age << " ms ago" << std::endl;

    if (update_age > 100) {
      std::cout << "⚠️ Warning: Stale wrist data!" << std::endl;
    }
    std::cout << "=========================" << std::endl;
  }

  bool is_wrist_monitoring_running() const { return wrist_monitor_running_; }

  bool is_control_running() const { return control_running_; }

  void
  set_target_positions_interpolated(const std::vector<double> &target_positions,
                                    double max_velocity) {
    if (target_positions.size() < 9) {
      std::cout << "❌ Target positions must have at least 9 elements"
                << std::endl;
      return;
    }

    std::lock_guard<std::mutex> lock(interpolation_mutex_);
    target_positions_ = target_positions;
    max_velocity_ = max_velocity;
    std::cout << "🎯 [DEBUG] set_target_positions_interpolated: size="
              << target_positions_.size() << ", max_velocity=" << max_velocity_
              << std::endl;
  }

  static std::vector<double>
  interpolate_positions_static(const std::vector<double> &current_positions,
                               const std::vector<double> &target_positions,
                               double dt, double max_velocity) {
    if (current_positions.size() != target_positions.size()) {
      return current_positions;
    }

    std::vector<double> interpolated_positions(current_positions.size());
    double max_step = max_velocity * dt;

    for (size_t i = 0; i < current_positions.size(); ++i) {
      double error = target_positions[i] - current_positions[i];
      double step = std::clamp(error, -max_step, max_step);
      interpolated_positions[i] = current_positions[i] + step;
    }

    return interpolated_positions;
  }

  std::map<std::string, std::string> get_system_status() {
    return {{"connected", connected_ ? "true" : "false"},
            {"hf_control", hf_control_running_ ? "running" : "stopped"},
            {"device_sn", device_sn_},
            {"motors_count", "9"},
            {"damiao_motors", "6"},
            {"ht_motors", "2"},
            {"servo_motors", "1"}};
  }

  // Motor gain management methods
  bool set_motor_gains(int motor_id, double kp, double kd) {
    if (motor_id < 1 || motor_id > 9) {
      std::cout << "❌ Invalid motor ID: " << motor_id << " (must be 1-9)"
                << std::endl;
      return false;
    }

    std::lock_guard<std::mutex> lock(motor_gains_mutex_);
    motor_kp_gains_[motor_id - 1] = kp;
    motor_kd_gains_[motor_id - 1] = kd;

    if (debug_enabled_) {
      std::cout << "✅ Set motor " << motor_id << " gains: kp=" << kp
                << ", kd=" << kd << std::endl;
    }
    return true;
  }

  bool set_all_motor_gains(const std::vector<double> &kp_values,
                           const std::vector<double> &kd_values) {
    if (kp_values.size() < 9 || kd_values.size() < 9) {
      std::cout << "❌ Gain vectors must have at least 9 elements" << std::endl;
      return false;
    }

    std::lock_guard<std::mutex> lock(motor_gains_mutex_);
    for (int i = 0; i < 9; i++) {
      motor_kp_gains_[i] = kp_values[i];
      motor_kd_gains_[i] = kd_values[i];
    }

    if (debug_enabled_) {
      std::cout << "✅ Set all motor gains" << std::endl;
    }
    return true;
  }

  bool get_motor_gains(int motor_id, double &kp, double &kd) {
    if (motor_id < 1 || motor_id > 9) {
      std::cout << "❌ Invalid motor ID: " << motor_id << " (must be 1-9)"
                << std::endl;
      return false;
    }

    std::lock_guard<std::mutex> lock(motor_gains_mutex_);
    kp = motor_kp_gains_[motor_id - 1];
    kd = motor_kd_gains_[motor_id - 1];
    return true;
  }

  // Gravity compensation configuration
  bool enable_gravity_compensation() {
    if (!torque_predictor_ || !torque_predictor_->is_initialized()) {
      std::cout << "❌ Cannot enable gravity compensation - torque predictor "
                   "not initialized"
                << std::endl;
      return false;
    }

    gravity_compensation_enabled_ = true;
    std::cout << "✅ Gravity compensation enabled" << std::endl;
    return true;
  }

  bool disable_gravity_compensation() {
    gravity_compensation_enabled_ = false;
    std::cout << "✅ Gravity compensation disabled" << std::endl;
    return true;
  }

  bool is_gravity_compensation_enabled() const {
    return gravity_compensation_enabled_;
  }

  // Friction compensation configuration
  bool enable_friction_compensation() {
    friction_compensation_enabled_ = true;
    std::cout << "✅ Friction compensation enabled" << std::endl;
    return true;
  }

  bool disable_friction_compensation() {
    friction_compensation_enabled_ = false;
    std::cout << "✅ Friction compensation disabled" << std::endl;
    return true;
  }

  bool is_friction_compensation_enabled() const {
    return friction_compensation_enabled_;
  }

  std::vector<double>
  get_friction_compensation_torques(const std::vector<double> &velocities) {
    std::vector<double> friction_torques(9, 0.0);

    if (!friction_compensation_enabled_) {
      return friction_torques;
    }

    std::lock_guard<std::mutex> lock(friction_params_mutex_);

    for (int i = 0; i < 9 && i < velocities.size(); i++) {
      double velocity = velocities[i];
      const auto &params = friction_params_[i];

      // Check if velocity is above threshold
      if (std::abs(velocity) > params.velocity_threshold) {
        // Coulomb friction: Fc * sign(velocity)
        double coulomb_friction = params.Fc * ((velocity > 0) ? 1.0 : -1.0);

        // Viscous friction: Fv * velocity
        double viscous_friction = params.Fv * velocity;

        // Apply velocity damping if enabled
        if (velocity_damping_ > 0.0) {
          viscous_friction += velocity_damping_ * velocity;
        }

        // Smooth transition using sigmoid function if enabled
        double total_friction = coulomb_friction + viscous_friction;
        if (smooth_transition_) {
          double weight =
              std::tanh(std::abs(velocity) / params.velocity_threshold);
          total_friction *= weight;
        }

        friction_torques[i] = total_friction * 0.1;

        if (debug_enabled_ && i < 6) { // Only log for first 6 joints
          std::cout << "🔧 Joint " << i
                    << " friction torque: " << total_friction
                    << " (velocity: " << velocity << ")" << std::endl;
        }
      }
    }

    return friction_torques;
  }

  std::vector<double> get_gravity_compensation_torques() {
    std::vector<double> gravity_torques(9, 0.0);

    if (!gravity_compensation_enabled_ || !torque_predictor_ ||
        !torque_predictor_->is_initialized()) {
      return gravity_torques;
    }

    // Get current joint positions (only first 6 joints for arm dynamics)
    auto positions = get_joint_positions();
    if (positions.size() < 6) {
      return gravity_torques;
    }

    std::array<double, 6> q;
    std::array<double, 6> gravity_arm;

    for (int i = 0; i < 6; i++) {
      q[i] = positions[i];
    }

    if (torque_predictor_->predict_gravity_torque(q.data(),
                                                  gravity_arm.data())) {
      // Copy gravity torques for first 6 arm joints
      for (int i = 0; i < 6; i++) {
        gravity_torques[i] = gravity_arm[i];
      }

      if (debug_enabled_) {
        std::cout << "🔧 Gravity torques (N⋅m): ";
        for (int i = 0; i < 6; i++) {
          std::cout << std::fixed << std::setprecision(3) << gravity_torques[i]
                    << " ";
        }
        std::cout << std::endl;
      }
    }

    return gravity_torques;
  }

  std::vector<double> get_all_predicted_torques() {
    std::vector<double> total_torques(9, 0.0);

    if (!torque_predictor_ || !torque_predictor_->is_initialized()) {
      std::cout << "❌ Torque predictor not initialized" << std::endl;
      return total_torques;
    }

    // Get current joint state
    auto positions = get_joint_positions();
    auto velocities = get_joint_velocities();

    if (positions.size() < 6 || velocities.size() < 6) {
      std::cout << "❌ Insufficient joint state data" << std::endl;
      return total_torques;
    }

    // Use zero accelerations for current torque prediction
    std::array<double, 6> q, dq, ddq;
    std::array<double, 6> predicted_torques;

    for (int i = 0; i < 6; i++) {
      q[i] = positions[i];
      dq[i] = velocities[i];
      ddq[i] = 0.0; // Zero acceleration for steady-state
    }

    if (torque_predictor_->predict_total_torque(q.data(), dq.data(), ddq.data(),
                                                predicted_torques.data())) {
      // Copy predicted torques for first 6 arm joints
      for (int i = 0; i < 6; i++) {
        total_torques[i] = predicted_torques[i];
      }

      if (debug_enabled_) {
        std::cout << "🔧 Predicted torques (N⋅m): ";
        for (int i = 0; i < 6; i++) {
          std::cout << std::fixed << std::setprecision(3) << total_torques[i]
                    << " ";
        }
        std::cout << std::endl;
      }
    }

    return total_torques;
  }

  // Torque prediction method switching
  bool switch_torque_prediction_method(int method_id) {
    if (!torque_predictor_ || !torque_predictor_->is_initialized()) {
      std::cout << "❌ Cannot switch method - torque predictor not initialized"
                << std::endl;
      return false;
    }

    TorquePredictionMethod method;
    switch (method_id) {
    case 0:
      method = TorquePredictionMethod::PINOCCHIO_URDF;
      break;
    case 1:
      method = TorquePredictionMethod::PURE_C_MATLAB;
      break;
    case 2:
      method = TorquePredictionMethod::REGRESSOR_BASED;
      break;
    default:
      std::cout << "❌ Invalid method ID: " << method_id
                << " (0=Pinocchio URDF, 1=Pure C MATLAB, 2=Regressor-Based)"
                << std::endl;
      return false;
    }

    return torque_predictor_->switch_method(method);
  }

  void print_torque_method_status() {
    if (torque_predictor_ && torque_predictor_->is_initialized()) {
      torque_predictor_->print_method_status();
    } else {
      std::cout << "❌ Torque predictor not initialized" << std::endl;
    }
  }

  void print_torque_breakdown() {
    if (!torque_predictor_ || !torque_predictor_->is_initialized()) {
      std::cout << "❌ Torque predictor not initialized" << std::endl;
      return;
    }

    auto positions = get_joint_positions();
    auto velocities = get_joint_velocities();

    if (positions.size() < 6 || velocities.size() < 6) {
      std::cout << "❌ Insufficient joint state data for torque breakdown"
                << std::endl;
      return;
    }

    std::array<double, 6> q, dq;
    std::array<double, 6> ddq; // Zero accelerations for current state

    for (int i = 0; i < 6; i++) {
      q[i] = positions[i];
      dq[i] = velocities[i];
      ddq[i] = 0.0;
    }

    torque_predictor_->print_torque_breakdown(q.data(), dq.data(), ddq.data());
  }

  void print_system_info() {
    std::cout << "\n=== IC_CAN System Information ===" << std::endl;
    std::cout << "Device SN: " << device_sn_ << std::endl;
    std::cout << "Connected: " << (connected_ ? "Yes" : "No") << std::endl;
    std::cout << "Torque Predictor: "
              << (torque_predictor_ && torque_predictor_->is_initialized()
                      ? "Available"
                      : "Unavailable")
              << std::endl;
    std::cout << "Gravity Compensation: "
              << (gravity_compensation_enabled_ ? "Enabled" : "Disabled")
              << std::endl;
    std::cout << "Friction Compensation: "
              << (friction_compensation_enabled_ ? "Enabled" : "Disabled")
              << std::endl;
    /*std::cout << "Motors: 9 total (6 Damiao + 2 HT + 1 Servo)" <<
     * std::endl;*/
    std::cout << "Control Frequency: 500Hz" << std::endl;
    std::cout << "Debug: " << (debug_enabled_ ? "Enabled" : "Disabled")
              << std::endl;
  }

  void print_friction_compensation_status() {
    std::cout << "\n=== Friction Compensation Status ===" << std::endl;
    std::cout << "Friction Compensation: "
              << (friction_compensation_enabled_ ? "Enabled" : "Disabled")
              << std::endl;
    std::cout << "Velocity Damping: " << velocity_damping_ << std::endl;
    std::cout << "Smooth Transition: "
              << (smooth_transition_ ? "Enabled" : "Disabled") << std::endl;
    std::cout << "Sign Threshold: " << sgn_threshold_ << std::endl;

    if (friction_compensation_enabled_) {
      std::cout << "\n📋 Friction Parameters:" << std::endl;
      std::lock_guard<std::mutex> lock(friction_params_mutex_);
      for (int i = 0; i < 9; i++) {
        std::cout << "  Joint " << i << ": Fc=" << friction_params_[i].Fc
                  << " Nm, Fv=" << friction_params_[i].Fv
                  << " Nm/(rad/s), threshold="
                  << friction_params_[i].velocity_threshold << " rad/s"
                  << std::endl;
      }
    }

    // Show current friction compensation values
    auto velocities = get_joint_velocities();
    auto friction_torques = get_friction_compensation_torques(velocities);
    std::cout << "\n🔧 Current Friction Compensation Torques:" << std::endl;
    for (int i = 0; i < 6; i++) { // Show first 6 joints
      std::cout << "  Joint " << i << ": " << friction_torques[i] << " Nm"
                << " (velocity: " << velocities[i] << " rad/s)" << std::endl;
    }
  }

private:
  // Component instances
  std::unique_ptr<WristComponent> wrist_component_;
  std::unique_ptr<GripperComponent> gripper_component_;
  std::unique_ptr<ArmComponent> arm_component_;

  // Communication architecture (temporarily disabled)
  // std::unique_ptr<USB2CANCommunicationAdapter> communication_adapter_;
  // std::unique_ptr<CANFrameDispatcher> can_dispatcher_;

  // Torque predictor instance - using stub implementation
  std::unique_ptr<TorquePredictorUnified> torque_predictor_;
  bool gravity_compensation_enabled_;

  // Friction compensation
  bool friction_compensation_enabled_;
  double velocity_damping_;
  bool smooth_transition_;
  double sgn_threshold_;
  std::array<FrictionParams, 9> friction_params_;
  std::mutex friction_params_mutex_;

  void handle_can_frame(can_value_type &frame) {
    uint32_t can_id = frame.head.id;
    // Track receive frequency - count ALL frames
    receive_count_++;
    total_bytes_received_ += frame.head.dlc;

    if (debug_enabled_) {
      std::cout << "📥 RECV: ID=0x" << std::hex << can_id << std::dec << " ("
                << receive_count_.load() << " total)" << std::endl;
    }
    // Debug: Print ALL received CAN frames when debug mode is enabled
    if (debug_enabled_) {
      std::cout << "📥 CAN RECV: ID=0x" << std::hex << can_id << std::dec
                << ", DLC=" << (int)frame.head.dlc << ", Data: ";

      // Print ALL data bytes in the frame (up to 64 bytes for safety)
      for (int i = 0; i < frame.head.dlc && i < 64; i++) {
        std::cout << std::hex << "0x" << std::setw(2) << std::setfill('0')
                  << (int)frame.data[i] << " ";
      }
      std::cout << std::dec << std::endl;

      // Print frame timestamp and additional details
      auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now().time_since_epoch())
                           .count();
      std::cout << "   Timestamp: " << timestamp << " ms" << std::endl;

      // Print raw frame structure details
      std::cout << "   Frame Structure:" << std::endl;
      std::cout << "     CAN ID: 0x" << std::hex << can_id << std::dec << " ("
                << can_id << ")" << std::endl;
      std::cout << "     DLC: " << (int)frame.head.dlc << " bytes" << std::endl;
      std::cout << "     Data Bytes (" << frame.head.dlc << "): ";
      for (int i = 0; i < frame.head.dlc; i++) {
        std::cout << "[" << i << "]=0x" << std::hex << std::setw(2)
                  << std::setfill('0') << (int)frame.data[i] << std::dec << " ";
      }
      std::cout << std::endl;

      // Check if this is an HT motor frame
      if (can_id == 0x700) {
        std::cout << "   → This is HT motor 7 (pitch) feedback!" << std::endl;
      } else if (can_id == 0x800) {
        std::cout << "   → This is HT motor 8 (roll) feedback!" << std::endl;
      } else if (can_id >= 0x11 && can_id <= 0x16) {
        std::cout << "   → This is DM motor " << (can_id - 0x10) << " feedback!"
                  << std::endl;
      } else if (can_id == 0x19) {
        std::cout << "   → This is Servo motor 9 feedback!" << std::endl;
      } else {
        std::cout << "   → Unknown CAN ID, not handled by any component!"
                  << std::endl;
      }
    }

    // Handle Damiao motor feedback (motors 1-6)
    if (can_id >= 0x11 && can_id <= 0x16) {
      process_dm_motor_feedback(frame, can_id - 1 - 0x10);
    }
    // Handle HT motor feedback (motors 7-8)
    else if (can_id == 0x700 || can_id == 0x800) {
      process_ht_motor_feedback(frame,
                                can_id == 0x700 ? 6 : 7); // Map to motors 7-8
    }
  }

  void process_dm_motor_feedback(can_value_type &frame, int motor_idx) {
    if (frame.head.dlc < 6)
      return;

    // Extract motor response data (same as working test)
    uint16_t q_uint = (uint16_t(frame.data[1]) << 8) | frame.data[2];
    uint16_t dq_uint = (uint16_t(frame.data[3]) << 4) | (frame.data[4] >> 4);
    uint16_t tau_uint = (uint16_t(frame.data[4] & 0xf) << 8) | frame.data[5];

    // Use correct limits for each motor type
    double p_max, v_max, t_max;
    if (motor_idx == 0) { // Motor 1 - DM10010L
      p_max = 12.5;
      v_max = 25.0;
      t_max = 200.0;
    } else if (motor_idx == 1 || motor_idx == 2) { // Motors 2-3 - DM6248
      p_max = 12.566;
      v_max = 20.0;
      t_max = 120.0;
    } else if (motor_idx == 3 || motor_idx == 4) { // Motors 4-5 - DM4340
      p_max = 12.5;
      v_max = 10.0;
      t_max = 28.0;
    } else { // Motor 6 - DM4310
      p_max = 12.5;
      v_max = 30.0;
      t_max = 10.0;
    }

    // Convert to real values
    auto uint_to_float = [](uint16_t x, float xmin, float xmax,
                            uint8_t bits) -> float {
      float span = xmax - xmin;
      float data_norm = float(x) / ((1 << bits) - 1);
      float data = data_norm * span + xmin;
      return data;
    };

    double position = uint_to_float(q_uint, -p_max, p_max, 16);
    double velocity = uint_to_float(dq_uint, -v_max, v_max, 12);
    double torque = uint_to_float(tau_uint, -t_max, t_max, 12);

    // Update atomic values
    positions_[motor_idx].store(position);
    velocities_[motor_idx].store(velocity);
    torques_[motor_idx].store(torque);
  }

  void process_ht_motor_feedback(can_value_type &frame, int motor_idx) {
    if (frame.head.dlc < 7)
      return;

    // Debug: Print HT motor processing
    if (debug_enabled_) {
      std::cout << "🔧 Processing HT motor feedback for motor idx: "
                << motor_idx << " (actual motor " << (motor_idx + 1) << ")"
                << std::endl;
    }

    // Extract HT motor data (same as HT test)
    int16_t pos_int =
        static_cast<int16_t>(frame.data[2] | (frame.data[3] << 8));
    int16_t vel_int =
        static_cast<int16_t>(frame.data[4] | (frame.data[5] << 8));
    int16_t torque_int =
        static_cast<int16_t>(frame.data[6] | (frame.data[7] << 8));

    // Convert to physical units (HT protocol)
    const double TURN_TO_RAD = 2.0 * M_PI;
    double position = pos_int * 0.0001 * TURN_TO_RAD;
    double velocity = vel_int * 0.00025 * TURN_TO_RAD;
    const double torque_k = 0.004855;
    const double torque_d = -0.083;
    double torque = torque_int * torque_k + torque_d;

    if (debug_enabled_) {
      std::cout << "   Raw pos_int: " << pos_int << " → position: " << position
                << " rad (" << (position * 180.0 / M_PI) << "°)" << std::endl;
      std::cout << "   Raw vel_int: " << vel_int << " → velocity: " << velocity
                << " rad/s" << std::endl;
      std::cout << "   Raw torque_int: " << torque_int
                << " → torque: " << torque << " Nm" << std::endl;
    }

    // Update atomic values
    positions_[motor_idx].store(position);
    velocities_[motor_idx].store(velocity);
    torques_[motor_idx].store(torque);

    // Forward to wrist component if this is motor 7 or 8
    if (wrist_component_ && (motor_idx == 6 || motor_idx == 7)) {
      int wrist_motor_id =
          (motor_idx == 6) ? 7 : 8; // Convert to wrist motor IDs

      // Create CAN frame for wrist component
      CANFrame wrist_frame;
      wrist_frame.id = (motor_idx == 6) ? 0x700 : 0x800;
      wrist_frame.data.resize(frame.head.dlc);
      for (int i = 0; i < frame.head.dlc; i++) {
        wrist_frame.data[i] = frame.data[i];
      }

      if (debug_enabled_) {
        std::cout << "📤 Forwarding HT motor " << wrist_motor_id
                  << " data to wrist component (CAN ID: 0x" << std::hex
                  << wrist_frame.id << std::dec << ")" << std::endl;
      }

      // Send to wrist component
      wrist_component_->process_can_frame(wrist_frame);
    }
  }

  void send_dm_mit_command(int motor_id, double position, double velocity,
                           double torque, double kp, double kd) {
    // Track send frequency
    /*std::cout << motor_id << std::endl;*/
    auto float_to_uint = [](double x, double min, double max,
                            int bits) -> uint16_t {
      double span = max - min;
      if (x < min)
        x = min;
      if (x > max)
        x = max;
      return static_cast<uint16_t>((x - min) * ((1 << bits) - 1) / span);
    };

    // Determine correct limits for motor
    double p_max, v_max, t_max;
    if (motor_id == 1) {
      p_max = 12.5;
      v_max = 25.0;
      t_max = 200.0;
    } else if (motor_id == 2 || motor_id == 3) {
      p_max = 12.566;
      v_max = 20.0;
      t_max = 120.0;
    } else if (motor_id == 4 || motor_id == 5) {
      p_max = 12.5;
      v_max = 10.0;
      t_max = 28.0;
    } else {
      p_max = 12.5;
      v_max = 30.0;
      t_max = 10.0;
    }

    uint16_t kp_uint = float_to_uint(kp, 0.0, 500.0, 12);
    uint16_t kd_uint = float_to_uint(kd, 0.0, 5.0, 12);
    uint16_t q_uint = float_to_uint(position, -p_max, p_max, 16);
    uint16_t dq_uint = float_to_uint(velocity, -v_max, v_max, 12);
    uint16_t tau_uint = float_to_uint(torque, -t_max, t_max, 12);

    // Pack DM MIT command
    std::vector<uint8_t> data(8, 0);
    data[0] = (q_uint >> 8) & 0xFF;
    data[1] = q_uint & 0xFF;
    data[2] = dq_uint >> 4;
    data[3] = ((dq_uint & 0xF) << 4) | ((kp_uint >> 8) & 0xF);
    data[4] = kp_uint & 0xFF;
    data[5] = kd_uint >> 4;
    data[6] = ((kd_uint & 0xF) << 4) | ((tau_uint >> 8) & 0xF);
    data[7] = tau_uint & 0xFF;

    // Send using universal CAN frame function (standard frame for Damiao)
    send_can_frame(motor_id, data, false);

    /*print_send_info(motor_id, data);*/
    /*usleep(200);*/
  }
  void print_send_info(int motor_id, const std::vector<uint8_t> &data) {
    std::cout << "Sending DM command for motor " << motor_id << " with ";

    for (int i = 0; i < 8; i++) {
      std::cout << "0x" << std::setw(2) << std::setfill('0') << std::hex
                << std::uppercase << static_cast<int>(data[i]) << " ";
    }
    std::cout << std::dec << std::endl;
    return;
  }

  void send_ht_brake_command() {
    // Send HT brake command based on Python implementation
    // Format: [0x01, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00] (8 bytes)
    // CAN ID: (0x80 | id) << 8 | id, where id is 7 or 8
    std::vector<uint8_t> data = {0x01, 0x00, 0x0f, 0x00,
                                 0x00, 0x00, 0x00, 0x00};

    // Send to both HT motors 7 and 8
    for (int motor_id = 7; motor_id <= 8; motor_id++) {
      uint32_t can_id = ((0x80 | motor_id) << 8) | motor_id;

      total_bytes_sent_ += 8;
      std::cout << "📤 Sending HT BRAKE command to motor " << motor_id
                << std::endl;

      if (debug_enabled_) {
        std::cout << "   📤 CAN SEND: ID=0x" << std::hex << can_id << std::dec
                  << ", DLC=8, Data: ";
        for (int i = 0; i < 8; i++) {
          std::cout << std::hex << "0x" << (int)data[i] << " ";
        }
        std::cout << std::dec << std::endl;
      }

      send_can_frame(can_id, data, true); // Extended frame for HT brake
      if (debug_enabled_) {
        std::cout << "   ✅ HT BRAKE command sent to motor " << motor_id
                  << " successfully" << std::endl;
      }
    }
  }

  void send_ht_read_state() {
    std::cout << "📤 Reading state of two HT motors" << std::endl;

    // HT read state command format
    /*std::vector<unsigned char> cmd = {0x01, 0x00, 0x00, 0x11, 0x00, 0x1f,*/
    /*                                  0x01, 0x13, 0x0d, 0x50, 0x50, 0x50};*/
    /*std::vector<unsigned char> cmd = {0x15, 0x05, 0x02};*/
    std::vector<unsigned char> cmd = {0x17, 0x01, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00};

    // Send to HT motor 7 (CAN ID: 0x8707)
    send_can_frame(0x8007, cmd, true); // Extended frame for HT
    //
    // Send to HT motor 8 (CAN ID: 0x8808)
    send_can_frame(0x8008, cmd, true); // Extended frame for HT

    if (debug_enabled_) {
      std::cout << "   📤 CAN SEND: ID=0x8007, DLC=8, Data: ";
      for (size_t i = 0; i < cmd.size(); i++) {
        std::cout << std::hex << "0x" << (int)cmd[i] << " ";
      }
      std::cout << std::dec << " (HT Motor 7)" << std::endl;

      std::cout << "   📤 CAN SEND: ID=0x8008, DLC=8, Data: ";
      for (size_t i = 0; i < cmd.size(); i++) {
        std::cout << std::hex << "0x" << (int)cmd[i] << " ";
      }
      std::cout << std::dec << " (HT Motor 8)" << std::endl;

      std::cout << "   ✅ HT read state commands sent successfully"
                << std::endl;
    }
  }

  void send_ht_mit_command(double position, double velocity, double torque,
                           double kp, double kd) {
    // Placeholder implementation for HT MIT command (single motor interface)
    std::cout << "📤 HT MIT command called (placeholder) - pos=" << position
              << ", vel=" << velocity << ", tau=" << torque << std::endl;
    // This would need to be implemented based on your specific HT motor
    // protocol
  }

  void send_ht_set_zero_command() {
    std::cout << "📤 HT set zero command called (placeholder)" << std::endl;
    send_can_frame(0x8007, {0x40, 0x01, 0x04, 0x64, 0x20, 0x63, 0x0A}, true);
    send_can_frame(0x8008, {0x40, 0x01, 0x04, 0x64, 0x20, 0x63, 0x0A}, true);
  }

  // Universal CAN frame sending function
  void send_can_frame(uint32_t can_id, const std::vector<uint8_t> &data,
                      bool is_extended_frame = false) {
    if (!device_) {
      std::cout << "❌ Device not initialized for CAN frame sending"
                << std::endl;
      return;
    }

    can_tx_type frame;
    memset(&frame, 0, sizeof(can_tx_type));

    // Configure frame
    frame.can_id = can_id;
    frame.can_type = 1; // 2.0/fd
    frame.fd_acc = 1;
    frame.fram_type = 1; // 数据帧
    frame.id_type =
        is_extended_frame ? 1 : 0; // 扩展帧 for HT, 标准帧 for Damiao
    frame.id_increase = 0;
    frame.data_increase = 0;
    frame.cmd = 0; // 0是开始
    frame.send_time = 1;
    frame.interval = 0;

    // Copy data to frame
    uint8_t *dest = frame.data;
    uint8_t *src = const_cast<uint8_t *>(data.data());
    int dataLength = static_cast<int>(data.size());
    int copySize =
        (dataLength > sizeof(frame.data)) ? sizeof(frame.data) : dataLength;
    memcpy(dest, src, copySize);
    frame.dlc = dataLength;

    // Send frame using device's methods
    device_->set_tx_frame(&frame);
    device_->send_data();

    total_bytes_sent_ += dataLength;
  }

  void send_ht_mit_command_with_movement(double m7_position, double m7_velocity,
                                         double m7_torque, double m8_position,
                                         double m8_velocity, double m8_torque,
                                         double kp, double kd) {
    // Track send frequency
    std::cout
        << "📤 Sending HT MIT command - Movement mode (Independent motors)"
        << std::endl;
    total_bytes_sent_ += 24; // HT command is 24 bytes
    // HT motor conversion constants
    const double RAD_TO_TURN = 1.0 / (2.0 * M_PI);
    const double torque_k = 0.004855;
    const double torque_d = -0.083;

    // Convert Motor 7 to motor units
    double m7_pos_turns = m7_position * RAD_TO_TURN;
    double m7_vel_turns = m7_velocity * RAD_TO_TURN;
    int16_t m7_pos_int = static_cast<int16_t>(m7_pos_turns / 0.0001);
    int16_t m7_vel_int = static_cast<int16_t>(m7_vel_turns / 0.00025);
    int16_t m7_torque_int =
        static_cast<int16_t>((m7_torque - torque_d) / torque_k);

    // Convert Motor 8 to motor units
    double m8_pos_turns = m8_position * RAD_TO_TURN;
    double m8_vel_turns = m8_velocity * RAD_TO_TURN;
    int16_t m8_pos_int = static_cast<int16_t>(m8_pos_turns / 0.0001);
    int16_t m8_vel_int = static_cast<int16_t>(m8_vel_turns / 0.00025);
    int16_t m8_torque_int =
        static_cast<int16_t>((m8_torque - torque_d) / torque_k);

    // Convert gains to int16 format (same for both motors)
    int16_t kp_int = static_cast<int16_t>(kp * 10);
    int16_t kd_int = static_cast<int16_t>(kd * 10);

    // Pack HT MIT command (24 bytes split correctly)
    std::vector<uint8_t> data(24, 0);

    // First 10 bytes: Motor 7 MIT command
    data[0] = m7_pos_int & 0xFF;           // Motor 7 Position LSB
    data[1] = (m7_pos_int >> 8) & 0xFF;    // Motor 7 Position MSB
    data[2] = m7_vel_int & 0xFF;           // Motor 7 Velocity LSB
    data[3] = (m7_vel_int >> 8) & 0xFF;    // Motor 7 Velocity MSB
    data[4] = m7_torque_int & 0xFF;        // Motor 7 Torque LSB
    data[5] = (m7_torque_int >> 8) & 0xFF; // Motor 7 Torque MSB
    data[6] = kp_int & 0xFF;               // Motor 7 Kp LSB
    data[7] = (kp_int >> 8) & 0xFF;        // Motor 7 Kp MSB
    data[8] = kd_int & 0xFF;               // Motor 7 Kd LSB
    data[9] = (kd_int >> 8) & 0xFF;        // Motor 7 Kd MSB

    // Next 10 bytes: Motor 8 MIT command (independent data)
    data[10] = m8_pos_int & 0xFF;           // Motor 8 Position LSB
    data[11] = (m8_pos_int >> 8) & 0xFF;    // Motor 8 Position MSB
    data[12] = m8_vel_int & 0xFF;           // Motor 8 Velocity LSB
    data[13] = (m8_vel_int >> 8) & 0xFF;    // Motor 8 Velocity MSB
    data[14] = m8_torque_int & 0xFF;        // Motor 8 Torque LSB
    data[15] = (m8_torque_int >> 8) & 0xFF; // Motor 8 Torque MSB
    data[16] = kp_int & 0xFF;               // Motor 8 Kp LSB
    data[17] = (kp_int >> 8) & 0xFF;        // Motor 8 Kp MSB
    data[18] = kd_int & 0xFF;               // Motor 8 Kd LSB
    data[19] = (kd_int >> 8) & 0xFF;        // Motor 8 Kd MSB

    // Last 4 bytes: Position request flags
    data[20] = 0x50;
    data[21] = 0x50;
    data[22] = 0x17;
    data[23] = 0x01;

    if (debug_enabled_) {
      std::cout << "   Motor 7 - Position: " << m7_position << " rad ("
                << (m7_position * 180.0 / M_PI) << "°)" << std::endl;
      std::cout << "   Motor 8 - Position: " << m8_position << " rad ("
                << (m8_position * 180.0 / M_PI) << "°)" << std::endl;
      std::cout << "   HT Command Structure:" << std::endl;
      std::cout << "   - Motor 7 (bytes 0-9): ";
      for (int i = 0; i < 10; i++) {
        std::cout << std::hex << "0x" << std::setw(2) << std::setfill('0')
                  << (int)data[i] << " ";
      }
      std::cout << std::dec << std::endl;
      std::cout << "   - Motor 8 (bytes 10-19): ";
      for (int i = 10; i < 20; i++) {
        std::cout << std::hex << "0x" << std::setw(2) << std::setfill('0')
                  << (int)data[i] << " ";
      }
      std::cout << std::dec << std::endl;
      std::cout << "   - Flags (bytes 20-23): 0x" << std::hex << (int)data[20]
                << " 0x" << (int)data[21] << " 0x" << (int)data[22] << " 0x"
                << (int)data[23] << std::dec << std::endl;
    }

    // Send using universal CAN frame function (extended frame for HT)
    send_can_frame(0x8094, data, true); // HT uses extended frame
  }

  std::string device_sn_;
  bool debug_enabled_;
  bool connected_;
  std::unique_ptr<usb_class> device_;

  std::atomic<double> positions_[9];
  std::atomic<double> velocities_[9];
  std::atomic<double> torques_[9];

  std::mutex positions_mutex_;
  std::mutex velocities_mutex_;
  std::mutex torques_mutex_;

  // Motor-specific P and D gains (9 motors: 6 DM + 2 HT + 1 Servo)
  std::array<double, 9> motor_kp_gains_;
  std::array<double, 9> motor_kd_gains_;
  std::mutex motor_gains_mutex_;

  // High-frequency control
  std::atomic<bool> hf_control_running_;
  std::thread hf_control_thread_;

  // Configurable control loop
  std::atomic<bool> control_running_;
  std::thread control_thread_;
  double control_frequency_;

  // Interpolation state
  std::vector<double> target_positions_;
  std::vector<double> current_positions_;
  double max_velocity_;
  std::mutex interpolation_mutex_;

  // Pre-computed trajectory
  std::vector<std::vector<double>> trajectory_points_;
  size_t current_trajectory_index_;

  // Generate complete trajectory offline
  std::vector<std::vector<double>>
  generate_trajectory_offline(const std::vector<double> &start_positions,
                              const std::vector<double> &target_positions,
                              double dt, double max_velocity) {

    std::vector<std::vector<double>> trajectory;

    // Safety checks
    if (start_positions.size() == 0 || target_positions.size() == 0) {
      std::cout << "❌ ERROR: Empty position vectors" << std::endl
                << "start position size " << start_positions.size()
                << " target position size " << target_positions.size()
                << std::endl;
      return trajectory;
    }

    if (start_positions.size() != target_positions.size()) {
      std::cout << "❌ ERROR: Position size mismatch - start: "
                << start_positions.size()
                << ", target: " << target_positions.size() << std::endl;
      return trajectory;
    }

    std::vector<double> current_pos = start_positions;
    std::vector<double> target_pos = target_positions;

    std::cout << "📐 Generating trajectory from: ";
    for (size_t i = 0; i < current_pos.size() && i < 9; ++i) {
      std::cout << std::fixed << std::setprecision(3) << current_pos[i] << " ";
    }
    std::cout << "\n📐 Target positions: ";
    for (size_t i = 0; i < target_pos.size() && i < 9; ++i) {
      std::cout << std::fixed << std::setprecision(3) << target_pos[i] << " ";
    }
    std::cout << std::endl;

    // Generate trajectory points until we reach the target
    int max_steps = 1000; // Prevent infinite loops
    int step_count = 0;

    while (step_count < max_steps) {
      trajectory.push_back(current_pos);

      // Check if we've reached the target
      bool reached_target = true;
      for (size_t i = 0; i < current_pos.size() && i < target_pos.size(); ++i) {
        double error = std::abs(target_pos[i] - current_pos[i]);
        if (error > 0.001) { // 0.001 rad tolerance
          reached_target = false;
          break;
        }
      }

      if (reached_target) {
        trajectory.push_back(target_pos); // Ensure final position is exact
        break;
      }

      // Compute next position using velocity-limited interpolation
      std::vector<double> next_pos = current_pos;
      double max_step = max_velocity * dt;

      // Ensure vectors have consistent size
      if (next_pos.size() != current_pos.size() ||
          next_pos.size() != target_pos.size()) {
        next_pos.resize(current_pos.size());
        target_pos.resize(current_pos.size());
      }

      for (size_t i = 0; i < current_pos.size() && i < target_pos.size(); ++i) {
        double error = target_pos[i] - current_pos[i];
        double step = std::clamp(error, -max_step, max_step);
        next_pos[i] = current_pos[i] + step;
      }

      current_pos = next_pos;
      step_count++;
    }

    std::cout << "📊 Trajectory generated with " << trajectory.size()
              << " points over " << step_count << " steps" << std::endl;

    return trajectory;
  }

  // Frequency monitoring and logging
  std::atomic<bool> logging_running_;
  std::atomic<bool> performance_monitoring_;
  std::thread logger_thread_;
  std::thread performance_thread_;
  std::string logging_directory_;
  std::mutex logging_mutex_;

  // Performance counters
  std::atomic<uint64_t> send_count_;
  std::atomic<uint64_t> receive_count_;
  std::atomic<uint64_t> total_bytes_sent_;
  std::atomic<uint64_t> total_bytes_received_;
  std::chrono::high_resolution_clock::time_point performance_start_time_;
  std::mutex performance_mutex_;

  // Logging state
  std::chrono::high_resolution_clock::time_point logging_start_time_;
  std::vector<double> last_sent_positions_;
  std::vector<double> last_sent_velocities_;
  std::vector<double> last_sent_torques_;

  // Logger thread function
  void logger_thread_function();

  // Performance monitor thread function
  void performance_monitor_thread_function();

  // Wrist monitoring member variables
  std::atomic<bool> wrist_monitor_running_;
  std::thread wrist_monitor_thread_;
  double wrist_monitor_frequency_;
  std::mutex wrist_monitor_mutex_;

  // Wrist monitoring data storage
  std::vector<double> last_wrist_positions_;
  std::vector<double> last_wrist_velocities_;
  std::vector<double> last_wrist_torques_;
  std::chrono::steady_clock::time_point last_wrist_update_time_;

  // Get current timestamp as ISO string
  std::string get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
  }
};

// Implementation of logger thread function
void IC_CAN::Impl::logger_thread_function() {
  // Ensure logging directory exists
  if (std::system(("mkdir -p " + logging_directory_).c_str()) != 0) {
    std::cout << "❌ Failed to create logging directory: " << logging_directory_
              << std::endl;
    return;
  }

  // Open CSV files for writing
  std::string joint_commands_path = logging_directory_ + "/joint_commands.csv";
  std::string motor_states_path = logging_directory_ + "/motor_states.csv";

  std::ofstream joint_commands_file(joint_commands_path);
  std::ofstream motor_states_file(motor_states_path);

  if (!joint_commands_file.is_open() || !motor_states_file.is_open()) {
    std::cout << "❌ Failed to open log files:" << std::endl;
    std::cout << "   joint_commands: " << joint_commands_path << " ("
              << (joint_commands_file.is_open() ? "open" : "failed") << ")"
              << std::endl;
    std::cout << "   motor_states: " << motor_states_path << " ("
              << (motor_states_file.is_open() ? "open" : "failed") << ")"
              << std::endl;
    return;
  }

  // Write CSV headers based on the log example format
  joint_commands_file << "timestamp";
  motor_states_file << "timestamp";

  for (int i = 1; i <= 9; i++) {
    joint_commands_file << ",target_position_motor_" << i
                        << ",target_velocity_motor_" << i
                        << ",target_torque_motor_" << i;
    motor_states_file << ",position_motor_" << i << ",velocity_motor_" << i
                      << ",torque_motor_" << i;
  }
  joint_commands_file << std::endl;
  motor_states_file << std::endl;

  std::cout << "✅ Logger thread started, writing to CSV files" << std::endl;

  std::vector<double> last_sent_positions(9, 0.0);
  std::vector<double> last_sent_velocities(9, 0.0);
  std::vector<double> last_sent_torques(9, 0.0);
  bool has_sent_data = false;

  while (logging_running_) {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(1000 / 400)); // Log at 400Hz

    try {
      std::string timestamp = get_current_timestamp();

      // Log motor states (actual positions/velocities/torques)
      auto current_positions = get_joint_positions();
      auto current_velocities = get_joint_velocities();
      auto current_torques = get_joint_torques();

      motor_states_file << timestamp;
      for (int i = 0; i < 9; i++) {
        motor_states_file << "," << std::fixed << std::setprecision(12)
                          << current_positions[i] << "," << std::fixed
                          << std::setprecision(12) << current_velocities[i]
                          << "," << std::fixed << std::setprecision(12)
                          << current_torques[i];
      }
      motor_states_file << std::endl;

      // Log the actually sent commands (what was actually sent to motors)
      {
        std::lock_guard<std::mutex> lock(logging_mutex_);
        if (!last_sent_positions_.empty()) {
          last_sent_positions = last_sent_positions_;
          last_sent_velocities = last_sent_velocities_;
          last_sent_torques = last_sent_torques_;
          has_sent_data = true;
        }
      }

      if (has_sent_data) {
        joint_commands_file << timestamp;
        for (int i = 0; i < 9; i++) {
          joint_commands_file << "," << std::fixed << std::setprecision(12)
                              << last_sent_positions[i] << "," << std::fixed
                              << std::setprecision(12)
                              << last_sent_velocities[i] << "," << std::fixed
                              << std::setprecision(12) << last_sent_torques[i];
        }
        joint_commands_file << std::endl;
      }

    } catch (const std::exception &e) {
      std::cout << "⚠️ Logger thread error: " << e.what() << std::endl;
    }
  }

  std::cout << "✅ Logger thread stopped" << std::endl;
}

// Implementation of performance monitor thread function
void IC_CAN::Impl::performance_monitor_thread_function() {
  std::cout << "✅ Performance monitor thread started" << std::endl;

  while (performance_monitoring_) {
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Update every second

    std::lock_guard<std::mutex> lock(performance_mutex_);

    // Calculate instantaneous frequencies
    auto now = std::chrono::high_resolution_clock::now();
    double elapsed_sec =
        std::chrono::duration<double>(now - performance_start_time_).count();

    if (elapsed_sec > 1.0) {
      double send_freq = send_count_.load() / elapsed_sec;
      double recv_freq = receive_count_.load() / elapsed_sec;

      if (debug_enabled_) {
        std::cout << "📊 Performance: Send=" << std::fixed
                  << std::setprecision(1) << send_freq
                  << "Hz, Receive=" << recv_freq << "Hz" << std::endl;
      }

      // Reset counters every 10 seconds to get current rates
      if (elapsed_sec > 10.0) {
        send_count_ = 0;
        receive_count_ = 0;
        total_bytes_sent_ = 0;
        total_bytes_received_ = 0;
        performance_start_time_ = now;
      }
    }
  }

  std::cout << "✅ Performance monitor thread stopped" << std::endl;
}

// IC_CAN class implementation
IC_CAN::IC_CAN(const std::string &device_sn, bool debug)
    : impl_(std::make_unique<Impl>(device_sn, debug)) {}

IC_CAN::~IC_CAN() = default;

bool IC_CAN::initialize() { return impl_->initialize(); }
void IC_CAN::shutdown() { impl_->shutdown(); }
bool IC_CAN::enable_all() { return impl_->enable_all(); }
bool IC_CAN::disable_all() { return impl_->disable_all(); }
bool IC_CAN::set_zero_all() { return impl_->set_zero_all(); }
bool IC_CAN::set_motor_zero_calibration(int motor_id) {
  return impl_->set_motor_zero_calibration(motor_id);
}
bool IC_CAN::refresh_all() { return impl_->refresh_all(); }

std::vector<double> IC_CAN::get_joint_positions() {
  return impl_->get_joint_positions();
}
std::vector<double> IC_CAN::get_joint_velocities() {
  return impl_->get_joint_velocities();
}
std::vector<double> IC_CAN::get_joint_torques() {
  return impl_->get_joint_torques();
}

bool IC_CAN::set_joint_positions(const std::vector<double> &positions,
                                 const std::vector<double> &velocities,
                                 const std::vector<double> &torques) {
  return impl_->set_joint_positions(positions, velocities, torques);
}

bool IC_CAN::set_joint_velocities(const std::vector<double> &velocities,
                                  const std::vector<double> &torques) {
  std::vector<double> dummy_positions(9, 0.0);
  return impl_->set_joint_positions(dummy_positions, velocities, torques);
}

bool IC_CAN::set_joint_torques(const std::vector<double> &torques) {
  std::vector<double> dummy_positions(9, 0.0);
  std::vector<double> dummy_velocities(9, 0.0);
  return impl_->set_joint_positions(dummy_positions, dummy_velocities, torques);
}

bool IC_CAN::start_control_loop(double frequency) {
  return impl_->start_control_loop(frequency);
}

bool IC_CAN::start_high_frequency_control() {
  return impl_->start_control_loop(500.0); // Default 500Hz
}

void IC_CAN::stop_control_loop() { impl_->stop_control_loop(); }

void IC_CAN::stop_high_frequency_control() { impl_->stop_control_loop(); }

bool IC_CAN::is_control_running() const { return impl_->is_control_running(); }

bool IC_CAN::is_hf_control_running() const {
  return impl_->is_control_running();
}

void IC_CAN::set_target_positions_interpolated(
    const std::vector<double> &target_positions, double max_velocity) {
  impl_->set_target_positions_interpolated(target_positions, max_velocity);
}

std::vector<double>
IC_CAN::interpolate_positions(const std::vector<double> &current_positions,
                              const std::vector<double> &target_positions,
                              double dt, double max_velocity) {
  return Impl::interpolate_positions_static(current_positions, target_positions,
                                            dt, max_velocity);
}

std::map<std::string, std::string> IC_CAN::get_system_status() {
  return impl_->get_system_status();
}
void IC_CAN::print_system_info() { impl_->print_system_info(); }

// Component implementations
ArmComponent &IC_CAN::get_arm() { return *impl_->arm_component_; }

WristComponent &IC_CAN::get_wrist() { return *impl_->wrist_component_; }

GripperComponent &IC_CAN::get_gripper() { return *impl_->gripper_component_; }

SafetyModule &IC_CAN::get_safety() {
  static SafetyModule dummy_safety;
  return dummy_safety;
}

// Motor gain management API
bool IC_CAN::set_motor_gains(int motor_id, double kp, double kd) {
  return impl_->set_motor_gains(motor_id, kp, kd);
}

bool IC_CAN::set_all_motor_gains(const std::vector<double> &kp_values,
                                 const std::vector<double> &kd_values) {
  return impl_->set_all_motor_gains(kp_values, kd_values);
}

bool IC_CAN::get_motor_gains(int motor_id, double &kp, double &kd) {
  return impl_->get_motor_gains(motor_id, kp, kd);
}

void IC_CAN::load_default_motor_gains() { impl_->load_default_motor_gains(); }

// Frequency monitoring and logging API implementation
bool IC_CAN::start_logging(const std::string &log_directory) {
  return impl_->start_logging(log_directory);
}

bool IC_CAN::start_trajectory_logging(const std::string &log_directory) {
  return impl_->start_trajectory_logging(log_directory);
}

void IC_CAN::stop_logging() { impl_->stop_logging(); }

void IC_CAN::enable_frequency_monitoring() {
  impl_->enable_frequency_monitoring();
}

void IC_CAN::disable_frequency_monitoring() {
  impl_->disable_frequency_monitoring();
}

std::map<std::string, double> IC_CAN::get_performance_stats() {
  return impl_->get_performance_stats();
}

void IC_CAN::print_performance_stats() { impl_->print_performance_stats(); }

// Gravity compensation API implementation
bool IC_CAN::enable_gravity_compensation() {
  return impl_->enable_gravity_compensation();
}

bool IC_CAN::disable_gravity_compensation() {
  return impl_->disable_gravity_compensation();
}

bool IC_CAN::is_gravity_compensation_enabled() const {
  return impl_->is_gravity_compensation_enabled();
}

std::vector<double> IC_CAN::get_gravity_compensation_torques() {
  return impl_->get_gravity_compensation_torques();
}

std::vector<double> IC_CAN::get_all_predicted_torques() {
  return impl_->get_all_predicted_torques();
}

void IC_CAN::print_torque_breakdown() { impl_->print_torque_breakdown(); }

// Torque prediction method switching API
bool IC_CAN::switch_torque_prediction_method(int method_id) {
  return impl_->switch_torque_prediction_method(method_id);
}

void IC_CAN::print_torque_method_status() {
  impl_->print_torque_method_status();
}

// Friction compensation API implementation
bool IC_CAN::enable_friction_compensation() {
  return impl_->enable_friction_compensation();
}

bool IC_CAN::disable_friction_compensation() {
  return impl_->disable_friction_compensation();
}

bool IC_CAN::is_friction_compensation_enabled() const {
  return impl_->is_friction_compensation_enabled();
}

bool IC_CAN::load_friction_params_from_file(const std::string &filename) {
  return impl_->load_friction_params_from_file(filename);
}

void IC_CAN::print_friction_compensation_status() {
  impl_->print_friction_compensation_status();
}

void IC_CAN::send_ht_mit_command(double position, double velocity,
                                 double torque, double kp, double kd) {
  impl_->send_ht_mit_command(position, velocity, torque, kp, kd);
}

void IC_CAN::send_ht_mit_command_with_movement(
    double m7_position, double m7_velocity, double m7_torque,
    double m8_position, double m8_velocity, double m8_torque, double kp,
    double kd) {
  impl_->send_ht_mit_command_with_movement(m7_position, m7_velocity, m7_torque,
                                           m8_position, m8_velocity, m8_torque,
                                           kp, kd);
}

void IC_CAN::send_ht_set_zero_command() { impl_->send_ht_set_zero_command(); }

void IC_CAN::send_ht_brake_command() { impl_->send_ht_brake_command(); }

void IC_CAN::send_ht_read_state() { impl_->send_ht_read_state(); }

// Safety and emergency methods
bool IC_CAN::emergency_stop() { return impl_->emergency_stop(); }

bool IC_CAN::check_system_safety() { return impl_->check_system_safety(); }

// Wrist position monitoring methods
bool IC_CAN::start_wrist_position_monitoring(double frequency) {
  return impl_->start_wrist_position_monitoring(frequency);
}

void IC_CAN::stop_wrist_position_monitoring() {
  impl_->stop_wrist_position_monitoring();
}

bool IC_CAN::refresh_wrist_motors_only() {
  return impl_->refresh_wrist_motors_only();
}

std::vector<double> IC_CAN::get_wrist_positions() {
  return impl_->get_wrist_positions();
}

std::vector<double> IC_CAN::get_wrist_velocities() {
  return impl_->get_wrist_velocities();
}

std::vector<double> IC_CAN::get_wrist_torques() {
  return impl_->get_wrist_torques();
}

std::map<std::string, std::vector<double>> IC_CAN::get_wrist_monitoring_data() {
  return impl_->get_wrist_monitoring_data();
}

void IC_CAN::print_wrist_status() { impl_->print_wrist_status(); }

bool IC_CAN::is_wrist_monitoring_running() const {
  return impl_->is_wrist_monitoring_running();
}

} // namespace ic_can
