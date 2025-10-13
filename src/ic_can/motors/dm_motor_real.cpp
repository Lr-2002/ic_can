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

#include <ic_can/motors/dm_motor_real.hpp>
#include <iostream>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace ic_can {

DMMotorReal::DMMotorReal(int motor_id, uint32_t can_send_id, uint32_t can_recv_id,
                         double kp, double kd, double max_torque, bool debug)
    : BaseMotor(motor_id, MotorType::DM_DAMIAO, can_send_id, can_recv_id),
      motor_id_(motor_id), can_send_id_(can_send_id), can_recv_id_(can_recv_id),
      kp_(kp), kd_(kd), max_torque_(max_torque),
      position_(0.0), velocity_(0.0), torque_(0.0), temperature_(25.0),
      enabled_(false), debug_enabled_(debug) {

    // Set default limits (these can be customized based on motor type)
    position_limit_min_ = -95.5 * M_PI / 180.0; // Convert to radians
    position_limit_max_ = 95.5 * M_PI / 180.0;
    velocity_limit_max_ = 5.0; // rad/s
    torque_limit_max_ = max_torque_;

    command_data_.resize(8, 0); // MIT command is 8 bytes
    debug_print("DM motor created: ID=" + std::to_string(motor_id) +
                ", SendID=0x" + std::to_string(can_send_id) +
                ", RecvID=0x" + std::to_string(can_recv_id));
}

// ========== BaseMotor Interface Implementation ==========

bool DMMotorReal::enable() {
    enabled_ = true;
    debug_print("Motor enabled");
    return true;
}

bool DMMotorReal::disable() {
    enabled_ = false;
    debug_print("Motor disabled");
    return true;
}

bool DMMotorReal::set_zero() {
    debug_print("Setting zero position");
    // Send zero position command
    command_data_ = pack_mit_command(0.0, 0.0, 0.0);
    return true;
}

bool DMMotorReal::is_enabled() const {
    return enabled_;
}

bool DMMotorReal::set_position(double position, double velocity, double torque) {
    if (!enabled_) {
        debug_print("Cannot set position: motor not enabled");
        return false;
    }

    // Clamp values to limits
    position = std::clamp(position, position_limit_min_, position_limit_max_);
    velocity = std::clamp(velocity, -velocity_limit_max_, velocity_limit_max_);
    torque = std::clamp(torque, -torque_limit_max_, torque_limit_max_);

    command_data_ = pack_mit_command(position, velocity, torque);
    debug_print("Position set: p=" + std::to_string(position) +
                ", v=" + std::to_string(velocity) +
                ", t=" + std::to_string(torque));
    return true;
}

bool DMMotorReal::set_velocity(double velocity, double torque) {
    if (!enabled_) {
        debug_print("Cannot set velocity: motor not enabled");
        return false;
    }

    // Clamp values to limits
    velocity = std::clamp(velocity, -velocity_limit_max_, velocity_limit_max_);
    torque = std::clamp(torque, -torque_limit_max_, torque_limit_max_);

    command_data_ = pack_mit_command(position_, velocity, torque);
    debug_print("Velocity set: v=" + std::to_string(velocity) +
                ", t=" + std::to_string(torque));
    return true;
}

bool DMMotorReal::set_torque(double torque) {
    if (!enabled_) {
        debug_print("Cannot set torque: motor not enabled");
        return false;
    }

    // Clamp torque to limits
    torque = std::clamp(torque, -torque_limit_max_, torque_limit_max_);

    command_data_ = pack_mit_command(position_, velocity_, torque);
    debug_print("Torque set: t=" + std::to_string(torque));
    return true;
}

bool DMMotorReal::mit_control(double position, double velocity, double torque, double kp, double kd) {
    if (!enabled_) {
        debug_print("Cannot MIT control: motor not enabled");
        return false;
    }

    // Update control parameters
    kp_ = kp;
    kd_ = kd;

    // Clamp values to limits
    position = std::clamp(position, position_limit_min_, position_limit_max_);
    velocity = std::clamp(velocity, -velocity_limit_max_, velocity_limit_max_);
    torque = std::clamp(torque, -torque_limit_max_, torque_limit_max_);

    command_data_ = pack_mit_command(position, velocity, torque);
    debug_print("MIT control: p=" + std::to_string(position) +
                ", v=" + std::to_string(velocity) +
                ", t=" + std::to_string(torque) +
                ", kp=" + std::to_string(kp) +
                ", kd=" + std::to_string(kd));
    return true;
}

bool DMMotorReal::set_command(const MotorCommand& command) {
    if (command.use_position) {
        return set_position(command.position, command.velocity, command.torque);
    } else if (command.use_velocity) {
        return set_velocity(command.velocity, command.torque);
    } else if (command.use_torque) {
        return set_torque(command.torque);
    }
    return false;
}

double DMMotorReal::get_position() const {
    return position_;
}

double DMMotorReal::get_velocity() const {
    return velocity_;
}

double DMMotorReal::get_torque() const {
    return torque_;
}

double DMMotorReal::get_temperature() const {
    return temperature_;
}

void DMMotorReal::update_state() {
    // State is updated via process_response() called from CAN frame handling
}

// Additional virtual method implementations
void DMMotorReal::set_limits(const MotorLimits& limits) {
    position_limit_min_ = limits.min_position;
    position_limit_max_ = limits.max_position;
    velocity_limit_max_ = limits.max_velocity;
    torque_limit_max_ = limits.max_torque;
}

void DMMotorReal::set_max_position(double max_position) {
    position_limit_max_ = max_position;
}

void DMMotorReal::set_min_position(double min_position) {
    position_limit_min_ = min_position;
}

void DMMotorReal::set_max_velocity(double max_velocity) {
    velocity_limit_max_ = max_velocity;
}

void DMMotorReal::set_max_torque(double max_torque) {
    torque_limit_max_ = max_torque;
}

bool DMMotorReal::is_position_safe(double position) const {
    return position >= position_limit_min_ && position <= position_limit_max_;
}

bool DMMotorReal::is_velocity_safe(double velocity) const {
    return std::abs(velocity) <= velocity_limit_max_;
}

bool DMMotorReal::is_torque_safe(double torque) const {
    return std::abs(torque) <= torque_limit_max_;
}

bool DMMotorReal::is_temperature_safe(double temperature) const {
    return temperature <= 80.0; // Standard temperature limit
}

MotorState DMMotorReal::get_state() const {
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

void DMMotorReal::reset_error() {
    // TODO: Implement error reset logic
    debug_print("Error reset");
}

int DMMotorReal::get_motor_id() const {
    return motor_id_;
}

uint32_t DMMotorReal::get_can_send_id() const {
    return can_send_id_;
}

uint32_t DMMotorReal::get_can_recv_id() const {
    return can_recv_id_;
}

std::vector<uint8_t> DMMotorReal::get_command_data() const {
    return command_data_;
}

bool DMMotorReal::process_response(const std::vector<uint8_t>& data) {
    if (data.size() < 8) {
        debug_print("Invalid response data size: " + std::to_string(data.size()));
        return false;
    }

    unpack_mit_response(data);
    return true;
}

// ========== Damiao Motor Specific Methods ==========

void DMMotorReal::set_mit_params(double kp, double kd, double max_torque) {
    kp_ = kp;
    kd_ = kd;
    max_torque_ = max_torque;
    torque_limit_max_ = max_torque;
    debug_print("MIT params updated: kp=" + std::to_string(kp) +
                ", kd=" + std::to_string(kd) +
                ", max_tau=" + std::to_string(max_torque));
}

void DMMotorReal::enter_mit_mode() {
    // Send MIT mode entry command
    std::vector<uint8_t> mit_cmd = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC};
    command_data_ = mit_cmd;
    debug_print("Entering MIT mode");
}

void DMMotorReal::exit_mit_mode() {
    // Send exit MIT mode command
    std::vector<uint8_t> exit_cmd = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD};
    command_data_ = exit_cmd;
    debug_print("Exiting MIT mode");
}

void DMMotorReal::set_zero_position() {
    // Send zero position command
    std::vector<uint8_t> zero_cmd = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE};
    command_data_ = zero_cmd;
    debug_print("Setting zero position");
}

// ========== Private Methods ==========

std::vector<uint8_t> DMMotorReal::pack_mit_command(double position, double velocity, double torque) {
    std::vector<uint8_t> data(8, 0);

    // Pack data according to Damiao MIT protocol (similar to openarm implementation)
    // Data format: [p_des[31:16], p_des[15:0], v_des[15:0], kp[11:0], kd[11:0], tau_ff[12:0]]

    // Convert to fixed-point representations
    float p_des = static_cast<float>(position);
    float v_des = static_cast<float>(velocity);
    float kp = static_cast<float>(kp_);
    float kd = static_cast<float>(kd_);
    float tau_ff = static_cast<float>(torque);

    // Convert to uint32 with proper bit manipulation
    uint32_t p_int = float_to_uint32(p_des);
    uint16_t v_int = static_cast<uint16_t>(float_to_uint32(v_des) >> 16);
    uint16_t kp_int = static_cast<uint16_t>(float_to_uint32(kp) >> 20);
    uint16_t kd_int = static_cast<uint16_t>(float_to_uint32(kd) >> 20);
    uint16_t tau_int = static_cast<uint16_t>(float_to_uint32(tau_ff) >> 19);

    // Pack into 8-byte array
    data[0] = static_cast<uint8_t>((p_int >> 24) & 0xFF);
    data[1] = static_cast<uint8_t>((p_int >> 16) & 0xFF);
    data[2] = static_cast<uint8_t>((p_int >> 8) & 0xFF);
    data[3] = static_cast<uint8_t>(p_int & 0xFF);
    data[4] = static_cast<uint8_t>((v_int >> 8) & 0xFF);
    data[5] = static_cast<uint8_t>(v_int & 0xFF);
    data[6] = static_cast<uint8_t>((kp_int & 0x0F) << 4 | (kd_int >> 8) & 0x0F);
    data[7] = static_cast<uint8_t>(kd_int & 0xFF);

    // Add torque feedforward (this is a simplified packing)
    uint16_t tau_packed = std::min(tau_int, static_cast<uint16_t>(0x1FFF));
    data[6] |= static_cast<uint8_t>((tau_packed >> 5) & 0xE0);
    data[7] |= static_cast<uint8_t>((tau_packed & 0x1F) << 3);

    return data;
}

void DMMotorReal::unpack_mit_response(const std::vector<uint8_t>& data) {
    // Unpack MIT response data (similar to openarm implementation)
    // Response format: [id, temperature[7:0], position[31:16], position[15:0], velocity[15:0], current[15:0]]

    if (data.size() < 6) return;

    // Extract position (float)
    uint32_t p_raw = (static_cast<uint32_t>(data[1]) << 24) |
                     (static_cast<uint32_t>(data[2]) << 16) |
                     (static_cast<uint32_t>(data[3]) << 8) |
                     static_cast<uint32_t>(data[4]);

    // Extract velocity (int16)
    int16_t v_raw = (static_cast<int16_t>(data[5]) << 8) |
                    static_cast<int16_t>(data[6]);

    // Extract current/torque (int16)
    int16_t tau_raw = (static_cast<int16_t>(data[7]) << 8) |
                      static_cast<int16_t>(data[8]);

    // Convert to real values
    position_ = uint32_to_float(p_raw);
    velocity_ = static_cast<double>(v_raw) * 0.01; // Scale factor
    torque_ = static_cast<double>(tau_raw) * 0.01;  // Scale factor
    temperature_ = static_cast<double>(data[0]);  // Temperature is first byte

    if (debug_enabled_) {
        debug_print("Response: p=" + std::to_string(position_) +
                    ", v=" + std::to_string(velocity_) +
                    ", t=" + std::to_string(torque_) +
                    ", temp=" + std::to_string(temperature_));
    }
}

uint32_t DMMotorReal::float_to_uint32(float value) {
    // Convert float to uint32 preserving bit pattern
    uint32_t result;
    std::memcpy(&result, &value, sizeof(float));
    return result;
}

float DMMotorReal::uint32_to_float(uint32_t value) {
    // Convert uint32 to float preserving bit pattern
    float result;
    std::memcpy(&result, &value, sizeof(uint32_t));
    return result;
}

void DMMotorReal::debug_print(const std::string& message) {
    if (debug_enabled_) {
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        std::cout << "[" << timestamp << "] [DM-MOTOR-" << motor_id_ << "] " << message << std::endl;
    }
}

} // namespace ic_can