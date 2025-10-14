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

#include <ic_can/motors/ht_motor.hpp>
#include <iostream>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace ic_can {

HTMotor::HTMotor(int motor_id, uint32_t can_send_id, uint32_t can_recv_id,
                 double kp, double kd, double max_torque, bool debug)
    : BaseMotor(motor_id, MotorType::HT_HIGH_TORQUE, can_send_id, can_recv_id),
      motor_id_(motor_id), can_send_id_(can_send_id), can_recv_id_(can_recv_id),
      kp_(kp), kd_(kd), max_torque_(max_torque),
      position_(0.0), velocity_(0.0), torque_(0.0), temperature_(25.0),
      enabled_(false), debug_enabled_(debug) {

    // HT4438 motor constants based on Python implementation
    torque_k_ = 0.004855;  // Slope for int16 torque
    torque_d_ = -0.083;    // Offset for int16 torque

    // Conversion factors
    RAD_TO_TURN_ = 1.0 / (2.0 * M_PI);  // Convert radians to turns
    TURN_TO_RAD_ = 2.0 * M_PI;          // Convert turns to radians

    // Set HT motor limits based on Python code and project requirements
    position_limit_min_ = -12.5;  // Conservative limit for HT motor
    position_limit_max_ = 12.5;
    velocity_limit_max_ = 15.0;   // Conservative limit for HT motor
    torque_limit_max_ = 18.0;     // Conservative torque limit

    command_data_.resize(12, 0); // HT MIT command is 12 bytes
    debug_print("HT motor created: ID=" + std::to_string(motor_id) +
                ", SendID=0x" + std::to_string(can_send_id) +
                ", RecvID=0x" + std::to_string(can_recv_id) +
                " (HT4438 MIT protocol)");
}

// ========== BaseMotor Interface Implementation ==========

bool HTMotor::enable() {
    // HT motor enable command (using DM format for now since HT enable protocol not specified)
    command_data_ = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC};
    enabled_ = true;
    debug_print("HT motor enabled (using DM enable command as placeholder)");
    return true;
}

bool HTMotor::disable() {
    // HT motor disable command (using DM format for now)
    command_data_ = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD};
    enabled_ = false;
    debug_print("HT motor disabled (using DM disable command as placeholder)");
    return true;
}

bool HTMotor::set_zero() {
    debug_print("Setting zero position for HT motor");
    // HT zero position command (using DM format for now)
    command_data_ = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE};
    return true;
}

bool HTMotor::is_enabled() const {
    return enabled_;
}

bool HTMotor::set_position(double position, double velocity, double torque) {
    if (!enabled_) {
        debug_print("Cannot set position: HT motor not enabled");
        return false;
    }

    // Clamp values to limits
    position = std::clamp(position, position_limit_min_, position_limit_max_);
    velocity = std::clamp(velocity, -velocity_limit_max_, velocity_limit_max_);
    torque = std::clamp(torque, -torque_limit_max_, torque_limit_max_);

    command_data_ = pack_ht_mit_command(position, velocity, torque, kp_, kd_);
    debug_print("HT position set: p=" + std::to_string(position) +
                ", v=" + std::to_string(velocity) +
                ", t=" + std::to_string(torque));
    return true;
}

bool HTMotor::set_velocity(double velocity, double torque) {
    if (!enabled_) {
        debug_print("Cannot set velocity: HT motor not enabled");
        return false;
    }

    // Clamp values to limits
    velocity = std::clamp(velocity, -velocity_limit_max_, velocity_limit_max_);
    torque = std::clamp(torque, -torque_limit_max_, torque_limit_max_);

    command_data_ = pack_ht_mit_command(position_, velocity, torque, kp_, kd_);
    debug_print("HT velocity set: v=" + std::to_string(velocity) +
                ", t=" + std::to_string(torque));
    return true;
}

bool HTMotor::set_torque(double torque) {
    if (!enabled_) {
        debug_print("Cannot set torque: HT motor not enabled");
        return false;
    }

    // Clamp torque to limits
    torque = std::clamp(torque, -torque_limit_max_, torque_limit_max_);

    command_data_ = pack_ht_mit_command(position_, velocity_, torque, kp_, kd_);
    debug_print("HT torque set: t=" + std::to_string(torque));
    return true;
}

bool HTMotor::mit_control(double position, double velocity, double torque, double kp, double kd) {
    if (!enabled_) {
        debug_print("Cannot MIT control: HT motor not enabled");
        return false;
    }

    // Update control parameters
    kp_ = kp;
    kd_ = kd;

    // Clamp values to limits
    position = std::clamp(position, position_limit_min_, position_limit_max_);
    velocity = std::clamp(velocity, -velocity_limit_max_, velocity_limit_max_);
    torque = std::clamp(torque, -torque_limit_max_, torque_limit_max_);

    command_data_ = pack_ht_mit_command(position, velocity, torque, kp, kd);
    debug_print("HT MIT control: p=" + std::to_string(position) +
                ", v=" + std::to_string(velocity) +
                ", t=" + std::to_string(torque) +
                ", kp=" + std::to_string(kp) +
                ", kd=" + std::to_string(kd));
    return true;
}

bool HTMotor::set_command(const MotorCommand& command) {
    if (command.use_position) {
        return set_position(command.position, command.velocity, command.torque);
    } else if (command.use_velocity) {
        return set_velocity(command.velocity, command.torque);
    } else if (command.use_torque) {
        return set_torque(command.torque);
    }
    return false;
}

double HTMotor::get_position() const {
    return position_;
}

double HTMotor::get_velocity() const {
    return velocity_;
}

double HTMotor::get_torque() const {
    return torque_;
}

double HTMotor::get_temperature() const {
    return temperature_;
}

void HTMotor::update_state() {
    // State is updated via process_response() called from CAN frame handling
}

// Additional virtual method implementations
void HTMotor::set_limits(const MotorLimits& limits) {
    position_limit_min_ = limits.min_position;
    position_limit_max_ = limits.max_position;
    velocity_limit_max_ = limits.max_velocity;
    torque_limit_max_ = limits.max_torque;
}

void HTMotor::set_max_position(double max_position) {
    position_limit_max_ = max_position;
}

void HTMotor::set_min_position(double min_position) {
    position_limit_min_ = min_position;
}

void HTMotor::set_max_velocity(double max_velocity) {
    velocity_limit_max_ = max_velocity;
}

void HTMotor::set_max_torque(double max_torque) {
    torque_limit_max_ = max_torque;
}

bool HTMotor::is_position_safe(double position) const {
    return position >= position_limit_min_ && position <= position_limit_max_;
}

bool HTMotor::is_velocity_safe(double velocity) const {
    return std::abs(velocity) <= velocity_limit_max_;
}

bool HTMotor::is_torque_safe(double torque) const {
    return std::abs(torque) <= torque_limit_max_;
}

bool HTMotor::is_temperature_safe(double temperature) const {
    return temperature <= 80.0; // Standard temperature limit
}

MotorState HTMotor::get_state() const {
    MotorState state;
    state.position = position_;
    state.velocity = velocity_;
    state.torque = torque_;
    state.temperature = temperature_;
    state.enabled = enabled_;
    state.error = false; // TODO: Implement error detection
    state.timestamp = std::chrono::steady_clock::now();
    return state;
}

void HTMotor::reset_error() {
    // TODO: Implement HT motor error reset logic
    debug_print("HT motor error reset");
}

int HTMotor::get_motor_id() const {
    return motor_id_;
}

uint32_t HTMotor::get_can_send_id() const {
    return can_send_id_;
}

uint32_t HTMotor::get_can_recv_id() const {
    return can_recv_id_;
}

std::vector<uint8_t> HTMotor::get_command_data() const {
    return command_data_;
}

bool HTMotor::process_response(const std::vector<uint8_t>& data) {
    if (data.size() < 7) {
        debug_print("Invalid HT response data size: " + std::to_string(data.size()));
        return false;
    }

    unpack_ht_response(data);
    return true;
}

// ========== HT Motor Specific Methods ==========

void HTMotor::set_ht_params(double kp, double kd, double max_torque) {
    kp_ = kp;
    kd_ = kd;
    max_torque_ = max_torque;
    torque_limit_max_ = max_torque;
    debug_print("HT params updated: kp=" + std::to_string(kp) +
                ", kd=" + std::to_string(kd) +
                ", max_tau=" + std::to_string(max_torque));
}

void HTMotor::set_control_mode(HTControlMode mode) {
    control_mode_ = mode;
    debug_print("HT control mode set to: " + std::to_string(static_cast<int>(mode)));
}

void HTMotor::set_position_limits(double min_pos, double max_pos) {
    position_limit_min_ = min_pos;
    position_limit_max_ = max_pos;
    debug_print("HT position limits set: [" + std::to_string(min_pos) + ", " + std::to_string(max_pos) + "] rad");
}

void HTMotor::set_force_limit(double max_force) {
    // Convert force to torque for internal consistency
    torque_limit_max_ = max_force;
    debug_print("HT force limit set: " + std::to_string(max_force) + " N");
}

bool HTMotor::set_position_control(double position, double velocity) {
    return set_position(position, velocity, 0.0);
}

bool HTMotor::set_force_control(double force) {
    return set_torque(force);
}

bool HTMotor::set_velocity_control(double velocity) {
    return set_velocity(velocity, 0.0);
}

// ========== Private Methods ==========

std::vector<uint8_t> HTMotor::pack_ht_mit_command(double position, double velocity,
                                                   double torque, double kp, double kd) {
    std::vector<uint8_t> data(12, 0);

    // Convert to motor units based on Python implementation
    double pos_turns = position * RAD_TO_TURN_;
    double vel_turns = velocity * RAD_TO_TURN_;

    // Convert to int16 format (same as Python)
    int16_t pos_int = static_cast<int16_t>(pos_turns / 0.0001);  // LSB = 0.0001 turns
    int16_t vel_int = static_cast<int16_t>(vel_turns / 0.00025); // LSB = 0.00025 turns/s
    int16_t kp_int = static_cast<int16_t>(kp * 10);             // LSB = 0.1
    int16_t kd_int = static_cast<int16_t>(kd * 10);             // LSB = 0.1

    // Convert torque from Nm to motor units (reverse Python equation)
    int16_t torque_int = static_cast<int16_t>((torque - torque_d_) / torque_k_);

    // Clamp values to int16 range
    pos_int = std::clamp(pos_int, static_cast<int16_t>(-32768), static_cast<int16_t>(32767));
    vel_int = std::clamp(vel_int, static_cast<int16_t>(-32768), static_cast<int16_t>(32767));
    kp_int = std::clamp(kp_int, static_cast<int16_t>(-32768), static_cast<int16_t>(32767));
    kd_int = std::clamp(kd_int, static_cast<int16_t>(-32768), static_cast<int16_t>(32767));
    torque_int = std::clamp(torque_int, static_cast<int16_t>(-32768), static_cast<int16_t>(32767));

    // Pack as little-endian int16 values (same as Python struct.pack("<h", value))
    data[0] = pos_int & 0xFF;         // Position LSB
    data[1] = (pos_int >> 8) & 0xFF;  // Position MSB
    data[2] = vel_int & 0xFF;         // Velocity LSB
    data[3] = (vel_int >> 8) & 0xFF;  // Velocity MSB
    data[4] = torque_int & 0xFF;      // Torque LSB
    data[5] = (torque_int >> 8) & 0xFF; // Torque MSB
    data[6] = kp_int & 0xFF;          // Kp LSB
    data[7] = (kp_int >> 8) & 0xFF;   // Kp MSB
    data[8] = kd_int & 0xFF;          // Kd LSB
    data[9] = (kd_int >> 8) & 0xFF;   // Kd MSB

    // Add padding bytes at the end (same as Python)
    data[10] = 0x00;
    data[11] = 0x00;

    if (debug_enabled_) {
        debug_print("HT MIT command packed: p=" + std::to_string(position) +
                    " (pos_int=" + std::to_string(pos_int) + ")" +
                    ", v=" + std::to_string(velocity) +
                    " (vel_int=" + std::to_string(vel_int) + ")" +
                    ", t=" + std::to_string(torque) +
                    " (torque_int=" + std::to_string(torque_int) + ")" +
                    ", kp=" + std::to_string(kp) +
                    " (kp_int=" + std::to_string(kp_int) + ")" +
                    ", kd=" + std::to_string(kd) +
                    " (kd_int=" + std::to_string(kd_int) + ")");
    }

    return data;
}

void HTMotor::unpack_ht_response(const std::vector<uint8_t>& data) {
    // Unpack HT response data based on Python implementation
    // HT motor status frame format (0x700/0x800 series):
    // - byte 0: error code
    // - bytes 1-2: position (int16, little-endian)
    // - bytes 3-4: velocity (int16, little-endian)
    // - bytes 5-6: torque (int16, little-endian)

    if (data.size() < 7) return;

    // Extract error code (first byte)
    error_ = data[0];

    // Extract position, velocity, and torque as int16 values (little-endian)
    int16_t pos_int = static_cast<int16_t>(data[1] | (data[2] << 8));
    int16_t vel_int = static_cast<int16_t>(data[3] | (data[4] << 8));
    int16_t torque_int = static_cast<int16_t>(data[5] | (data[6] << 8));

    // Convert to physical units (same as Python conversion)
    position_ = pos_int * 0.0001 * TURN_TO_RAD_;  // Convert to radians
    velocity_ = vel_int * 0.00025 * TURN_TO_RAD_; // Convert to rad/s
    torque_ = torque_int * torque_k_ + torque_d_;  // Convert to Nm

    if (debug_enabled_) {
        debug_print("HT response unpacked: p=" + std::to_string(position_) +
                    " (pos_int=" + std::to_string(pos_int) + ")" +
                    ", v=" + std::to_string(velocity_) +
                    " (vel_int=" + std::to_string(vel_int) + ")" +
                    ", t=" + std::to_string(torque_) +
                    " (torque_int=" + std::to_string(torque_int) + ")" +
                    ", error=" + std::to_string(error_));
    }
}

uint16_t HTMotor::float_to_ht_format(float value, float min_val, float max_val) {
    // Convert float to HT format (16-bit)
    float span = max_val - min_val;
    if (span == 0.0f) return 0;

    float normalized = (value - min_val) / span;
    normalized = std::clamp(normalized, 0.0f, 1.0f);

    return static_cast<uint16_t>(normalized * 65535.0f);
}

float HTMotor::ht_format_to_float(uint16_t raw_value, float min_val, float max_val) {
    // Convert HT format (16-bit) back to float
    float normalized = static_cast<float>(raw_value) / 65535.0f;
    return min_val + normalized * (max_val - min_val);
}

void HTMotor::debug_print(const std::string& message) {
    if (debug_enabled_) {
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        std::cout << "[" << timestamp << "] [HT-MOTOR-" << motor_id_ << "] " << message << std::endl;
    }
}

} // namespace ic_can