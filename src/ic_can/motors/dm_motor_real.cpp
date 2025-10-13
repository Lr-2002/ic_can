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

    // Set correct Damiao motor limits based on dm-tools SDK
    // Default to DM4340 limits if not specified
    position_limit_min_ = -12.5;  // Q_MAX from dm-tools SDK (radians)
    position_limit_max_ = 12.5;
    velocity_limit_max_ = 10.0;   // DQ_MAX from dm-tools SDK (rad/s)
    torque_limit_max_ = 28.0;     // TAU_MAX from dm-tools SDK (Nm)

    command_data_.resize(8, 0); // MIT command is 8 bytes
    debug_print("DM motor created: ID=" + std::to_string(motor_id) +
                ", SendID=0x" + std::to_string(can_send_id) +
                ", RecvID=0x" + std::to_string(can_recv_id) +
                " (using DM4340 limits)");
}

// ========== BaseMotor Interface Implementation ==========

bool DMMotorReal::enable() {
    // Send motor enable command
    enter_mit_mode();
    enabled_ = true;
    debug_print("Motor enabled");
    return true;
}

bool DMMotorReal::disable() {
    // Send motor disable command
    exit_mit_mode();
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

void DMMotorReal::set_motor_type_limits(const std::string& motor_type) {
    // Set correct limits based on dm-tools SDK data
    if (motor_type == "DM10010L") {
        position_limit_min_ = -12.5;  // Q_MAX
        position_limit_max_ = 12.5;
        velocity_limit_max_ = 25.0;   // DQ_MAX
        torque_limit_max_ = 200.0;    // TAU_MAX
        debug_print("Set DM10010L limits: ±12.5 rad, 25 rad/s, 200 Nm");
    } else if (motor_type == "DM6248") {
        position_limit_min_ = -12.566;
        position_limit_max_ = 12.566;
        velocity_limit_max_ = 20.0;
        torque_limit_max_ = 120.0;
        debug_print("Set DM6248 limits: ±12.566 rad, 20 rad/s, 120 Nm");
    } else if (motor_type == "DM4340") {
        position_limit_min_ = -12.5;
        position_limit_max_ = 12.5;
        velocity_limit_max_ = 10.0;
        torque_limit_max_ = 28.0;
        debug_print("Set DM4340 limits: ±12.5 rad, 10 rad/s, 28 Nm");
    } else if (motor_type == "DM4310") {
        position_limit_min_ = -12.5;
        position_limit_max_ = 12.5;
        velocity_limit_max_ = 30.0;
        torque_limit_max_ = 10.0;
        debug_print("Set DM4310 limits: ±12.5 rad, 30 rad/s, 10 Nm");
    } else {
        // Default to DM4340
        position_limit_min_ = -12.5;
        position_limit_max_ = 12.5;
        velocity_limit_max_ = 10.0;
        torque_limit_max_ = 28.0;
        debug_print("Unknown motor type, using DM4340 limits: ±12.5 rad, 10 rad/s, 28 Nm");
    }
}

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
    // Send MIT mode entry command (Motor Enable)
    std::vector<uint8_t> enable_cmd = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC};
    command_data_ = enable_cmd;
    debug_print("Motor enable command sent");
}

void DMMotorReal::exit_mit_mode() {
    // Send exit MIT mode command (Motor Disable)
    std::vector<uint8_t> disable_cmd = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD};
    command_data_ = disable_cmd;
    debug_print("Motor disable command sent");
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

    // Use dm-tools SDK MIT command packing (from test_dm_fixed.cpp - working implementation)
    auto float_to_uint = [](double x, double min, double max, int bits) -> uint16_t {
        double span = max - min;
        if (x < min) x = min;
        if (x > max) x = max;
        return static_cast<uint16_t>((x - min) * ((1 << bits) - 1) / span);
    };

    // Convert to uint16 with proper limits from dm-tools SDK
    uint16_t kp_uint = float_to_uint(kp_, 0.0, 500.0, 12);
    uint16_t kd_uint = float_to_uint(kd_, 0.0, 5.0, 12);
    uint16_t q_uint = float_to_uint(position, -position_limit_max_, position_limit_max_, 16);
    uint16_t dq_uint = float_to_uint(velocity, -velocity_limit_max_, velocity_limit_max_, 12);
    uint16_t tau_uint = float_to_uint(torque, -torque_limit_max_, torque_limit_max_, 12);

    // Pack according to dm-tools format (from working test_dm_fixed.cpp)
    data[0] = (q_uint >> 8) & 0xFF;
    data[1] = q_uint & 0xFF;
    data[2] = dq_uint >> 4;
    data[3] = ((dq_uint & 0xF) << 4) | ((kp_uint >> 8) & 0xF);
    data[4] = kp_uint & 0xFF;
    data[5] = kd_uint >> 4;
    data[6] = ((kd_uint & 0xF) << 4) | ((tau_uint >> 8) & 0xF);
    data[7] = tau_uint & 0xFF;

    if (debug_enabled_) {
        debug_print("MIT command packed: p=" + std::to_string(position) +
                    " (q_uint=0x" + std::to_string(q_uint) + ")" +
                    ", v=" + std::to_string(velocity) +
                    " (dq_uint=0x" + std::to_string(dq_uint) + ")" +
                    ", t=" + std::to_string(torque) +
                    " (tau_uint=0x" + std::to_string(tau_uint) + ")" +
                    ", kp=" + std::to_string(kp_) +
                    " (kp_uint=0x" + std::to_string(kp_uint) + ")" +
                    ", kd=" + std::to_string(kd_) +
                    " (kd_uint=0x" + std::to_string(kd_uint) + ")");
    }

    return data;
}

void DMMotorReal::unpack_mit_response(const std::vector<uint8_t>& data) {
    // Unpack MIT response using working implementation from test_dm_fixed.cpp
    // Response format based on dm-tools SDK feedback callback

    if (data.size() < 6) return;

    // Extract motor response data (same as motor_feedback_callback from test_dm_fixed.cpp)
    uint16_t q_uint = (uint16_t(data[1]) << 8) | data[2];
    uint16_t dq_uint = (uint16_t(data[3]) << 4) | (data[4] >> 4);
    uint16_t tau_uint = (uint16_t(data[4] & 0xf) << 8) | data[5];

    // Use the same conversion logic as the working test
    auto uint_to_float = [](uint16_t x, float xmin, float xmax, uint8_t bits) -> float {
        float span = xmax - xmin;
        float data_norm = float(x) / ((1 << bits) - 1);
        float data = data_norm * span + xmin;
        return data;
    };

    // Convert to real values using current motor limits
    position_ = uint_to_float(q_uint, static_cast<float>(-position_limit_max_), static_cast<float>(position_limit_max_), 16);
    velocity_ = uint_to_float(dq_uint, static_cast<float>(-velocity_limit_max_), static_cast<float>(velocity_limit_max_), 12);
    torque_ = uint_to_float(tau_uint, static_cast<float>(-torque_limit_max_), static_cast<float>(torque_limit_max_), 12);

    if (debug_enabled_) {
        debug_print("Response unpacked: p=" + std::to_string(position_) +
                    " (q_uint=0x" + std::to_string(q_uint) + ")" +
                    ", v=" + std::to_string(velocity_) +
                    " (dq_uint=0x" + std::to_string(dq_uint) + ")" +
                    ", t=" + std::to_string(torque_) +
                    " (tau_uint=0x" + std::to_string(tau_uint) + ")");
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