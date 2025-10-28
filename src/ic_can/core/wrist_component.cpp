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

#include <ic_can/core/wrist_component.hpp>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace ic_can {

WristComponent::WristComponent()
    : MotorProtocolBase("WristHT", HT_MIN_CAN_ID, HT_MAX_CAN_ID)
    , target_position_(2, 0.0)  // [pitch, roll]
    , pitch_min_(DEFAULT_PITCH_MIN)
    , pitch_max_(DEFAULT_PITCH_MAX)
    , roll_min_(DEFAULT_ROLL_MIN)
    , roll_max_(DEFAULT_ROLL_MAX)
    , max_velocity_(DEFAULT_MAX_VELOCITY)
    , initialized_(false)
    , last_error_("") {

    std::cout << "🔧 WristComponent created for HT motors (m7-m8)" << std::endl;
    std::cout << "   CAN ID range: [0x" << std::hex << HT_MIN_CAN_ID
              << ", 0x" << HT_MAX_CAN_ID << std::dec << "]" << std::endl;
}

// ========== CANProtocolInterface Implementation ==========

bool WristComponent::process_can_frame(const CANFrame& frame) {
    if (!enabled_) {
        return false;
    }

    // Check if frame ID is within HT motor range
    if (frame.id < HT_MIN_CAN_ID || frame.id > HT_MAX_CAN_ID) {
        return false;
    }

    // Extract motor ID from CAN ID
    // HT motors use 0x700 series (m7) and 0x800 series (m8)
    int motor_id = -1;
    if (frame.id >= 0x700 && frame.id < 0x800) {
        motor_id = 7;  // Pitch motor
    } else if (frame.id >= 0x800 && frame.id < 0x900) {
        motor_id = 8;  // Roll motor
    }

    if (motor_id == -1 || !is_valid_motor_id(motor_id)) {
        if (debug_enabled_) {
            std::cout << "⚠️  WristComponent: Invalid motor ID from CAN frame 0x"
                      << std::hex << frame.id << std::dec << std::endl;
        }
        return false;
    }

    auto motor = get_motor(motor_id);
    if (!motor) {
        if (debug_enabled_) {
            std::cout << "⚠️  WristComponent: Motor " << motor_id << " not found for frame processing" << std::endl;
        }
        return false;
    }

    // Process the frame with the motor
    bool success = motor->process_response(frame.data);
    if (success && debug_enabled_) {
        std::cout << "📥 WristComponent: Processed frame for motor " << motor_id
                  << " (ID: 0x" << std::hex << frame.id << std::dec << ")" << std::endl;
    }

    // Update wrist state after motor state update
    update_wrist_state();

    return success;
}

// ========== Wrist-Specific Control Interface ==========

bool WristComponent::set_pitch_angle(double pitch_angle, double velocity) {
    auto motor7 = get_motor(7);
    if (!motor7) {
        std::lock_guard<std::mutex> lock(status_mutex_);
        last_error_ = "Motor 7 (pitch) not found";
        return false;
    }

    double clamped_angle = clamp_angle(pitch_angle, pitch_min_, pitch_max_);
    double clamped_velocity = clamp_velocity(velocity);

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        target_position_[0] = clamped_angle;
    }

    bool success = motor7->set_position(clamped_angle, clamped_velocity);
    if (debug_enabled_) {
        std::cout << "🎯 WristComponent: Set pitch to " << std::fixed << std::setprecision(3)
                  << clamped_angle << " rad (" << (clamped_angle * 180.0 / M_PI) << "°)" << std::endl;
    }

    return success;
}

bool WristComponent::set_roll_angle(double roll_angle, double velocity) {
    auto motor8 = get_motor(8);
    if (!motor8) {
        std::lock_guard<std::mutex> lock(status_mutex_);
        last_error_ = "Motor 8 (roll) not found";
        return false;
    }

    double clamped_angle = clamp_angle(roll_angle, roll_min_, roll_max_);
    double clamped_velocity = clamp_velocity(velocity);

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        target_position_[1] = clamped_angle;
    }

    bool success = motor8->set_position(clamped_angle, clamped_velocity);
    if (debug_enabled_) {
        std::cout << "🎯 WristComponent: Set roll to " << std::fixed << std::setprecision(3)
                  << clamped_angle << " rad (" << (clamped_angle * 180.0 / M_PI) << "°)" << std::endl;
    }

    return success;
}

bool WristComponent::set_wrist_pose(double pitch_angle, double roll_angle, double velocity) {
    bool pitch_success = set_pitch_angle(pitch_angle, velocity);
    bool roll_success = set_roll_angle(roll_angle, velocity);
    return pitch_success && roll_success;
}

double WristComponent::get_pitch_angle() const {
    auto motor7 = get_motor(7);
    if (motor7) {
        return motor7->get_position();
    }
    return 0.0;
}

double WristComponent::get_roll_angle() const {
    auto motor8 = get_motor(8);
    if (motor8) {
        return motor8->get_position();
    }
    return 0.0;
}

std::vector<double> WristComponent::get_wrist_pose() const {
    return {get_pitch_angle(), get_roll_angle()};
}

std::vector<double> WristComponent::get_positions() const {
    std::vector<double> positions;

    auto motor7 = get_motor(7);
    if (motor7) {
        positions.push_back(motor7->get_position());
    } else {
        positions.push_back(0.0);
    }

    auto motor8 = get_motor(8);
    if (motor8) {
        positions.push_back(motor8->get_position());
    } else {
        positions.push_back(0.0);
    }

    return positions;
}

std::vector<double> WristComponent::get_velocities() const {
    std::vector<double> velocities;

    auto motor7 = get_motor(7);
    if (motor7) {
        velocities.push_back(motor7->get_velocity());
    } else {
        velocities.push_back(0.0);
    }

    auto motor8 = get_motor(8);
    if (motor8) {
        velocities.push_back(motor8->get_velocity());
    } else {
        velocities.push_back(0.0);
    }

    return velocities;
}

std::vector<double> WristComponent::get_torques() const {
    std::vector<double> torques;

    auto motor7 = get_motor(7);
    if (motor7) {
        torques.push_back(motor7->get_torque());
    } else {
        torques.push_back(0.0);
    }

    auto motor8 = get_motor(8);
    if (motor8) {
        torques.push_back(motor8->get_torque());
    } else {
        torques.push_back(0.0);
    }

    return torques;
}

std::vector<double> WristComponent::get_temperatures() const {
    std::vector<double> temperatures;

    auto motor7 = get_motor(7);
    if (motor7) {
        temperatures.push_back(motor7->get_temperature());
    } else {
        temperatures.push_back(0.0);
    }

    auto motor8 = get_motor(8);
    if (motor8) {
        temperatures.push_back(motor8->get_temperature());
    } else {
        temperatures.push_back(0.0);
    }

    return temperatures;
}

bool WristComponent::at_target_position(double tolerance) const {
    auto current_pos = get_positions();
    std::lock_guard<std::mutex> lock(state_mutex_);

    for (size_t i = 0; i < current_pos.size() && i < target_position_.size(); ++i) {
        if (std::abs(current_pos[i] - target_position_[i]) > tolerance) {
            return false;
        }
    }
    return true;
}

bool WristComponent::has_motor_errors() const {
    auto motor7 = get_motor(7);
    auto motor8 = get_motor(8);

    bool has_errors = false;
    if (motor7 && motor7->has_error()) {
        has_errors = true;
    }
    if (motor8 && motor8->has_error()) {
        has_errors = true;
    }

    return has_errors;
}

std::vector<int> WristComponent::get_error_motor_ids() const {
    std::vector<int> error_ids;

    auto motor7 = get_motor(7);
    if (motor7 && motor7->has_error()) {
        error_ids.push_back(7);
    }

    auto motor8 = get_motor(8);
    if (motor8 && motor8->has_error()) {
        error_ids.push_back(8);
    }

    return error_ids;
}

// ========== HT Motor Specific Methods ==========

bool WristComponent::set_ht_motor_params(int motor_id, double kp, double kd, double max_torque) {
    if (!is_valid_motor_id(motor_id)) {
        std::lock_guard<std::mutex> lock(status_mutex_);
        last_error_ = "Invalid motor ID: " + std::to_string(motor_id);
        return false;
    }

    auto motor = get_motor(motor_id);
    if (!motor) {
        std::lock_guard<std::mutex> lock(status_mutex_);
        last_error_ = "Motor " + std::to_string(motor_id) + " not found";
        return false;
    }

    // Try to cast to HT motor and set parameters
    // Note: This would need dynamic_cast if we want HT-specific methods
    // For now, use generic motor interface
    motor->set_max_torque(max_torque);

    if (debug_enabled_) {
        std::cout << "⚙️  WristComponent: Set motor " << motor_id
                  << " params - kp: " << kp << ", kd: " << kd
                  << ", max_torque: " << max_torque << std::endl;
    }

    return true;
}

bool WristComponent::send_mit_command(int motor_id, double position, double velocity,
                                       double torque, double kp, double kd) {
    if (!is_valid_motor_id(motor_id)) {
        std::lock_guard<std::mutex> lock(status_mutex_);
        last_error_ = "Invalid motor ID: " + std::to_string(motor_id);
        return false;
    }

    auto motor = get_motor(motor_id);
    if (!motor) {
        std::lock_guard<std::mutex> lock(status_mutex_);
        last_error_ = "Motor " + std::to_string(motor_id) + " not found";
        return false;
    }

    bool success = motor->mit_control(position, velocity, torque, kp, kd);
    if (debug_enabled_) {
        std::cout << "📤 WristComponent: Sent MIT command to motor " << motor_id
                  << " - p: " << position << ", v: " << velocity
                  << ", t: " << torque << ", kp: " << kp << ", kd: " << kd << std::endl;
    }

    return success;
}

// ========== Configuration ==========

void WristComponent::set_pitch_limits(double min_angle, double max_angle) {
    pitch_min_ = min_angle;
    pitch_max_ = max_angle;
    std::cout << "✅ WristComponent: Pitch limits set to ["
              << (min_angle * 180.0 / M_PI) << "°, " << (max_angle * 180.0 / M_PI) << "°]" << std::endl;
}

void WristComponent::set_roll_limits(double min_angle, double max_angle) {
    roll_min_ = min_angle;
    roll_max_ = max_angle;
    std::cout << "✅ WristComponent: Roll limits set to ["
              << (min_angle * 180.0 / M_PI) << "°, " << (max_angle * 180.0 / M_PI) << "°]" << std::endl;
}

std::pair<double, double> WristComponent::get_pitch_limits() const {
    return {pitch_min_, pitch_max_};
}

std::pair<double, double> WristComponent::get_roll_limits() const {
    return {roll_min_, roll_max_};
}

std::string WristComponent::get_state_summary() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    std::lock_guard<std::mutex> status_lock(status_mutex_);

    std::ostringstream ss;
    ss << "WristComponent State:\n";
    ss << "  Enabled: " << (enabled_ ? "Yes" : "No") << "\n";
    ss << "  Motors: " << get_motor_count() << "/2\n";
    ss << "  Target Pose: [" << std::fixed << std::setprecision(3)
       << target_position_[0] << ", " << target_position_[1] << "] rad\n";

    auto current_pose = get_wrist_pose();
    ss << "  Current Pose: [" << current_pose[0] << ", " << current_pose[1] << "] rad\n";

    ss << "  At Target: " << (at_target_position() ? "Yes" : "No") << "\n";
    ss << "  Has Errors: " << (has_motor_errors() ? "Yes" : "No") << "\n";

    if (!last_error_.empty()) {
        ss << "  Last Error: " << last_error_ << "\n";
    }

    return ss.str();
}

// ========== Helper Methods ==========

bool WristComponent::is_valid_motor_id(int motor_id) const {
    return motor_id == 7 || motor_id == 8;
}

double WristComponent::clamp_angle(double angle, double min_angle, double max_angle) const {
    return std::clamp(angle, min_angle, max_angle);
}

double WristComponent::clamp_velocity(double velocity) const {
    return std::clamp(velocity, -max_velocity_, max_velocity_);
}

void WristComponent::update_wrist_state() {
    // This method can be used to update any internal wrist state
    // based on motor states. For now, the state is computed on-demand.
    if (debug_enabled_) {
        auto pose = get_wrist_pose();
        std::cout << "🔄 WristComponent: State updated - pose ["
                  << pose[0] << ", " << pose[1] << "] rad" << std::endl;
    }
}

// ========== Factory Function ==========

std::unique_ptr<WristComponent> create_wrist_component() {
    auto wrist = std::make_unique<WristComponent>();
    std::cout << "✅ WristComponent created successfully" << std::endl;
    return wrist;
}

} // namespace ic_can