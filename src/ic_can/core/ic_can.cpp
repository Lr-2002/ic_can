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
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <sys/types.h>
#include <thread>

// Use working dm-tools SDK directly
#include "dm-tools/USB2FDCAN/SDK/C++/ubuntu/include/protocol/usb_class.h"

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
        hf_control_running_(false) {}

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
      device_->USB_CMD_STOP_CAP();
      connected_ = false;
    }
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
    if (!connected_)
      return false;
    if (positions.size() < 9)
      return false;

    for (int i = 0; i < 9; i++) {
      double pos = positions[i];
      double vel = (velocities.size() > i) ? velocities[i] : 0.0;
      double tau = (torques.size() > i) ? torques[i] : 0.0;

      if (i < 6) {
        // Damiao motors 1-6: use DM MIT protocol
        send_dm_mit_command(i + 1, pos, vel, tau, 0, 0);
      } else if (i < 8) {
        // HT motors 7-8: use HT MIT protocol
        send_ht_mit_command(pos, vel, tau, 0, 0);
      } else {
        // Servo motor 9: use DM protocol as placeholder
        send_dm_mit_command(i + 1, pos, vel, tau, 0, 0);
      }
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

  std::map<std::string, std::string> get_system_status() {
    return {{"connected", connected_ ? "true" : "false"},
            {"hf_control", hf_control_running_ ? "running" : "stopped"},
            {"device_sn", device_sn_},
            {"motors_count", "9"},
            {"damiao_motors", "6"},
            {"ht_motors", "2"},
            {"servo_motors", "1"}};
  }

  void print_system_info() {
    std::cout << "\n=== IC_CAN System Information ===" << std::endl;
    std::cout << "Device SN: " << device_sn_ << std::endl;
    std::cout << "Connected: " << (connected_ ? "Yes" : "No") << std::endl;
    std::cout << "Motors: 9 total (6 Damiao + 2 HT + 1 Servo)" << std::endl;
    std::cout << "Control Frequency: 500Hz" << std::endl;
    std::cout << "Debug: " << (debug_enabled_ ? "Enabled" : "Disabled")
              << std::endl;
  }

private:
  void handle_can_frame(can_value_type &frame) {
    uint32_t can_id = frame.head.id;
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

    device_->fdcanFrameSend(data, motor_id);
  }

  void send_ht_mit_command(double position, double velocity, double torque,
                           double kp, double kd) {
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

  // High-frequency control
  std::atomic<bool> hf_control_running_;
  std::thread hf_control_thread_;
};

// IC_CAN class implementation
IC_CAN::IC_CAN(const std::string &device_sn, bool debug)
    : impl_(std::make_unique<Impl>(device_sn, debug)) {}

IC_CAN::~IC_CAN() = default;

bool IC_CAN::initialize() { return impl_->initialize(); }
void IC_CAN::shutdown() { impl_->shutdown(); }
bool IC_CAN::enable_all() { return impl_->enable_all(); }
bool IC_CAN::disable_all() { return impl_->disable_all(); }
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

bool IC_CAN::start_high_frequency_control() {
  return impl_->start_high_frequency_control();
}
void IC_CAN::stop_high_frequency_control() {
  impl_->stop_high_frequency_control();
}
bool IC_CAN::is_hf_control_running() const {
  return impl_->is_hf_control_running();
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

} // namespace ic_can
