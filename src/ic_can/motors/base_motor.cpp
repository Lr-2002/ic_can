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

#include "ic_can/motors/base_motor.hpp"
#include <iostream>
#include <cmath>
#include <chrono>

namespace ic_can {

BaseMotor::BaseMotor(int motor_id, MotorType type, uint32_t can_send_id, uint32_t can_recv_id)
    : motor_id_(motor_id), type_(type), can_send_id_(can_send_id), can_recv_id_(can_recv_id),
      position_(0.0), velocity_(0.0), torque_(0.0), temperature_(25.0),
      enabled_(false), error_(false) {

    // 根据电机类型设置默认限制参数
    switch (type_) {
        case MotorType::DM_DAMIAO:
            limits_.max_position = 3.14159;    // ±180度
            limits_.min_position = -3.14159;
            limits_.max_velocity = 5.0;        // 5 rad/s
            limits_.max_torque = 10.0;         // 10 Nm
            limits_.max_temperature = 80.0;     // 80°C
            break;

        case MotorType::HT_HIGH_TORQUE:
            limits_.max_position = 6.28318;    // ±360度
            limits_.min_position = -6.28318;
            limits_.max_velocity = 2.0;        // 2 rad/s (较慢)
            limits_.max_torque = 20.0;         // 20 Nm (更高扭矩)
            limits_.max_temperature = 85.0;     // 85°C
            break;

        case MotorType::SERVO:
            limits_.max_position = 3.14159;    // ±180度 (典型舵机)
            limits_.min_position = -3.14159;
            limits_.max_velocity = 10.0;       // 10 rad/s (较快)
            limits_.max_torque = 5.0;          // 5 Nm
            limits_.max_temperature = 70.0;     // 70°C
            break;
    }

    // 初始化命令参数
    last_command_.kp = 0.0;
    last_command_.kd = 0.0;
    last_command_.position = 0.0;
    last_command_.velocity = 0.0;
    last_command_.torque = 0.0;
}

MotorState BaseMotor::get_state() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return get_state_snapshot();
}

void BaseMotor::set_limits(const MotorLimits& limits) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    limits_ = limits;
}

void BaseMotor::set_max_position(double max_position) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    limits_.max_position = max_position;
}

void BaseMotor::set_min_position(double min_position) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    limits_.min_position = min_position;
}

void BaseMotor::set_max_velocity(double max_velocity) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    limits_.max_velocity = max_velocity;
}

void BaseMotor::set_max_torque(double max_torque) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    limits_.max_torque = max_torque;
}

bool BaseMotor::is_position_safe(double position) const {
    return (position >= limits_.min_position) && (position <= limits_.max_position);
}

bool BaseMotor::is_velocity_safe(double velocity) const {
    return std::abs(velocity) <= limits_.max_velocity;
}

bool BaseMotor::is_torque_safe(double torque) const {
    return std::abs(torque) <= limits_.max_torque;
}

bool BaseMotor::is_temperature_safe(double temperature) const {
    return temperature <= limits_.max_temperature;
}

bool BaseMotor::is_command_safe(const MotorCommand& command) const {
    if (command.use_position && !is_position_safe(command.position)) {
        return false;
    }
    if (command.use_velocity && !is_velocity_safe(command.velocity)) {
        return false;
    }
    if (command.use_torque && !is_torque_safe(command.torque)) {
        return false;
    }
    return true;
}

void BaseMotor::update_internal_state(double position, double velocity,
                                    double torque, double temperature) {
    position_.store(position);
    velocity_.store(velocity);
    torque_.store(torque);
    temperature_.store(temperature);
}

void BaseMotor::set_enabled_state(bool enabled) {
    enabled_.store(enabled);
}

void BaseMotor::set_error_state(bool error) {
    error_.store(error);
}

MotorState BaseMotor::get_state_snapshot() const {
    MotorState state;
    state.position = position_.load();
    state.velocity = velocity_.load();
    state.torque = torque_.load();
    state.temperature = temperature_.load();
    state.enabled = enabled_.load();
    state.error = error_.load();
    state.timestamp = std::chrono::steady_clock::now();
    return state;
}

} // namespace ic_can