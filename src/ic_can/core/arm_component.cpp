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

#include "ic_can/core/arm_component.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>

namespace ic_can {

ArmComponent::ArmComponent()
    : MotorProtocolBase("ArmDM", 0x11, 0x16) {

    debug_print("ArmComponent created for DM motors (m1-m6)");
    std::cout << "   Motor types: DM10010L(m1), DM6248(m2,m3), DM4340(m4,m5), DM4310(m6)" << std::endl;
    std::cout << "   CAN Receive ID range: [0x" << std::hex << 0x11
              << ", 0x" << 0x16 << std::dec << "] (m1-m6 feedback)" << std::endl;
}

// ========== CANProtocolInterface Implementation ==========

bool ArmComponent::process_can_frame(const CANFrame& frame) {
    if (!enabled_) {
        return false;
    }

    // Check if frame ID is within DM motor range
    if (frame.id < DM_MIN_CAN_ID || frame.id > DM_MAX_CAN_ID) {
        return false;
    }

    // Extract motor ID from CAN ID (DM motors typically use direct mapping)
    int motor_id = static_cast<int>(frame.id & 0x0F);  // Lower 4 bits for motor ID
    if (motor_id < 1 || motor_id > 6) {
        return false;  // Not an arm motor
    }

    if (!is_valid_motor_id(motor_id)) {
        if (debug_enabled_) {
            debug_print("Invalid motor ID from CAN frame: " + std::to_string(motor_id));
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

    // Update arm state after motor state update
    update_arm_state();

    return success;
}

// ========== 批量控制接口 ==========

bool ArmComponent::enable_all() {
    bool success = true;
    for (int motor_id : JOINT_MOTOR_IDS) {
        auto motor = get_motor(motor_id);
        if (motor && !motor->enable()) {
            success = false;
            debug_print("Failed to enable motor " + std::to_string(motor_id));
        }
    }

    if (success) {
        debug_print("All arm motors enabled");
    }
    return success;
}

bool ArmComponent::disable_all() {
    bool success = true;
    for (int motor_id : JOINT_MOTOR_IDS) {
        auto motor = get_motor(motor_id);
        if (motor && !motor->disable()) {
            success = false;
            debug_print("Failed to disable motor " + std::to_string(motor_id));
        }
    }

    if (success) {
        debug_print("All arm motors disabled");
    }
    return success;
}

bool ArmComponent::set_zero_all() {
    bool success = true;
    for (int motor_id : JOINT_MOTOR_IDS) {
        auto motor = get_motor(motor_id);
        if (motor && !motor->set_zero()) {
            success = false;
            debug_print("Failed to set zero for motor " + std::to_string(motor_id));
        }
    }

    if (success) {
        debug_print("All arm motors zero set");
    }
    return success;
}

bool ArmComponent::stop_all_motion() {
    bool success = true;
    for (int motor_id : JOINT_MOTOR_IDS) {
        auto motor = get_motor(motor_id);
        if (motor && !motor->set_velocity(0.0, 0.0)) {
            success = false;
            debug_print("Failed to stop motor " + std::to_string(motor_id));
        }
    }

    if (success) {
        debug_print("All arm motion stopped");
    }
    return success;
}

// ========== 位置控制接口 ==========

bool ArmComponent::set_positions(const std::vector<double>& positions,
                                 const std::vector<double>& velocities) {
    if (!is_valid_array_length(positions, NUM_JOINTS)) {
        debug_print("Invalid positions array length");
        return false;
    }

    if (!velocities.empty() && !is_valid_array_length(velocities, NUM_JOINTS)) {
        debug_print("Invalid velocities array length");
        return false;
    }

    bool success = true;
    for (size_t i = 0; i < NUM_JOINTS; ++i) {
        double clamped_position = clamp_position(positions[i], static_cast<int>(i));
        double velocity = velocities.empty() ? 0.0 : clamp_velocity(velocities[i]);

        auto motor = get_motor(JOINT_MOTOR_IDS[i]);
        if (motor && !motor->set_position(clamped_position, velocity)) {
            success = false;
            debug_print("Failed to set position for joint " + std::to_string(i));
        }
    }

    if (success && debug_enabled_) {
        debug_print("Set all joint positions");
    }

    return success;
}

bool ArmComponent::set_single_position(int joint_index, double position, double velocity) {
    if (!is_valid_joint_index(joint_index)) {
        debug_print("Invalid joint index: " + std::to_string(joint_index));
        return false;
    }

    int motor_id = joint_to_motor_id(joint_index);
    double clamped_position = clamp_position(position, joint_index);
    double clamped_velocity = clamp_velocity(velocity);

    auto motor = get_motor(motor_id);
    if (!motor) {
        debug_print("Motor not found for joint " + std::to_string(joint_index));
        return false;
    }

    bool success = motor->set_position(clamped_position, clamped_velocity);
    if (success && debug_enabled_) {
        debug_print("Set joint " + std::to_string(joint_index) +
                   " position to " + std::to_string(clamped_position) + " rad");
    }

    return success;
}

std::vector<double> ArmComponent::get_positions() {
    std::vector<double> positions(NUM_JOINTS, 0.0);

    for (size_t i = 0; i < NUM_JOINTS; ++i) {
        auto motor = get_motor(JOINT_MOTOR_IDS[i]);
        if (motor) {
            positions[i] = motor->get_position();
        }
    }

    return positions;
}

double ArmComponent::get_single_position(int joint_index) {
    if (!is_valid_joint_index(joint_index)) {
        return 0.0;
    }

    int motor_id = joint_to_motor_id(joint_index);
    auto motor = get_motor(motor_id);
    return motor ? motor->get_position() : 0.0;
}

// ========== 速度控制接口 ==========

bool ArmComponent::set_velocities(const std::vector<double>& velocities) {
    if (!is_valid_array_length(velocities, NUM_JOINTS)) {
        debug_print("Invalid velocities array length");
        return false;
    }

    bool success = true;
    for (size_t i = 0; i < NUM_JOINTS; ++i) {
        double clamped_velocity = clamp_velocity(velocities[i]);

        auto motor = get_motor(JOINT_MOTOR_IDS[i]);
        if (motor && !motor->set_velocity(clamped_velocity, 0.0)) {
            success = false;
            debug_print("Failed to set velocity for joint " + std::to_string(i));
        }
    }

    if (success && debug_enabled_) {
        debug_print("Set all joint velocities");
    }

    return success;
}

std::vector<double> ArmComponent::get_velocities() {
    std::vector<double> velocities(NUM_JOINTS, 0.0);

    for (size_t i = 0; i < NUM_JOINTS; ++i) {
        auto motor = get_motor(JOINT_MOTOR_IDS[i]);
        if (motor) {
            velocities[i] = motor->get_velocity();
        }
    }

    return velocities;
}

// ========== 力矩控制接口 ==========

bool ArmComponent::set_torques(const std::vector<double>& torques) {
    if (!is_valid_array_length(torques, NUM_JOINTS)) {
        debug_print("Invalid torques array length");
        return false;
    }

    bool success = true;
    for (size_t i = 0; i < NUM_JOINTS; ++i) {
        double clamped_torque = clamp_torque(torques[i]);

        auto motor = get_motor(JOINT_MOTOR_IDS[i]);
        if (motor && !motor->set_torque(clamped_torque)) {
            success = false;
            debug_print("Failed to set torque for joint " + std::to_string(i));
        }
    }

    if (success && debug_enabled_) {
        debug_print("Set all joint torques");
    }

    return success;
}

std::vector<double> ArmComponent::get_torques() {
    std::vector<double> torques(NUM_JOINTS, 0.0);

    for (size_t i = 0; i < NUM_JOINTS; ++i) {
        auto motor = get_motor(JOINT_MOTOR_IDS[i]);
        if (motor) {
            torques[i] = motor->get_torque();
        }
    }

    return torques;
}

// ========== MIT控制接口 ==========

bool ArmComponent::mit_control(const std::vector<double>& positions,
                               const std::vector<double>& velocities,
                               const std::vector<double>& torques,
                               const std::vector<double>& kp_array,
                               const std::vector<double>& kd_array) {
    if (!is_valid_array_length(positions, NUM_JOINTS) ||
        !is_valid_array_length(velocities, NUM_JOINTS) ||
        !is_valid_array_length(torques, NUM_JOINTS) ||
        !is_valid_array_length(kp_array, NUM_JOINTS) ||
        !is_valid_array_length(kd_array, NUM_JOINTS)) {
        debug_print("Invalid array lengths for MIT control");
        return false;
    }

    bool success = true;
    for (size_t i = 0; i < NUM_JOINTS; ++i) {
        double clamped_position = clamp_position(positions[i], static_cast<int>(i));
        double clamped_velocity = clamp_velocity(velocities[i]);
        double clamped_torque = clamp_torque(torques[i]);

        auto motor = get_motor(JOINT_MOTOR_IDS[i]);
        if (motor) {
            // For DM motors with MIT protocol, we need to use MIT-specific control
            // This is a simplified implementation - real MIT control may need different method calls
            if (!motor->set_position(clamped_position, clamped_velocity, clamped_torque)) {
                success = false;
                debug_print("Failed to set MIT control for joint " + std::to_string(i));
            }
        }
    }

    if (success && debug_enabled_) {
        debug_print("Set MIT control for all joints");
    }

    return success;
}

// ========== 运动控制接口 ==========

bool ArmComponent::home_to_zero(double speed, double timeout) {
    if (is_homing_.load()) {
        debug_print("Homing already in progress");
        return false;
    }

    debug_print("Starting homing sequence");
    is_homing_ = true;

    // Enable all motors first
    if (!enable_all()) {
        is_homing_ = false;
        debug_print("Failed to enable motors for homing");
        return false;
    }

    // Set all joints to zero position
    std::vector<double> zero_positions(NUM_JOINTS, 0.0);
    std::vector<double> velocities(NUM_JOINTS, speed);

    bool success = set_positions(zero_positions, velocities);
    if (!success) {
        is_homing_ = false;
        debug_print("Failed to set zero positions");
        return false;
    }

    // Wait for motion to complete
    success = wait_for_motion_complete(zero_positions, 0.01, timeout);

    is_homing_ = false;

    if (success) {
        debug_print("Homing completed successfully");
    } else {
        debug_print("Homing failed or timeout");
    }

    return success;
}

// ========== 状态查询接口 ==========

std::vector<double> ArmComponent::get_temperatures() {
    std::vector<double> temperatures(NUM_JOINTS, 0.0);

    for (size_t i = 0; i < NUM_JOINTS; ++i) {
        auto motor = get_motor(JOINT_MOTOR_IDS[i]);
        if (motor) {
            temperatures[i] = motor->get_temperature();
        }
    }

    return temperatures;
}

bool ArmComponent::has_motor_errors() {
    for (int motor_id : JOINT_MOTOR_IDS) {
        auto motor = get_motor(motor_id);
        if (motor && motor->has_error()) {
            return true;
        }
    }
    return false;
}

std::vector<int> ArmComponent::get_error_motor_ids() {
    std::vector<int> error_ids;

    for (int motor_id : JOINT_MOTOR_IDS) {
        auto motor = get_motor(motor_id);
        if (motor && motor->has_error()) {
            error_ids.push_back(motor_id);
        }
    }

    return error_ids;
}

std::string ArmComponent::get_state_summary() const {
    std::ostringstream ss;
    ss << "ArmComponent State:\n";
    ss << "  Enabled: " << (enabled_ ? "Yes" : "No") << "\n";
    ss << "  Motors: " << get_motor_count() << "/6\n";
    ss << "  Homing: " << (is_homing_.load() ? "In Progress" : "Idle") << "\n";

    // Add joint states
    auto positions = const_cast<ArmComponent*>(this)->get_positions();
    auto velocities = const_cast<ArmComponent*>(this)->get_velocities();

    ss << "  Joint States:\n";
    for (size_t i = 0; i < NUM_JOINTS; ++i) {
        ss << "    Joint " << i << " (m" << JOINT_MOTOR_IDS[i] << "): "
           << std::fixed << std::setprecision(3) << positions[i] << " rad, "
           << velocities[i] << " rad/s\n";
    }

    return ss.str();
}

// ========== 辅助方法 ==========

bool ArmComponent::is_fully_enabled() const {
    for (int motor_id : JOINT_MOTOR_IDS) {
        auto motor = get_motor(motor_id);
        if (!motor || !motor->is_enabled()) {
            return false;
        }
    }
    return true;
}

bool ArmComponent::is_any_motor_moving() const {
    for (int motor_id : JOINT_MOTOR_IDS) {
        auto motor = get_motor(motor_id);
        if (motor && std::abs(motor->get_velocity()) > 0.01) {
            return true;
        }
    }
    return false;
}

std::map<std::string, std::vector<double>> ArmComponent::get_complete_state() {
    std::map<std::string, std::vector<double>> state;

    state["positions"] = get_positions();
    state["velocities"] = get_velocities();
    state["torques"] = get_torques();
    state["temperatures"] = get_temperatures();

    return state;
}

// ========== 诊断接口 ==========

void ArmComponent::print_arm_state() {
    auto state = get_complete_state();

    std::cout << "\n=== Arm Component State ===" << std::endl;
    std::cout << "Enabled: " << (enabled_ ? "Yes" : "No") << std::endl;
    std::cout << "Motors: " << get_motor_count() << "/6" << std::endl;
    std::cout << "Fully Enabled: " << (is_fully_enabled() ? "Yes" : "No") << std::endl;
    std::cout << "Any Motion: " << (is_any_motor_moving() ? "Yes" : "No") << std::endl;
    std::cout << "Motor Errors: " << (has_motor_errors() ? "Yes" : "No") << std::endl;

    if (has_motor_errors()) {
        auto error_ids = get_error_motor_ids();
        std::cout << "Error Motor IDs: ";
        for (int id : error_ids) {
            std::cout << id << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "\nJoint States:" << std::endl;
    std::cout << "Joint | Motor | Position (rad) | Velocity (rad/s) | Torque (Nm) | Temp (°C)" << std::endl;
    std::cout << "------|-------|----------------|------------------|-------------|----------" << std::endl;

    auto positions = state["positions"];
    auto velocities = state["velocities"];
    auto torques = state["torques"];
    auto temperatures = state["temperatures"];

    for (size_t i = 0; i < NUM_JOINTS; ++i) {
        std::cout << std::setw(5) << i << " |"
                  << std::setw(6) << "m" << JOINT_MOTOR_IDS[i] << " |"
                  << std::fixed << std::setw(15) << std::setprecision(3) << positions[i] << " |"
                  << std::fixed << std::setw(17) << std::setprecision(3) << velocities[i] << " |"
                  << std::fixed << std::setw(12) << std::setprecision(3) << torques[i] << " |"
                  << std::fixed << std::setw(9) << std::setprecision(1) << temperatures[i] << std::endl;
    }
    std::cout << "==========================\n" << std::endl;
}

bool ArmComponent::test_arm_functionality() {
    debug_print("Testing arm functionality");

    if (get_motor_count() != NUM_JOINTS) {
        debug_print("Arm does not have " + std::to_string(NUM_JOINTS) + " motors");
        return false;
    }

    // Test enable/disable
    debug_print("Testing enable/disable...");
    if (!enable_all()) {
        debug_print("Failed to enable all motors");
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    if (!disable_all()) {
        debug_print("Failed to disable all motors");
        return false;
    }

    debug_print("Enable/disable test passed");

    // Test position control
    debug_print("Testing position control...");
    if (!enable_all()) {
        debug_print("Failed to enable motors for position test");
        return false;
    }

    std::vector<double> test_positions(NUM_JOINTS, 0.1);  // Small test positions
    if (!set_positions(test_positions)) {
        debug_print("Failed to set test positions");
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    if (!set_positions(std::vector<double>(NUM_JOINTS, 0.0))) {
        debug_print("Failed to return to zero positions");
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    if (!disable_all()) {
        debug_print("Failed to disable motors");
        return false;
    }

    debug_print("Position control test passed");
    debug_print("Arm functionality test passed");
    return true;
}

// ========== Protected Helper Methods ==========

bool ArmComponent::is_valid_motor_id(int motor_id) const {
    return motor_id >= 1 && motor_id <= 6;
}

double ArmComponent::clamp_position(double position, int joint_index) const {
    if (!is_valid_joint_index(joint_index)) {
        return position;
    }
    return std::clamp(position, MIN_POSITIONS[joint_index], MAX_POSITIONS[joint_index]);
}

double ArmComponent::clamp_velocity(double velocity) const {
    return std::clamp(velocity, -max_velocity_, max_velocity_);
}

double ArmComponent::clamp_torque(double torque) const {
    return std::clamp(torque, -max_torque_, max_torque_);
}

// ========== Private Methods ==========

bool ArmComponent::is_valid_joint_index(int joint_index) const {
    return joint_index >= 0 && joint_index < static_cast<int>(NUM_JOINTS);
}

bool ArmComponent::is_valid_array_length(const std::vector<double>& array, size_t expected_length) const {
    return array.size() == expected_length;
}

int ArmComponent::joint_to_motor_id(int joint_index) const {
    if (!is_valid_joint_index(joint_index)) {
        return -1;
    }
    return JOINT_MOTOR_IDS[joint_index];
}

int ArmComponent::motor_to_joint_index(int motor_id) const {
    for (size_t i = 0; i < NUM_JOINTS; ++i) {
        if (JOINT_MOTOR_IDS[i] == motor_id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void ArmComponent::update_arm_state() {
    // This method can be used to update any internal state tracking
    // For now, it's mainly used to maintain consistency with other components
    if (debug_enabled_) {
        debug_print("Arm state updated");
    }
}

void ArmComponent::debug_print(const std::string& message) {
    if (debug_enabled_) {
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        std::cout << "[" << timestamp << "] [ARM-DM] " << message << std::endl;
    }
}

bool ArmComponent::wait_for_motion_complete(const std::vector<double>& target_positions,
                                           double tolerance, double timeout) {
    auto start_time = std::chrono::steady_clock::now();

    while (std::chrono::duration_cast<std::chrono::seconds>(
           std::chrono::steady_clock::now() - start_time).count() < timeout) {

        bool all_reached = true;
        auto current_positions = get_positions();

        for (size_t i = 0; i < NUM_JOINTS; ++i) {
            double error = std::abs(current_positions[i] - target_positions[i]);
            if (error > tolerance) {
                all_reached = false;
                break;
            }
        }

        if (all_reached) {
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return false;
}

} // namespace ic_can