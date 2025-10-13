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
#include <ic_can/motors/dm_motor_real.hpp>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <thread>
#include <iomanip>

namespace ic_can {

// 简单的日志函数
namespace {
    void debug_print(const std::string& message) {
        std::cout << "[DEBUG] ArmComponent: " << message << std::endl;
    }

    void error_print(const std::string& message) {
        std::cerr << "[ERROR] ArmComponent: " << message << std::endl;
    }
}

// 手臂电机ID静态定义
const std::vector<int> ArmComponent::ARM_MOTOR_IDS = {1, 2, 3, 4, 5, 6};

ArmComponent::ArmComponent(USB2CANAdapter& can_adapter)
    : can_adapter_(can_adapter) {
    debug_print("ArmComponent created");
}

bool ArmComponent::add_motor(std::shared_ptr<BaseMotor> motor) {
    if (!motor) {
        error_print("Cannot add null motor");
        return false;
    }

    int motor_id = motor->get_motor_id();
    if (motor_id < 1 || motor_id > 6) {
        error_print("Invalid arm motor ID: " + std::to_string(motor_id) + " (must be 1-6)");
        return false;
    }

    std::lock_guard<std::mutex> lock(motors_mutex_);
    motors_[motor_id] = motor;
    debug_print("Added arm motor " + std::to_string(motor_id));
    return true;
}

bool ArmComponent::remove_motor(int motor_id) {
    std::lock_guard<std::mutex> lock(motors_mutex_);
    auto it = motors_.find(motor_id);
    if (it != motors_.end()) {
        motors_.erase(it);
        debug_print("Removed arm motor " + std::to_string(motor_id));
        return true;
    }
    error_print("Motor not found: " + std::to_string(motor_id));
    return false;
}

std::shared_ptr<BaseMotor> ArmComponent::get_motor(int motor_id) {
    std::lock_guard<std::mutex> lock(motors_mutex_);
    auto it = motors_.find(motor_id);
    if (it != motors_.end()) {
        return it->second;
    }
    return nullptr;
}

std::map<int, std::shared_ptr<BaseMotor>> ArmComponent::get_all_motors() const {
    std::lock_guard<std::mutex> lock(motors_mutex_);
    return motors_;
}

size_t ArmComponent::get_motor_count() const {
    std::lock_guard<std::mutex> lock(motors_mutex_);
    return motors_.size();
}

bool ArmComponent::enable_all() {
    std::lock_guard<std::mutex> lock(motors_mutex_);
    bool success = true;

    for (auto& pair : motors_) {
        if (!pair.second->enable()) {
            error_print("Failed to enable motor " + std::to_string(pair.first));
            success = false;
        }
    }

    if (success) {
        debug_print("All arm motors enabled");
    }
    return success;
}

bool ArmComponent::disable_all() {
    std::lock_guard<std::mutex> lock(motors_mutex_);
    bool success = true;

    for (auto& pair : motors_) {
        if (!pair.second->disable()) {
            error_print("Failed to disable motor " + std::to_string(pair.first));
            success = false;
        }
    }

    if (success) {
        debug_print("All arm motors disabled");
    }
    return success;
}

bool ArmComponent::set_zero_all() {
    std::lock_guard<std::mutex> lock(motors_mutex_);
    bool success = true;

    for (auto& pair : motors_) {
        if (!pair.second->set_zero()) {
            error_print("Failed to set zero for motor " + std::to_string(pair.first));
            success = false;
        }
    }

    if (success) {
        debug_print("All arm motors zero set");
    }
    return success;
}

void ArmComponent::update_all_states() {
    std::lock_guard<std::mutex> lock(motors_mutex_);
    for (auto& pair : motors_) {
        pair.second->update_state();
    }
}

bool ArmComponent::send_all_commands() {
    std::lock_guard<std::mutex> lock(motors_mutex_);
    bool success = true;

    for (auto& pair : motors_) {
        auto command_data = pair.second->get_command_data();
        if (!command_data.empty()) {
            uint32_t can_id = pair.second->get_can_send_id();
            if (!can_adapter_.send_frame(can_id, command_data)) {
                error_print("Failed to send command to motor " + std::to_string(pair.first));
                success = false;
            }
        }
    }

    return success;
}

bool ArmComponent::set_positions(const std::vector<double>& positions,
                                const std::vector<double>& velocities,
                                const std::vector<double>& torques) {
    if (!is_valid_array_length(positions, 6)) {
        error_print("Invalid positions array length");
        return false;
    }

    if (!velocities.empty() && !is_valid_array_length(velocities, 6)) {
        error_print("Invalid velocities array length");
        return false;
    }

    if (!torques.empty() && !is_valid_array_length(torques, 6)) {
        error_print("Invalid torques array length");
        return false;
    }

    bool success = true;
    std::lock_guard<std::mutex> lock(motors_mutex_);

    for (int i = 0; i < 6; ++i) {
        int motor_id = i + 1;
        auto motor = motors_[motor_id];
        if (motor) {
            double pos = clamp_position(positions[i], i);
            double vel = velocities.empty() ? 0.0 : clamp_velocity(velocities[i], i);
            double tau = torques.empty() ? 0.0 : clamp_torque(torques[i], i);

            if (!motor->set_position(pos, vel, tau)) {
                error_print("Failed to set position for motor " + std::to_string(motor_id));
                success = false;
            }
        }
    }

    return success;
}

bool ArmComponent::set_single_position(int joint_index, double position,
                                       double velocity, double torque) {
    if (!is_valid_joint_index(joint_index)) {
        error_print("Invalid joint index: " + std::to_string(joint_index));
        return false;
    }

    int motor_id = joint_index + 1;
    auto motor = get_motor(motor_id);
    if (!motor) {
        error_print("Motor not found: " + std::to_string(motor_id));
        return false;
    }

    double pos = clamp_position(position, joint_index);
    double vel = clamp_velocity(velocity, joint_index);
    double tau = clamp_torque(torque, joint_index);

    return motor->set_position(pos, vel, tau);
}

std::vector<double> ArmComponent::get_positions() {
    std::vector<double> positions(6, 0.0);
    std::lock_guard<std::mutex> lock(motors_mutex_);

    for (int i = 0; i < 6; ++i) {
        int motor_id = i + 1;
        auto motor = motors_[motor_id];
        if (motor) {
            positions[i] = motor->get_position();
        }
    }

    return positions;
}

double ArmComponent::get_single_position(int joint_index) {
    if (!is_valid_joint_index(joint_index)) {
        error_print("Invalid joint index: " + std::to_string(joint_index));
        return 0.0;
    }

    int motor_id = joint_index + 1;
    auto motor = get_motor(motor_id);
    if (!motor) {
        error_print("Motor not found: " + std::to_string(motor_id));
        return 0.0;
    }

    return motor->get_position();
}

bool ArmComponent::set_velocities(const std::vector<double>& velocities,
                                 const std::vector<double>& torques) {
    if (!is_valid_array_length(velocities, 6)) {
        error_print("Invalid velocities array length");
        return false;
    }

    if (!torques.empty() && !is_valid_array_length(torques, 6)) {
        error_print("Invalid torques array length");
        return false;
    }

    bool success = true;
    std::lock_guard<std::mutex> lock(motors_mutex_);

    for (int i = 0; i < 6; ++i) {
        int motor_id = i + 1;
        auto motor = motors_[motor_id];
        if (motor) {
            double vel = clamp_velocity(velocities[i], i);
            double tau = torques.empty() ? 0.0 : clamp_torque(torques[i], i);

            if (!motor->set_velocity(vel, tau)) {
                error_print("Failed to set velocity for motor " + std::to_string(motor_id));
                success = false;
            }
        }
    }

    return success;
}

std::vector<double> ArmComponent::get_velocities() {
    std::vector<double> velocities(6, 0.0);
    std::lock_guard<std::mutex> lock(motors_mutex_);

    for (int i = 0; i < 6; ++i) {
        int motor_id = i + 1;
        auto motor = motors_[motor_id];
        if (motor) {
            velocities[i] = motor->get_velocity();
        }
    }

    return velocities;
}

bool ArmComponent::set_torques(const std::vector<double>& torques) {
    if (!is_valid_array_length(torques, 6)) {
        error_print("Invalid torques array length");
        return false;
    }

    bool success = true;
    std::lock_guard<std::mutex> lock(motors_mutex_);

    for (int i = 0; i < 6; ++i) {
        int motor_id = i + 1;
        auto motor = motors_[motor_id];
        if (motor) {
            double tau = clamp_torque(torques[i], i);

            if (!motor->set_torque(tau)) {
                error_print("Failed to set torque for motor " + std::to_string(motor_id));
                success = false;
            }
        }
    }

    return success;
}

std::vector<double> ArmComponent::get_torques() {
    std::vector<double> torques(6, 0.0);
    std::lock_guard<std::mutex> lock(motors_mutex_);

    for (int i = 0; i < 6; ++i) {
        int motor_id = i + 1;
        auto motor = motors_[motor_id];
        if (motor) {
            torques[i] = motor->get_torque();
        }
    }

    return torques;
}

bool ArmComponent::mit_control(const std::vector<double>& positions,
                               const std::vector<double>& velocities,
                               const std::vector<double>& torques,
                               const std::vector<double>& kp_array,
                               const std::vector<double>& kd_array) {
    if (!is_valid_array_length(positions, 6) ||
        !is_valid_array_length(velocities, 6) ||
        !is_valid_array_length(torques, 6) ||
        !is_valid_array_length(kp_array, 6) ||
        !is_valid_array_length(kd_array, 6)) {
        error_print("Invalid array length for MIT control");
        return false;
    }

    bool success = true;
    std::lock_guard<std::mutex> lock(motors_mutex_);

    for (int i = 0; i < 6; ++i) {
        int motor_id = i + 1;
        auto motor = motors_[motor_id];
        if (motor) {
            // 转换为达妙电机以使用MIT控制
            auto dm_motor = std::dynamic_pointer_cast<DMMotorReal>(motor);
            if (dm_motor) {
                double pos = clamp_position(positions[i], i);
                double vel = clamp_velocity(velocities[i], i);
                double tau = clamp_torque(torques[i], i);
                double kp = kp_array[i];
                double kd = kd_array[i];

                if (!dm_motor->mit_control(pos, vel, tau, kp, kd)) {
                    error_print("Failed to set MIT control for motor " + std::to_string(motor_id));
                    success = false;
                }
            } else {
                error_print("Motor " + std::to_string(motor_id) + " is not a DM motor");
                success = false;
            }
        }
    }

    return success;
}

bool ArmComponent::home_to_zero(double speed, double timeout) {
    debug_print("Starting homing to zero with speed " + std::to_string(speed) + " rad/s");

    // 获取当前位置
    auto current_positions = get_positions();
    if (current_positions.size() != 6) {
        error_print("Failed to get current positions");
        return false;
    }

    // 生成平滑轨迹到零位
    return execute_smooth_trajectory(current_positions,
                                     std::vector<double>(6, 0.0),
                                     timeout, 100.0);
}

bool ArmComponent::stop_all_motion() {
    debug_print("Stopping all arm motion");

    // 保持当前位置，设置速度为0
    auto current_positions = get_positions();
    return set_positions(current_positions, std::vector<double>(6, 0.0));
}

bool ArmComponent::check_position_limits(const std::vector<double>& positions) {
    if (!is_valid_array_length(positions, 6)) {
        return false;
    }

    for (int i = 0; i < 6; ++i) {
        int motor_id = i + 1;
        auto motor = get_motor(motor_id);
        if (motor) {
            auto limits = motor->get_limits();
            if (positions[i] < limits.min_position || positions[i] > limits.max_position) {
                error_print("Position limit violation for motor " + std::to_string(motor_id) +
                           ": " + std::to_string(positions[i]));
                return false;
            }
        }
    }

    return true;
}

bool ArmComponent::check_velocity_limits(const std::vector<double>& velocities) {
    if (!is_valid_array_length(velocities, 6)) {
        return false;
    }

    for (int i = 0; i < 6; ++i) {
        int motor_id = i + 1;
        auto motor = get_motor(motor_id);
        if (motor) {
            auto limits = motor->get_limits();
            if (std::abs(velocities[i]) > limits.max_velocity) {
                error_print("Velocity limit violation for motor " + std::to_string(motor_id) +
                           ": " + std::to_string(velocities[i]));
                return false;
            }
        }
    }

    return true;
}

bool ArmComponent::check_torque_limits(const std::vector<double>& torques) {
    if (!is_valid_array_length(torques, 6)) {
        return false;
    }

    for (int i = 0; i < 6; ++i) {
        int motor_id = i + 1;
        auto motor = get_motor(motor_id);
        if (motor) {
            auto limits = motor->get_limits();
            if (std::abs(torques[i]) > limits.max_torque) {
                error_print("Torque limit violation for motor " + std::to_string(motor_id) +
                           ": " + std::to_string(torques[i]));
                return false;
            }
        }
    }

    return true;
}

std::vector<double> ArmComponent::get_temperatures() {
    std::vector<double> temperatures(6, 0.0);
    std::lock_guard<std::mutex> lock(motors_mutex_);

    for (int i = 0; i < 6; ++i) {
        int motor_id = i + 1;
        auto motor = motors_[motor_id];
        if (motor) {
            temperatures[i] = motor->get_temperature();
        }
    }

    return temperatures;
}

bool ArmComponent::has_motor_errors() {
    std::lock_guard<std::mutex> lock(motors_mutex_);
    for (auto& pair : motors_) {
        if (pair.second->has_error()) {
            return true;
        }
    }
    return false;
}

std::vector<int> ArmComponent::get_error_motor_ids() {
    std::vector<int> error_ids;
    std::lock_guard<std::mutex> lock(motors_mutex_);

    for (auto& pair : motors_) {
        if (pair.second->has_error()) {
            error_ids.push_back(pair.first);
        }
    }

    return error_ids;
}

std::map<std::string, std::vector<double>> ArmComponent::get_complete_state() {
    std::map<std::string, std::vector<double>> state;

    state["positions"] = get_positions();
    state["velocities"] = get_velocities();
    state["torques"] = get_torques();
    state["temperatures"] = get_temperatures();

    return state;
}

bool ArmComponent::set_joint_gains(int joint_index, double kp, double kd) {
    if (!is_valid_joint_index(joint_index)) {
        error_print("Invalid joint index: " + std::to_string(joint_index));
        return false;
    }

    int motor_id = joint_index + 1;
    auto motor = get_motor(motor_id);
    if (!motor) {
        error_print("Motor not found: " + std::to_string(motor_id));
        return false;
    }

    // ⚠️ 仿真：增益设置在实际实现中需要保存到电机配置中
    debug_print("⚠️ SIMULATED: Set gains for motor " + std::to_string(motor_id) +
                ": kp=" + std::to_string(kp) + ", kd=" + std::to_string(kd));
    return true;
}

bool ArmComponent::set_all_gains(const std::vector<double>& kp_array,
                                const std::vector<double>& kd_array) {
    if (!is_valid_array_length(kp_array, 6) || !is_valid_array_length(kd_array, 6)) {
        error_print("Invalid gains array length");
        return false;
    }

    bool success = true;
    for (int i = 0; i < 6; ++i) {
        if (!set_joint_gains(i, kp_array[i], kd_array[i])) {
            success = false;
        }
    }

    return success;
}

void ArmComponent::print_arm_state() {
    auto state = get_complete_state();

    std::cout << "\n=== Arm Component State ===" << std::endl;
    std::cout << "Motor ID | Position (rad) | Velocity (rad/s) | Torque (Nm) | Temp (°C)" << std::endl;
    std::cout << "---------|----------------|------------------|-------------|----------" << std::endl;

    for (int i = 0; i < 6; ++i) {
        std::cout << "    " << (i+1) << "    | "
                  << std::fixed << std::setprecision(3) << std::setw(14) << state["positions"][i] << " | "
                  << std::fixed << std::setprecision(3) << std::setw(16) << state["velocities"][i] << " | "
                  << std::fixed << std::setprecision(3) << std::setw(11) << state["torques"][i] << " | "
                  << std::fixed << std::setprecision(1) << std::setw(8) << state["temperatures"][i] << std::endl;
    }
    std::cout << "==========================\n" << std::endl;
}

bool ArmComponent::test_arm_functionality() {
    debug_print("Testing arm functionality");

    if (get_motor_count() != 6) {
        error_print("Arm does not have 6 motors");
        return false;
    }

    // 测试使能/禁用
    debug_print("Testing enable/disable...");
    if (!enable_all()) {
        error_print("Failed to enable all motors");
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    if (!disable_all()) {
        error_print("Failed to disable all motors");
        return false;
    }

    debug_print("Enable/disable test passed");

    // 测试状态读取
    debug_print("Testing state reading...");
    auto positions = get_positions();
    auto velocities = get_velocities();
    auto torques = get_torques();

    if (positions.size() != 6 || velocities.size() != 6 || torques.size() != 6) {
        error_print("State reading failed");
        return false;
    }

    debug_print("State reading test passed");

    debug_print("Arm functionality test passed");
    return true;
}

// ========== 私有方法实现 ==========

bool ArmComponent::is_valid_joint_index(int joint_index) const {
    return joint_index >= 0 && joint_index < 6;
}

bool ArmComponent::is_valid_array_length(const std::vector<double>& array, size_t expected_length) const {
    return array.size() == expected_length;
}

double ArmComponent::clamp_position(double position, int joint_index) {
    int motor_id = joint_index + 1;
    auto motor = get_motor(motor_id);
    if (motor) {
        auto limits = motor->get_limits();
        if (position < limits.min_position) {
            return limits.min_position;
        }
        if (position > limits.max_position) {
            return limits.max_position;
        }
    }
    return position;
}

double ArmComponent::clamp_velocity(double velocity, int joint_index) {
    int motor_id = joint_index + 1;
    auto motor = get_motor(motor_id);
    if (motor) {
        auto limits = motor->get_limits();
        double max_vel = limits.max_velocity;
        if (velocity > max_vel) return max_vel;
        if (velocity < -max_vel) return -max_vel;
    }
    return velocity;
}

double ArmComponent::clamp_torque(double torque, int joint_index) {
    int motor_id = joint_index + 1;
    auto motor = get_motor(motor_id);
    if (motor) {
        auto limits = motor->get_limits();
        double max_tau = limits.max_torque;
        if (torque > max_tau) return max_tau;
        if (torque < -max_tau) return -max_tau;
    }
    return torque;
}

bool ArmComponent::execute_smooth_trajectory(const std::vector<double>& start_positions,
                                           const std::vector<double>& end_positions,
                                           double duration, double frequency) {
    if (start_positions.size() != 6 || end_positions.size() != 6) {
        error_print("Invalid trajectory positions array size");
        return false;
    }

    debug_print("Executing smooth trajectory over " + std::to_string(duration) + " seconds");

    int num_steps = static_cast<int>(duration * frequency);
    double dt = 1.0 / frequency;

    for (int step = 0; step <= num_steps; ++step) {
        double t = static_cast<double>(step) / num_steps;
        // 使用余弦插值实现平滑过渡
        double smooth_t = 0.5 * (1.0 - std::cos(M_PI * t));

        std::vector<double> current_positions(6);
        std::vector<double> current_velocities(6);
        std::vector<double> current_torques(6);

        for (int i = 0; i < 6; ++i) {
            current_positions[i] = start_positions[i] + smooth_t * (end_positions[i] - start_positions[i]);
            current_velocities[i] = 0.0; // 简化：速度设为0
            current_torques[i] = 0.0;   // 简化：力矩设为0
        }

        if (!set_positions(current_positions, current_velocities, current_torques)) {
            error_print("Failed to set trajectory positions at step " + std::to_string(step));
            return false;
        }

        // 等待下一步
        std::this_thread::sleep_for(std::chrono::duration<double>(dt));
    }

    debug_print("Smooth trajectory completed");
    return true;
}

bool ArmComponent::wait_for_motion_complete(const std::vector<double>& target_positions,
                                            double tolerance, double timeout) {
    if (target_positions.size() != 6) {
        error_print("Invalid target positions size");
        return false;
    }

    debug_print("Waiting for motion completion");

    auto start_time = std::chrono::steady_clock::now();

    while (std::chrono::duration_cast<std::chrono::seconds>(
           std::chrono::steady_clock::now() - start_time).count() < timeout) {

        auto current_positions = get_positions();
        bool all_reached = true;

        for (int i = 0; i < 6; ++i) {
            double error = std::abs(current_positions[i] - target_positions[i]);
            if (error > tolerance) {
                all_reached = false;
                break;
            }
        }

        if (all_reached) {
            debug_print("Motion completed successfully");
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    error_print("Motion completion timeout");
    return false;
}

} // namespace ic_can