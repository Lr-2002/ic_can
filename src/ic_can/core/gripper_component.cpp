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

#include "ic_can/core/gripper_component.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace ic_can {

GripperComponent::GripperComponent()
    : MotorProtocolBase("GripperServo", 0x19, 0x19)
    , min_openness_(DEFAULT_MIN_OPENNESS)
    , max_openness_(DEFAULT_MAX_OPENNESS)
    , max_speed_(DEFAULT_MAX_SPEED)
    , max_force_(DEFAULT_MAX_FORCE) {

    debug_print("GripperComponent created for servo motor (m9)");
    std::cout << "   CAN Receive ID: 0x" << std::hex << 0x19
              << std::dec << " (m9 feedback)" << std::endl;
}

// ========== CANProtocolInterface Implementation ==========

bool GripperComponent::process_can_frame(const CANFrame& frame) {
    if (!enabled_) {
        return false;
    }

    // Check if frame ID is for servo motor (m9)
    if (frame.id != 0x19) {
        return false;
    }

    // Extract motor ID from CAN ID (servo motors typically use direct mapping)
    int motor_id = 9;  // Servo motor is always m9

    if (!is_valid_motor_id(motor_id)) {
        if (debug_enabled_) {
            debug_print("Invalid motor ID from CAN frame 0x" + std::to_string(frame.id));
        }
        return false;
    }

    auto motor = get_motor(motor_id);
    if (!motor) {
        if (debug_enabled_) {
            debug_print("Motor " + std::to_string(motor_id) + " not found for frame processing");
        }
        return false;
    }

    // Process the frame with the motor
    bool success = motor->process_response(frame.data);
    if (success && debug_enabled_) {
        std::ostringstream oss;
        oss << "Processed frame for motor " << motor_id
            << " (ID: 0x" << std::hex << frame.id << std::dec << ")";
        debug_print(oss.str());
    }

    // Update gripper state after motor state update
    update_gripper_state();

    return success;
}

// ========== Gripper Control Interface Implementation ==========

bool GripperComponent::open(double speed, double force) {
    return set_openness(1.0, speed, force);  // Fully open
}

bool GripperComponent::close(double speed, double force) {
    return set_openness(0.0, speed, force);  // Fully closed
}

bool GripperComponent::stop() {
    auto motor9 = get_motor(9);
    if (!motor9) {
        return false;
    }

    return motor9->set_velocity(0.0, 0.0);
}

bool GripperComponent::set_openness(double openness, double speed, double force) {
    auto motor9 = get_motor(9);
    if (!motor9) {
        return false;
    }

    // Clamp values to limits
    double clamped_openness = std::clamp(openness, min_openness_, max_openness_);
    double clamped_speed = std::clamp(speed, 0.0, max_speed_);

    // Convert openness to servo angle
    double target_angle = openness_to_angle(clamped_openness);

    // Set motor position (converted from angle to radians)
    double target_position = target_angle * M_PI / 180.0;

    bool success = motor9->set_position(target_position, clamped_speed);
    if (success) {
        current_openness_ = clamped_openness;
        is_moving_ = true;

        if (debug_enabled_) {
            debug_print("Set openness to " + std::to_string(clamped_openness) +
                        " (angle: " + std::to_string(target_angle) + "°)");
        }
    }

    return success;
}

double GripperComponent::get_openness() const {
    return current_openness_.load();
}

bool GripperComponent::set_servo_angle(double angle, double speed) {
    auto motor9 = get_motor(9);
    if (!motor9) {
        return false;
    }

    // Clamp angle to servo limits
    double clamped_angle = std::clamp(angle, -SERVO_ANGLE_RANGE/2, SERVO_ANGLE_RANGE/2);
    double clamped_speed = std::clamp(speed, 0.0, max_speed_);

    // Convert angle to radians and set position
    double target_position = clamped_angle * M_PI / 180.0;

    bool success = motor9->set_position(target_position, clamped_speed);
    if (success) {
        is_moving_ = true;

        if (debug_enabled_) {
            debug_print("Set servo angle to " + std::to_string(clamped_angle) + "°");
        }
    }

    return success;
}

double GripperComponent::get_servo_angle() const {
    auto motor9 = get_motor(9);
    if (!motor9) {
        return 0.0;
    }

    // Get position and convert to degrees
    double position = motor9->get_position();
    return position * 180.0 / M_PI;
}

bool GripperComponent::set_servo_pulse_width(uint16_t pulse_width_us) {
    auto motor9 = get_motor(9);
    if (!motor9) {
        return false;
    }

    // Convert pulse width to angle
    double target_angle = pulse_width_to_angle(pulse_width_us);

    return set_servo_angle(target_angle, 0.5);  // Use medium speed
}

std::string GripperComponent::get_state_summary() const {
    std::ostringstream ss;
    ss << "GripperComponent State:\n";
    ss << "  Enabled: " << (enabled_ ? "Yes" : "No") << "\n";
    ss << "  Motors: " << get_motor_count() << "/1\n";
    ss << "  Openness: " << std::fixed << std::setprecision(3)
       << current_openness_.load() << " (0.0=closed, 1.0=open)\n";
    ss << "  Servo Angle: " << get_servo_angle() << "°\n";
    ss << "  Moving: " << (is_moving_.load() ? "Yes" : "No") << "\n";
    ss << "  Grasping: " << (is_grasping_.load() ? "Yes" : "No") << "\n";

    return ss.str();
}

// ========== Helper Methods Implementation ==========

bool GripperComponent::is_valid_motor_id(int motor_id) const {
    return motor_id == 9;  // Only servo motor m9 is valid
}

double GripperComponent::clamp_angle(double angle, double min_angle, double max_angle) const {
    return std::clamp(angle, min_angle, max_angle);
}

double GripperComponent::clamp_velocity(double velocity) const {
    return std::clamp(velocity, -max_speed_, max_speed_);
}

// ========== Private Methods Implementation ==========

double GripperComponent::openness_to_angle(double openness) {
    // Map openness (0.0-1.0) to servo angle (-90° to +90°)
    return (openness - 0.5) * SERVO_ANGLE_RANGE;
}

double GripperComponent::angle_to_openness(double angle) {
    // Map servo angle (-90° to +90°) to openness (0.0-1.0)
    return (angle / SERVO_ANGLE_RANGE) + 0.5;
}

double GripperComponent::angle_to_pulse_width(double angle) {
    // Convert angle to pulse width (1000-2000 μs for 180° range)
    double normalized_angle = (angle + SERVO_ANGLE_RANGE/2) / SERVO_ANGLE_RANGE;
    return SERVO_MIN_PULSE_US + normalized_angle * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US);
}

double GripperComponent::pulse_width_to_angle(double pulse_width) {
    // Convert pulse width to angle
    double normalized = (pulse_width - SERVO_MIN_PULSE_US) / (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US);
    return normalized * SERVO_ANGLE_RANGE - SERVO_ANGLE_RANGE/2;
}

void GripperComponent::update_gripper_state() {
    auto motor9 = get_motor(9);
    if (!motor9) {
        return;
    }

    // Get current motor position and convert to openness
    double position = motor9->get_position();
    double angle = position * 180.0 / M_PI;
    current_openness_ = angle_to_openness(angle);

    // Update moving status based on velocity
    double velocity = motor9->get_velocity();
    is_moving_ = std::abs(velocity) > 0.01;

    // Update grasping status based on force or position
    double torque = motor9->get_torque();
    is_grasping_ = torque > 0.1;  // Simple threshold for grasping detection

    if (debug_enabled_) {
        debug_print("Updated state - openness: " + std::to_string(current_openness_.load()) +
                    ", angle: " + std::to_string(angle) + "°, moving: " +
                    (is_moving_.load() ? "yes" : "no"));
    }
}

void GripperComponent::debug_print(const std::string& message) {
    if (debug_enabled_) {
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        std::cout << "[" << timestamp << "] [GRIPPER-SERVO] " << message << std::endl;
    }
}

// ========== Additional Helper Methods ==========

bool GripperComponent::is_fully_open() const {
    return current_openness_.load() >= 0.95;
}

bool GripperComponent::is_fully_closed() const {
    return current_openness_.load() <= 0.05;
}

bool GripperComponent::is_moving() const {
    return is_moving_.load();
}

bool GripperComponent::set_openness_limits(double min_openness, double max_openness) {
    if (min_openness >= 0.0 && max_openness <= 1.0 && min_openness < max_openness) {
        min_openness_ = min_openness;
        max_openness_ = max_openness;
        debug_print("Set openness limits: [" + std::to_string(min_openness) + ", " +
                    std::to_string(max_openness) + "]");
        return true;
    }
    return false;
}

} // namespace ic_can