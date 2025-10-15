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
#include "ic_can/core/torque_predictor.h"
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <mutex>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

// Use working dm-tools SDK directly
#include "protocol/usb_class.h"

namespace ic_can {

// Forward declarations for component classes
class ArmComponent {
public:
  void print_info() { std::cout << "Arm Component (placeholder)" << std::endl; }
};

class GripperComponent {
public:
  void print_info() {
    std::cout << "Gripper Component (placeholder)" << std::endl;
  }
};

class SafetyModule {
public:
  void print_info() { std::cout << "Safety Module (placeholder)" << std::endl; }
};

class IC_CAN::Impl {
public:
  Impl(const std::string &device_sn, bool debug)
      : device_sn_(device_sn), debug_enabled_(debug), connected_(false),
        hf_control_running_(false), control_running_(false),
        logging_running_(false), performance_monitoring_(false),
        gravity_compensation_enabled_(false) {
    // Initialize motor gains with default values
    load_default_motor_gains();

    // Initialize performance counters
    send_count_ = 0;
    receive_count_ = 0;
    total_bytes_sent_ = 0;
    total_bytes_received_ = 0;
    performance_start_time_ = std::chrono::high_resolution_clock::now();

    // Initialize torque predictor
    torque_predictor_ = std::make_unique<TorquePredictor>();
    if (torque_predictor_->is_initialized()) {
      std::cout << "✅ Torque predictor initialized successfully" << std::endl;
    } else {
      std::cout << "⚠️  Torque predictor initialization failed - gravity "
                   "compensation unavailable"
                << std::endl;
    }
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
      if (capture_result == 0) {
        std::cout << "✅ SUCCESS: Data capture started" << std::endl;
      } else {
        std::cout << "⚠️  WARNING: Data capture start returned "
                  << (int)capture_result << std::endl;
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
      stop_logging();
      disable_frequency_monitoring();
      device_->USB_CMD_STOP_CAP();
      connected_ = false;
    }
  }
  void load_default_motor_gains() {
    std::lock_guard<std::mutex> lock(motor_gains_mutex_);

    motor_kp_gains_ = {250, 120, 120, 80, 150, 30, 8, 8, 0};
    motor_kd_gains_ = {5, 2, 2, 1.8, 2.2, 1, 1.2, 1.2, 0};

    /*motor_kp_gains_ = {0, 0, 0, 0, 0, 0, 0, 0, 0};*/
    /*motor_kd_gains_ = {0, 0, 0, 0, 0, 0, 0.0, 0.0, 0};*/

    if (debug_enabled_) {
      std::cout << "✅ Loaded default motor gains" << std::endl;
      for (auto i = 0; i <= 8; i++)
        std::cout << "Motor " << i << " kp: " << motor_kp_gains_[i]
                  << " kd: " << motor_kd_gains_[i] << std::endl;
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

    // Start logger thread
    logger_thread_ = std::thread([this]() { logger_thread_function(); });

    // Start performance monitoring if not already running
    if (!performance_monitoring_) {
      enable_frequency_monitoring();
    }

    std::cout << "✅ Started logging to timestamped directory: "
              << timestamped_dir << std::endl;
    return true;
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
      device_->fdcanFrameSend(enable_cmd, motor_id);
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
      device_->fdcanFrameSend(disable_cmd, motor_id);
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

    // Send the raw CAN command
    device_->fdcanFrameSend(zero_cmd, can_id);

    std::cout << "✅ Zero calibration command sent to Motor " << motor_id
              << std::endl;
    std::cout << "📝 The current position is now set as the new zero (0.0 rad)"
              << std::endl;

    // Give motor time to process the command
    usleep(5000); // 5ms

    return true;
  }

  bool refresh_all() {
    // Send status request to trigger position feedback

    for (int motor_id = 1; motor_id <= 9; motor_id++) {
      std::vector<uint8_t> status_cmd = {uint8_t(motor_id), 0x00, 0xCC, 0x00};
      device_->fdcanFrameSend(status_cmd, 0x7FF); // Broadcast status request
    }
    return true;
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

    for (int i = 0; i < 9; i++) {
      double pos = positions[i];
      double vel = (velocities.size() > i) ? velocities[i] : 0.0;
      double tau = (torques.size() > i) ? torques[i] : 0.0;
      double kp = kp_values[i];
      double kd = kd_values[i];

      // Add gravity compensation to torque feedforward
      if (gravity_compensation_enabled_ && i < 6) {
        /*tau += gravity_torques[i];*/
        std::cout << "motor id is " << i << ", torque is " << gravity_torques[i]
                  << std::endl;
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

    return true;
  }

  bool start_high_frequency_control() {
    if (hf_control_running_)
      return true;

    hf_control_running_ = true;
    hf_control_thread_ = std::thread([this]() {
      while (hf_control_running_) {
        auto start_time = std::chrono::steady_clock::now();

        // Send commands and receive data at 500Hz (2ms period)
        refresh_all();

        // Calculate time to sleep to maintain 500Hz
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        auto sleep_time = std::chrono::milliseconds(2) - elapsed;

        if (sleep_time.count() > 0) {
          std::this_thread::sleep_for(sleep_time);
        }
      }
    });

    return true;
  }

  void stop_high_frequency_control() {
    hf_control_running_ = false;
    if (hf_control_thread_.joinable()) {
      hf_control_thread_.join();
    }
  }

  bool is_hf_control_running() const { return hf_control_running_; }

  // Configurable control loop implementation with offline trajectory
  // generation
  bool start_control_loop(double frequency) {
    if (control_running_)
      return true;

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

      // Pre-compute entire trajectory offline
      std::cout << "📊 Pre-computing trajectory..." << std::endl;
      std::cout << "📊 [DEBUG] Current target_positions_ size: "
                << target_positions_.size() << std::endl;
      trajectory_points_ =
          generate_trajectory_offline(current_positions_, target_positions_,
                                      1.0 / control_frequency_, max_velocity_);
      current_trajectory_index_ = 0;

      std::cout << "✅ Generated " << trajectory_points_.size()
                << " trajectory points" << std::endl;
    }

    control_thread_ = std::thread([this]() {
      auto period = std::chrono::duration<double>(1.0 / control_frequency_);
      std::cout << "[Notice] sleep time is " << period.count() << std::endl;
      while (control_running_) {
        auto start_time = std::chrono::steady_clock::now();

        // Get next pre-computed position from trajectory
        std::vector<double> next_position;
        {
          std::lock_guard<std::mutex> lock(interpolation_mutex_);

          if (current_trajectory_index_ < trajectory_points_.size()) {
            next_position = trajectory_points_[current_trajectory_index_];
            current_trajectory_index_++;

            /*std::cout << "🎯 Step " << current_trajectory_index_ << "/"*/
            /*          << trajectory_points_.size() << ": ";*/
            /*for (int i = 0; i < 9; i++) {*/
            /*  std::cout << std::fixed << std::setprecision(3)*/
            /*            << next_position[i] << " ";*/
            /*}*/
            /*std::cout << std::endl;*/
          } else {
            // Trajectory completed, hold final position
            next_position = trajectory_points_.back();
            /*std::cout << "🏁 Trajectory completed, holding position"*/
            /*          << std::endl;*/
          }
        }

        // Send position to motors
        set_joint_positions(next_position, {}, {});

        // Request status updates
        /*refresh_all();*/

        // Calculate sleep time to maintain frequency
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        auto sleep_time = period - elapsed;

        if (sleep_time.count() > 0) {
          std::this_thread::sleep_for(sleep_time);
        }
      }
    });

    std::cout << "✅ Started control loop at " << frequency << " Hz"
              << std::endl;
    return true;
  }

  void stop_control_loop() {
    control_running_ = false;
    if (control_thread_.joinable()) {
      control_thread_.join();
    }
    std::cout << "✅ Control loop stopped" << std::endl;
  }

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

    if (torque_predictor_->predict_gravity_torque(q, gravity_arm)) {
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

    torque_predictor_->print_torque_breakdown(q, dq, ddq);
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
    /*std::cout << "Motors: 9 total (6 Damiao + 2 HT + 1 Servo)" <<
     * std::endl;*/
    std::cout << "Control Frequency: 500Hz" << std::endl;
    std::cout << "Debug: " << (debug_enabled_ ? "Enabled" : "Disabled")
              << std::endl;
  }

private:
  // Torque predictor instance
  std::unique_ptr<TorquePredictor> torque_predictor_;
  bool gravity_compensation_enabled_;

  void handle_can_frame(can_value_type &frame) {
    uint32_t can_id = frame.head.id;

    // Track receive frequency
    if (can_id == 0x11)
      receive_count_++;
    total_bytes_received_ += frame.head.dlc;

    /*std::cout << "receive from " << can_id << std::endl;*/

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

    // Extract HT motor data (same as HT test)
    int16_t pos_int =
        static_cast<int16_t>(frame.data[1] | (frame.data[2] << 8));
    int16_t vel_int =
        static_cast<int16_t>(frame.data[3] | (frame.data[4] << 8));
    int16_t torque_int =
        static_cast<int16_t>(frame.data[5] | (frame.data[6] << 8));

    // Convert to physical units (HT protocol)
    const double TURN_TO_RAD = 2.0 * M_PI;
    double position = pos_int * 0.0001 * TURN_TO_RAD;
    double velocity = vel_int * 0.00025 * TURN_TO_RAD;
    const double torque_k = 0.004855;
    const double torque_d = -0.083;
    double torque = torque_int * torque_k + torque_d;

    // Update atomic values
    positions_[motor_idx].store(position);
    velocities_[motor_idx].store(velocity);
    torques_[motor_idx].store(torque);
  }

  void send_dm_mit_command(int motor_id, double position, double velocity,
                           double torque, double kp, double kd) {
    // Track send frequency
    /*std::cout << motor_id << std::endl;*/
    total_bytes_sent_ += 8; // DM command is 8 bytes
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
    /*print_send_info(motor_id, data);*/
    device_->fdcanFrameSend(data, motor_id);
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
  void send_ht_mit_command(double position, double velocity, double torque,
                           double kp, double kd) {
    // Track send frequency
    std::cout << "-----------------------sennding ht " << std::endl;
    total_bytes_sent_ += 12; // HT command is 12 bytes
    // HT motor conversion constants
    const double RAD_TO_TURN = 1.0 / (2.0 * M_PI);
    const double torque_k = 0.004855;
    const double torque_d = -0.083;

    // Convert to motor units
    double pos_turns = position * RAD_TO_TURN;
    double vel_turns = velocity * RAD_TO_TURN;

    // Convert to int16 format
    int16_t pos_int = static_cast<int16_t>(pos_turns / 0.0001);
    int16_t vel_int = static_cast<int16_t>(vel_turns / 0.00025);
    int16_t kp_int = static_cast<int16_t>(kp * 10);
    int16_t kd_int = static_cast<int16_t>(kd * 10);
    int16_t torque_int = static_cast<int16_t>((torque - torque_d) / torque_k);

    // Pack HT MIT command (12 bytes)
    std::vector<uint8_t> data(12, 0);
    data[0] = pos_int & 0xFF;
    data[1] = (pos_int >> 8) & 0xFF;
    data[2] = vel_int & 0xFF;
    data[3] = (vel_int >> 8) & 0xFF;
    data[4] = torque_int & 0xFF;
    data[5] = (torque_int >> 8) & 0xFF;
    data[6] = kp_int & 0xFF;
    data[7] = (kp_int >> 8) & 0xFF;
    data[8] = kd_int & 0xFF;
    data[9] = (kd_int >> 8) & 0xFF;
    data[10] = 0x00;
    data[11] = 0x00;

    device_->fdcanFrameSend(data, 0x8094); // HT uses fixed ID 0x8094
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

  // Performance counters
  std::atomic<uint64_t> send_count_;
  std::atomic<uint64_t> receive_count_;
  std::atomic<uint64_t> total_bytes_sent_;
  std::atomic<uint64_t> total_bytes_received_;
  std::chrono::high_resolution_clock::time_point performance_start_time_;
  std::mutex performance_mutex_;

  // Logger thread function
  void logger_thread_function();

  // Performance monitor thread function
  void performance_monitor_thread_function();

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

  // Store last target values for logging
  std::vector<double> last_target_positions(9, 0.0);
  std::vector<double> last_target_velocities(9, 0.0);
  std::vector<double> last_target_torques(9, 0.0);
  bool targets_initialized = false;

  while (logging_running_) {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(1000 / 400)); // Log  logging frequency

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

      // Log target commands if we have interpolation state
      {
        std::lock_guard<std::mutex> lock(interpolation_mutex_);
        if (!target_positions_.empty()) {
          last_target_positions = target_positions_;
          targets_initialized = true;
        }
      }

      if (targets_initialized) {
        joint_commands_file << timestamp;
        for (int i = 0; i < 9; i++) {
          joint_commands_file
              << "," << std::fixed << std::setprecision(12)
              << last_target_positions[i] << "," << std::fixed
              << std::setprecision(12) << last_target_velocities[i] << ","
              << std::fixed << std::setprecision(12) << last_target_torques[i];
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

// Placeholder implementations for components
ArmComponent &IC_CAN::get_arm() {
  static ArmComponent dummy_arm;
  return dummy_arm;
}

GripperComponent &IC_CAN::get_gripper() {
  static GripperComponent dummy_gripper;
  return dummy_gripper;
}

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

} // namespace ic_can
