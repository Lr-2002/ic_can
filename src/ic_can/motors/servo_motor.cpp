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

#include "ic_can/motors/servo_motor.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>

namespace ic_can {

ServoMotor::ServoMotor(int motor_id, uint32_t can_send_id, uint32_t can_recv_id,
                       double min_pwm_us, double max_pwm_us)
    : BaseMotor(motor_id, MotorType::SERVO, can_send_id, can_recv_id),
      min_pwm_us_(min_pwm_us),
      max_pwm_us_(max_pwm_us),
      control_mode_(ServoControlMode::POSITION_PWM),
      current_pwm_us_((min_pwm_us + max_pwm_us) / 2.0),
      target_pwm_us_((min_pwm_us + max_pwm_us) / 2.0),
      current_velocity_(0.0),
      pwm_ramp_rate_((max_pwm_us - min_pwm_us) / 100.0), // Default 100ms full range
      velocity_ramping_enabled_(false),
      ramp_time_ms_(100.0),
      has_pending_command_(false) {

  // Initialize servo-specific limits
  limits_.min_position = 0.0;
  limits_.max_position = 1.0;  // Normalized position (0.0 to 1.0)
  limits_.max_velocity = 1.0;  // Full range per second
  limits_.max_torque = 1.0;    // Normalized torque

  // Initialize command buffer
  command_buffer_.resize(8, 0);

  // Set initial timestamp
  last_update_time_ = std::chrono::steady_clock::now();

  std::cout << "✅ ServoMotor " << motor_id << " initialized (CAN ID: 0x"
            << std::hex << can_send_id << std::dec << ")" << std::endl;
  std::cout << "   PWM range: " << min_pwm_us_ << "-" << max_pwm_us_ << " μs" << std::endl;
}

bool ServoMotor::enable() {
  command_buffer_ = create_enable_command();
  has_pending_command_ = true;
  set_enabled_state(true);

  std::cout << "🔧 Servo " << motor_id_ << " enabled" << std::endl;
  return true;
}

bool ServoMotor::disable() {
  command_buffer_ = create_disable_command();
  has_pending_command_ = true;
  set_enabled_state(false);

  std::cout << "🔧 Servo " << motor_id_ << " disabled" << std::endl;
  return true;
}

bool ServoMotor::set_zero() {
  command_buffer_ = create_set_zero_command();
  has_pending_command_ = true;

  // Reset position to center
  current_pwm_us_ = (min_pwm_us_ + max_pwm_us_) / 2.0;
  target_pwm_us_ = current_pwm_us_;
  update_internal_state(0.5, 0.0, 0.0, temperature_.load());

  std::cout << "🎯 Servo " << motor_id_ << " zero position set" << std::endl;
  return true;
}

bool ServoMotor::is_enabled() const {
  return enabled_.load();
}

bool ServoMotor::set_command(const MotorCommand& command) {
  if (!command.use_position && !command.use_velocity && !command.use_torque) {
    return false;
  }

  double target_position = 0.5; // Default center position
  double target_velocity = 0.0;
  double target_torque = 0.0;

  if (command.use_position) {
    target_position = std::clamp(command.position, 0.0, 1.0);
  }
  if (command.use_velocity) {
    target_velocity = std::clamp(command.velocity, -1.0, 1.0);
  }
  if (command.use_torque) {
    target_torque = std::clamp(command.torque, 0.0, 1.0);
  }

  if (command.use_position) {
    return set_position(target_position, target_velocity, target_torque);
  } else if (command.use_velocity) {
    return set_velocity(target_velocity, target_torque);
  } else if (command.use_torque) {
    return set_torque(target_torque);
  }

  return false;
}

bool ServoMotor::set_position(double position, double velocity, double torque) {
  // Clamp position to valid range
  position = std::clamp(position, 0.0, 1.0);

  // Convert position to PWM
  double target_pwm = position_to_pwm(position, min_pwm_us_, max_pwm_us_);

  if (velocity_ramping_enabled_ && std::abs(velocity) > 0.01) {
    // Enable velocity ramping
    enable_velocity_ramping(true, ramp_time_ms_);
    current_velocity_ = velocity;
  } else {
    // Direct control
    target_pwm_us_ = target_pwm;
  }

  // Create position command
  command_buffer_ = create_set_position_command(position);
  has_pending_command_ = true;

  // Store command
  last_command_.use_position = true;
  last_command_.position = position;
  last_command_.velocity = velocity;
  last_command_.torque = torque;

  return true;
}

bool ServoMotor::set_velocity(double velocity, double torque) {
  velocity = std::clamp(velocity, -1.0, 1.0);

  // Enable velocity ramping mode
  enable_velocity_ramping(true, ramp_time_ms_);
  current_velocity_ = velocity;

  // Calculate target PWM based on velocity
  if (velocity > 0) {
    target_pwm_us_ = max_pwm_us_;
  } else if (velocity < 0) {
    target_pwm_us_ = min_pwm_us_;
  } else {
    target_pwm_us_ = (min_pwm_us_ + max_pwm_us_) / 2.0;
  }

  return true;
}

bool ServoMotor::set_torque(double torque) {
  torque = std::clamp(torque, 0.0, 1.0);

  // For servos, torque affects PWM modulation
  // Higher torque = stronger positioning
  double base_pwm = (min_pwm_us_ + max_pwm_us_) / 2.0;
  double torque_offset = (max_pwm_us_ - min_pwm_us_) * torque * 0.5;

  target_pwm_us_ = base_pwm + torque_offset;

  return true;
}

bool ServoMotor::mit_control(double position, double velocity, double torque,
                            double kp, double kd) {
  // For servos, MIT control is simplified
  // kp affects PWM scaling, kd affects ramping speed
  set_position(position, velocity, torque);

  if (kp > 0) {
    // Adjust PWM range based on kp
    double range_factor = std::clamp(kp / 50.0, 0.1, 2.0);
    double adjusted_range = (max_pwm_us_ - min_pwm_us_) * range_factor;
    double center = (min_pwm_us_ + max_pwm_us_) / 2.0;

    double pos = std::clamp(position, 0.0, 1.0);
    target_pwm_us_ = center + (pos - 0.5) * adjusted_range;
    target_pwm_us_ = clamp_pwm(target_pwm_us_);
  }

  if (kd > 0 && velocity_ramping_enabled_) {
    // Adjust ramping speed based on kd
    double ramp_factor = std::clamp(kd / 2.0, 0.1, 10.0);
    pwm_ramp_rate_ = (max_pwm_us_ - min_pwm_us_) / (ramp_time_ms_ * ramp_factor);
  }

  return true;
}

bool ServoMotor::set_pwm_pulse_width(double pwm_us) {
  target_pwm_us_ = clamp_pwm(pwm_us);
  command_buffer_ = create_set_position_command(pwm_to_position(target_pwm_us_, min_pwm_us_, max_pwm_us_));
  has_pending_command_ = true;
  return true;
}

bool ServoMotor::set_angle_degrees(double angle_degrees) {
  double position = degrees_to_position(angle_degrees);
  return set_position(position);
}

double ServoMotor::get_pwm_pulse_width() const {
  return current_pwm_us_;
}

double ServoMotor::get_angle_degrees() const {
  double position = get_position();
  return position_to_degrees(position);
}

void ServoMotor::set_pwm_limits(double min_pwm_us, double max_pwm_us) {
  min_pwm_us_ = min_pwm_us;
  max_pwm_us_ = max_pwm_us;

  // Ensure current PWM is within new limits
  current_pwm_us_ = clamp_pwm(current_pwm_us_);
  target_pwm_us_ = clamp_pwm(target_pwm_us_);
}

void ServoMotor::enable_velocity_ramping(bool enable, double ramp_time_ms) {
  velocity_ramping_enabled_ = enable;
  ramp_time_ms_ = ramp_time_ms;

  if (enable) {
    pwm_ramp_rate_ = (max_pwm_us_ - min_pwm_us_) / ramp_time_ms;
  }
}

double ServoMotor::get_position() const {
  return pwm_to_position(current_pwm_us_, min_pwm_us_, max_pwm_us_);
}

double ServoMotor::get_velocity() const {
  return current_velocity_;
}

double ServoMotor::get_torque() const {
  // For servos, torque is simulated based on position error
  double center_pos = 0.5;
  double current_pos = get_position();
  double position_error = std::abs(current_pos - center_pos);
  return position_error * 2.0; // Normalize to 0.0-1.0 range
}

double ServoMotor::get_temperature() const {
  return temperature_.load();
}

MotorState ServoMotor::get_state() const {
  MotorState state;
  state.position = get_position();
  state.velocity = get_velocity();
  state.torque = get_torque();
  state.temperature = get_temperature();
  state.enabled = is_enabled();
  state.error = has_error();
  state.timestamp = std::chrono::steady_clock::now();
  return state;
}

void ServoMotor::update_state() {
  // Update PWM with ramping if enabled
  if (velocity_ramping_enabled_) {
    auto now = std::chrono::steady_clock::now();
    double dt_ms = std::chrono::duration<double, std::milli>(now - last_update_time_).count();
    update_pwm_with_ramping(dt_ms);
    last_update_time_ = now;
  } else {
    current_pwm_us_ = target_pwm_us_;
  }

  // Update internal state
  double position = get_position();
  update_internal_state(position, current_velocity_, get_torque(), temperature_.load());
}

bool ServoMotor::process_response(const std::vector<uint8_t>& data) {
  if (data.empty()) {
    return false;
  }

  // Parse servo response (simplified - assumes position feedback)
  if (data.size() >= 4) {
    // Extract position data (assuming 16-bit position value)
    uint16_t pos_raw = (static_cast<uint16_t>(data[0]) << 8) | data[1];
    double position = static_cast<double>(pos_raw) / 65535.0; // Normalize to 0.0-1.0

    // Update current position
    double pwm = position_to_pwm(position, min_pwm_us_, max_pwm_us_);
    current_pwm_us_ = pwm;

    // Extract status byte if available
    if (data.size() >= 3) {
      uint8_t status = data[2];
      set_error_state((status & 0x01) != 0); // Error flag in bit 0
      set_enabled_state((status & 0x02) != 0); // Enable flag in bit 1
    }

    return true;
  }

  return false;
}

std::vector<uint8_t> ServoMotor::get_command_data() const {
  if (has_pending_command_) {
    return command_buffer_;
  }
  return {};
}

bool ServoMotor::set_control_mode(ServoControlMode mode) {
  control_mode_ = mode;
  return true;
}

void ServoMotor::reset_error() {
  set_error_state(false);
  has_pending_command_ = false;
}

// Static utility methods
double ServoMotor::position_to_pwm(double position, double min_pwm_us, double max_pwm_us) {
  position = std::clamp(position, 0.0, 1.0);
  return min_pwm_us + position * (max_pwm_us - min_pwm_us);
}

double ServoMotor::pwm_to_position(double pwm_us, double min_pwm_us, double max_pwm_us) {
  pwm_us = std::clamp(pwm_us, min_pwm_us, max_pwm_us);
  return (pwm_us - min_pwm_us) / (max_pwm_us - min_pwm_us);
}

double ServoMotor::degrees_to_position(double angle_degrees) {
  // Assuming 0-180 degree range for standard servos
  angle_degrees = std::clamp(angle_degrees, 0.0, 180.0);
  return angle_degrees / 180.0;
}

double ServoMotor::position_to_degrees(double position) {
  position = std::clamp(position, 0.0, 1.0);
  return position * 180.0;
}

// Private methods
std::vector<uint8_t> ServoMotor::create_enable_command() {
  std::vector<uint8_t> cmd = {0x09, 0x00, static_cast<uint8_t>(ServoCommand::ENABLE), 0x00, 0x00, 0x00, 0x00, 0x00};
  return cmd;
}

std::vector<uint8_t> ServoMotor::create_disable_command() {
  std::vector<uint8_t> cmd = {0x09, 0x00, static_cast<uint8_t>(ServoCommand::DISABLE), 0x00, 0x00, 0x00, 0x00, 0x00};
  return cmd;
}

std::vector<uint8_t> ServoMotor::create_set_zero_command() {
  std::vector<uint8_t> cmd = {0x09, 0x00, static_cast<uint8_t>(ServoCommand::SET_ZERO), 0x00, 0x00, 0x00, 0x00, 0x00};
  return cmd;
}

std::vector<uint8_t> ServoMotor::create_set_position_command(double position) {
  position = std::clamp(position, 0.0, 1.0);
  uint16_t pos_raw = static_cast<uint16_t>(position * 65535.0);

  std::vector<uint8_t> cmd = {
    0x09, // Motor ID
    0x00, // Reserved
    static_cast<uint8_t>(ServoCommand::SET_POSITION),
    static_cast<uint8_t>((pos_raw >> 8) & 0xFF), // Position high byte
    static_cast<uint8_t>(pos_raw & 0xFF),        // Position low byte
    0x00, // Reserved
    0x00, // Reserved
    0x00  // Reserved
  };

  return cmd;
}

std::vector<uint8_t> ServoMotor::create_refresh_command() {
  std::vector<uint8_t> cmd = {0x09, 0x00, static_cast<uint8_t>(ServoCommand::REFRESH), 0x00, 0x00, 0x00, 0x00, 0x00};
  return cmd;
}

void ServoMotor::update_pwm_with_ramping(double dt_ms) {
  if (!velocity_ramping_enabled_) {
    current_pwm_us_ = target_pwm_us_;
    return;
  }

  double error = target_pwm_us_ - current_pwm_us_;
  double max_change = pwm_ramp_rate_ * dt_ms;

  if (std::abs(error) <= max_change) {
    current_pwm_us_ = target_pwm_us_;
  } else {
    current_pwm_us_ += (error > 0) ? max_change : -max_change;
  }
}

double ServoMotor::clamp_pwm(double pwm_us) {
  return std::clamp(pwm_us, min_pwm_us_, max_pwm_us_);
}

} // namespace ic_can