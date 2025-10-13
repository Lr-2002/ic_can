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
#include <iostream>
#include <algorithm>
#include <cmath>
#include <thread>
#include <iomanip>

namespace ic_can {

// 简单的日志函数
namespace {
    void debug_print(const std::string& message) {
        std::cout << "[DEBUG] GripperComponent: " << message << std::endl;
    }

    void error_print(const std::string& message) {
        std::cerr << "[ERROR] GripperComponent: " << message << std::endl;
    }
}

// 夹爪电机ID静态定义
const std::vector<int> GripperComponent::GRIPPER_MOTOR_IDS = {7, 8, 9};

GripperComponent::GripperComponent(USB2CANAdapter& can_adapter)
    : can_adapter_(can_adapter) {
    debug_print("GripperComponent created");
}

bool GripperComponent::add_motor(std::shared_ptr<BaseMotor> motor) {
    if (!motor) {
        error_print("Cannot add null motor");
        return false;
    }

    int motor_id = motor->get_motor_id();
    if (motor_id < 7 || motor_id > 9) {
        error_print("Invalid gripper motor ID: " + std::to_string(motor_id) + " (must be 7-9)");
        return false;
    }

    std::lock_guard<std::mutex> lock(motors_mutex_);
    motors_[motor_id] = motor;
    debug_print("Added gripper motor " + std::to_string(motor_id));
    return true;
}

bool GripperComponent::remove_motor(int motor_id) {
    std::lock_guard<std::mutex> lock(motors_mutex_);
    auto it = motors_.find(motor_id);
    if (it != motors_.end()) {
        motors_.erase(it);
        debug_print("Removed gripper motor " + std::to_string(motor_id));
        return true;
    }
    error_print("Motor not found: " + std::to_string(motor_id));
    return false;
}

std::shared_ptr<BaseMotor> GripperComponent::get_motor(int motor_id) const {
    std::lock_guard<std::mutex> lock(motors_mutex_);
    auto it = motors_.find(motor_id);
    if (it != motors_.end()) {
        return it->second;
    }
    return nullptr;
}

std::map<int, std::shared_ptr<BaseMotor>> GripperComponent::get_all_motors() const {
    std::lock_guard<std::mutex> lock(motors_mutex_);
    return motors_;
}

size_t GripperComponent::get_motor_count() const {
    std::lock_guard<std::mutex> lock(motors_mutex_);
    return motors_.size();
}

bool GripperComponent::enable_all() {
    std::lock_guard<std::mutex> lock(motors_mutex_);
    bool success = true;

    for (auto& pair : motors_) {
        if (!pair.second->enable()) {
            error_print("Failed to enable motor " + std::to_string(pair.first));
            success = false;
        }
    }

    if (success) {
        debug_print("All gripper motors enabled");
    }
    return success;
}

bool GripperComponent::disable_all() {
    std::lock_guard<std::mutex> lock(motors_mutex_);
    bool success = true;

    for (auto& pair : motors_) {
        if (!pair.second->disable()) {
            error_print("Failed to disable motor " + std::to_string(pair.first));
            success = false;
        }
    }

    if (success) {
        debug_print("All gripper motors disabled");
    }
    return success;
}

bool GripperComponent::set_zero_all() {
    std::lock_guard<std::mutex> lock(motors_mutex_);
    bool success = true;

    for (auto& pair : motors_) {
        if (!pair.second->set_zero()) {
            error_print("Failed to set zero for motor " + std::to_string(pair.first));
            success = false;
        }
    }

    if (success) {
        debug_print("All gripper motors zero set");
    }
    return success;
}

void GripperComponent::update_all_states() {
    std::lock_guard<std::mutex> lock(motors_mutex_);
    for (auto& pair : motors_) {
        pair.second->update_state();
    }

    // 更新夹爪状态
    update_gripper_state();
}

bool GripperComponent::send_all_commands() {
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

bool GripperComponent::open(double speed, double force) {
    debug_print("Opening gripper: speed=" + std::to_string(speed) + ", force=" + std::to_string(force));

    speed = clamp_speed(speed);
    force = clamp_force(force);

    // 设置最大开合度
    return set_openness(max_openness_, speed, force);
}

bool GripperComponent::close(double speed, double force) {
    debug_print("Closing gripper: speed=" + std::to_string(speed) + ", force=" + std::to_string(force));

    speed = clamp_speed(speed);
    force = clamp_force(force);

    // 设置最小开合度
    return set_openness(min_openness_, speed, force);
}

bool GripperComponent::stop() {
    debug_print("Stopping gripper motion");

    // 保持当前位置
    double current_openness = get_openness();
    return set_openness(current_openness, 0.0, 0.0);
}

bool GripperComponent::set_openness(double openness, double speed, double force) {
    openness = clamp_openness(openness);
    speed = clamp_speed(speed);
    force = clamp_force(force);

    // 更新最后的命令
    last_command_.position = openness;
    last_command_.velocity = speed;
    last_command_.torque = force;

    // 将开合度转换为电机位置
    auto positions = openness_to_positions(openness);
    auto velocities = std::vector<double>(3, speed); // 简化：所有电机使用相同速度
    auto torques = std::vector<double>(3, force);   // 简化：所有电机使用相同力矩

    // ⚠️ 仿真：简化实现，实际需要根据电机类型分别处理
    debug_print("⚠️ SIMULATED: Setting openness to " + std::to_string(openness));

    return set_positions(positions, velocities);
}

double GripperComponent::get_openness() const {
    return current_openness_.load();
}

bool GripperComponent::set_force(double force) {
    force = clamp_force(force);
    max_force_ = force;

    // ⚠️ 仿真：力控制需要更复杂的实现
    debug_print("⚠️ SIMULATED: Setting max force to " + std::to_string(force));
    return true;
}

double GripperComponent::get_current_force() const {
    // ⚠️ 仿真：基于当前位置和目标位置估算力
    double current_pos = get_openness();
    double error = std::abs(current_pos - last_command_.position);
    return error * max_force_; // 简化的力估算
}

bool GripperComponent::force_control(double target_force) {
    target_force = clamp_force(target_force);

    debug_print("Force control: target force=" + std::to_string(target_force));

    // ⚠️ 仿真：简化力控制实现
    // 实际需要根据力反馈调整开合度
    double current_force = get_current_force();
    double error = target_force - current_force;

    // 简单的P控制
    double adjustment = error * 0.1; // 简化的增益
    double new_openness = clamp_openness(get_openness() + adjustment);

    return set_openness(new_openness, 0.5, target_force);
}

bool GripperComponent::set_positions(const std::vector<double>& positions,
                                     const std::vector<double>& velocities) {
    if (positions.size() != 3) {
        error_print("Invalid positions array size: expected 3, got " + std::to_string(positions.size()));
        return false;
    }

    if (!velocities.empty() && velocities.size() != 3) {
        error_print("Invalid velocities array size");
        return false;
    }

    bool success = true;
    std::lock_guard<std::mutex> lock(motors_mutex_);

    for (int i = 0; i < 3; ++i) {
        int motor_id = i + 7;
        auto motor = motors_[motor_id];
        if (motor) {
            double vel = velocities.empty() ? 0.0 : velocities[i];
            double tau = 0.0; // 简化：力矩设为0

            if (!motor->set_position(positions[i], vel, tau)) {
                error_print("Failed to set position for motor " + std::to_string(motor_id));
                success = false;
            }
        }
    }

    return success;
}

std::vector<double> GripperComponent::get_positions() {
    std::vector<double> positions(3, 0.0);
    std::lock_guard<std::mutex> lock(motors_mutex_);

    for (int i = 0; i < 3; ++i) {
        int motor_id = i + 7;
        auto motor = motors_[motor_id];
        if (motor) {
            positions[i] = motor->get_position();
        }
    }

    return positions;
}

bool GripperComponent::set_velocities(const std::vector<double>& velocities) {
    if (velocities.size() != 3) {
        error_print("Invalid velocities array size: expected 3, got " + std::to_string(velocities.size()));
        return false;
    }

    bool success = true;
    std::lock_guard<std::mutex> lock(motors_mutex_);

    for (int i = 0; i < 3; ++i) {
        int motor_id = i + 7;
        auto motor = motors_[motor_id];
        if (motor) {
            if (!motor->set_velocity(velocities[i], 0.0)) {
                error_print("Failed to set velocity for motor " + std::to_string(motor_id));
                success = false;
            }
        }
    }

    return success;
}

std::vector<double> GripperComponent::get_velocities() {
    std::vector<double> velocities(3, 0.0);
    std::lock_guard<std::mutex> lock(motors_mutex_);

    for (int i = 0; i < 3; ++i) {
        int motor_id = i + 7;
        auto motor = motors_[motor_id];
        if (motor) {
            velocities[i] = motor->get_velocity();
        }
    }

    return velocities;
}

bool GripperComponent::set_torques(const std::vector<double>& torques) {
    if (torques.size() != 3) {
        error_print("Invalid torques array size: expected 3, got " + std::to_string(torques.size()));
        return false;
    }

    bool success = true;
    std::lock_guard<std::mutex> lock(motors_mutex_);

    for (int i = 0; i < 3; ++i) {
        int motor_id = i + 7;
        auto motor = motors_[motor_id];
        if (motor) {
            if (!motor->set_torque(torques[i])) {
                error_print("Failed to set torque for motor " + std::to_string(motor_id));
                success = false;
            }
        }
    }

    return success;
}

std::vector<double> GripperComponent::get_torques() {
    std::vector<double> torques(3, 0.0);
    std::lock_guard<std::mutex> lock(motors_mutex_);

    for (int i = 0; i < 3; ++i) {
        int motor_id = i + 7;
        auto motor = motors_[motor_id];
        if (motor) {
            torques[i] = motor->get_torque();
        }
    }

    return torques;
}

bool GripperComponent::set_servo_angle(double angle, double speed) {
    debug_print("Setting servo angle: " + std::to_string(angle) + " degrees");

    // 将角度转换为弧度
    double angle_rad = angle * M_PI / 180.0;

    // ⚠️ 仿真：舵机简化实现
    int motor_id = 9; // 舵机通常是第9个电机
    auto motor = get_motor(motor_id);
    if (motor) {
        return motor->set_position(angle_rad, speed, 0.0);
    }

    error_print("Servo motor not found");
    return false;
}

double GripperComponent::get_servo_angle() const {
    int motor_id = 9;
    auto motor = get_motor(motor_id);
    if (motor) {
        double angle_rad = motor->get_position();
        return angle_rad * 180.0 / M_PI; // 转换为度
    }
    return 0.0;
}

bool GripperComponent::set_servo_pulse_width(uint16_t pulse_width_us) {
    debug_print("Setting servo pulse width: " + std::to_string(pulse_width_us) + " us");

    // ⚠️ 仿真：脉冲宽度转换为角度的简化映射
    // 假设：1000us = 0度，2000us = 180度
    double angle = static_cast<double>(pulse_width_us - 1000) / 1000.0 * 180.0;

    return set_servo_angle(angle, 0.5);
}

bool GripperComponent::is_fully_open() const {
    return get_openness() >= (max_openness_ - 0.01);
}

bool GripperComponent::is_fully_closed() const {
    return get_openness() <= (min_openness_ + 0.01);
}

bool GripperComponent::is_moving() const {
    return is_moving_.load();
}

bool GripperComponent::is_grasping() const {
    return is_grasping_.load();
}

std::vector<double> GripperComponent::get_temperatures() {
    std::vector<double> temperatures(3, 0.0);
    std::lock_guard<std::mutex> lock(motors_mutex_);

    for (int i = 0; i < 3; ++i) {
        int motor_id = i + 7;
        auto motor = motors_[motor_id];
        if (motor) {
            temperatures[i] = motor->get_temperature();
        }
    }

    return temperatures;
}

bool GripperComponent::has_motor_errors() {
    std::lock_guard<std::mutex> lock(motors_mutex_);
    for (auto& pair : motors_) {
        if (pair.second->has_error()) {
            return true;
        }
    }
    return false;
}

std::vector<int> GripperComponent::get_error_motor_ids() {
    std::vector<int> error_ids;
    std::lock_guard<std::mutex> lock(motors_mutex_);

    for (auto& pair : motors_) {
        if (pair.second->has_error()) {
            error_ids.push_back(pair.first);
        }
    }

    return error_ids;
}

bool GripperComponent::set_openness_limits(double min_openness, double max_openness) {
    if (min_openness < 0.0 || max_openness > 1.0 || min_openness >= max_openness) {
        error_print("Invalid openness limits");
        return false;
    }

    min_openness_ = min_openness;
    max_openness_ = max_openness;

    debug_print("Openness limits set: [" + std::to_string(min_openness) + ", " +
                std::to_string(max_openness) + "]");
    return true;
}

bool GripperComponent::set_max_force(double max_force) {
    if (max_force < 0.0 || max_force > 1.0) {
        error_print("Invalid max force: " + std::to_string(max_force));
        return false;
    }

    max_force_ = max_force;
    debug_print("Max force set to " + std::to_string(max_force));
    return true;
}

bool GripperComponent::set_max_speed(double max_speed) {
    if (max_speed < 0.0 || max_speed > 1.0) {
        error_print("Invalid max speed: " + std::to_string(max_speed));
        return false;
    }

    max_speed_ = max_speed;
    debug_print("Max speed set to " + std::to_string(max_speed));
    return true;
}

bool GripperComponent::perform_grasp_sequence(double target_openness, double approach_speed,
                                               double grasp_force, double timeout) {
    debug_print("Starting grasp sequence");

    // 1. 打开到目标开合度
    if (!set_openness(target_openness, approach_speed, 0.1)) {
        error_print("Failed to open gripper");
        return false;
    }

    // 2. 等待开合完成
    if (!wait_for_motion_complete(openness_to_positions(target_openness), 0.01, timeout/2)) {
        error_print("Failed to complete opening motion");
        return false;
    }

    // 3. 关闭夹爪
    if (!close(approach_speed, grasp_force)) {
        error_print("Failed to close gripper");
        return false;
    }

    // 4. 等待关闭完成
    if (!wait_for_motion_complete(openness_to_positions(min_openness_), 0.01, timeout/2)) {
        error_print("Failed to complete closing motion");
        return false;
    }

    // 5. 检测是否成功抓取
    is_grasping_ = detect_grasp_state();

    debug_print("Grasp sequence completed, grasping: " + std::to_string(is_grasping_.load()));
    return is_grasping_.load();
}

bool GripperComponent::release_object(double release_speed, double final_openness) {
    debug_print("Releasing object");

    // 打开夹爪
    if (!set_openness(final_openness, release_speed, 0.1)) {
        error_print("Failed to open gripper for release");
        return false;
    }

    is_grasping_ = false;
    debug_print("Object released");
    return true;
}

bool GripperComponent::auto_grasp(double max_force, double speed, double timeout) {
    debug_print("Starting auto grasp");

    // 1. 完全打开
    if (!open(speed, 0.1)) {
        error_print("Failed to open gripper");
        return false;
    }

    // 2. 开始关闭
    auto start_time = std::chrono::steady_clock::now();
    is_moving_ = true;

    while (std::chrono::duration_cast<std::chrono::seconds>(
           std::chrono::steady_clock::now() - start_time).count() < timeout) {

        // 逐渐关闭
        double current_openness = get_openness();
        double new_openness = current_openness - 0.01; // 每次减少1%

        if (new_openness < min_openness_) {
            new_openness = min_openness_;
        }

        set_openness(new_openness, speed, max_force);

        // 检测是否接触到物体
        if (detect_grasp_state()) {
            is_grasping_ = true;
            is_moving_ = false;
            debug_print("Auto grasp successful");
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    is_moving_ = false;
    error_print("Auto grasp timeout");
    return false;
}

std::map<std::string, std::vector<double>> GripperComponent::get_complete_state() {
    std::map<std::string, std::vector<double>> state;

    auto positions = get_positions();
    auto velocities = get_velocities();
    auto torques = get_torques();
    auto temperatures = get_temperatures();

    state["positions"] = positions;
    state["velocities"] = velocities;
    state["torques"] = torques;
    state["temperatures"] = temperatures;

    return state;
}

void GripperComponent::print_gripper_state() {
    auto state = get_complete_state();

    std::cout << "\n=== Gripper Component State ===" << std::endl;
    std::cout << "Openness: " << std::fixed << std::setprecision(3) << get_openness()
              << " (0=closed, 1=open)" << std::endl;
    std::cout << "Force: " << std::fixed << std::setprecision(3) << get_current_force() << std::endl;
    std::cout << "Moving: " << (is_moving() ? "Yes" : "No") << std::endl;
    std::cout << "Grasping: " << (is_grasping() ? "Yes" : "No") << std::endl;
    std::cout << "Servo angle: " << std::fixed << std::setprecision(1) << get_servo_angle() << "°" << std::endl;

    std::cout << "\nMotor States:" << std::endl;
    std::cout << "Motor ID | Position (rad) | Velocity (rad/s) | Torque (Nm) | Temp (°C)" << std::endl;
    std::cout << "---------|----------------|------------------|-------------|----------" << std::endl;

    auto positions = state["positions"];
    auto velocities = state["velocities"];
    auto torques = state["torques"];
    auto temperatures = state["temperatures"];

    for (int i = 0; i < 3; ++i) {
        std::cout << "    " << (i+7) << "    | "
                  << std::fixed << std::setprecision(3) << std::setw(14) << positions[i] << " | "
                  << std::fixed << std::setprecision(3) << std::setw(16) << velocities[i] << " | "
                  << std::fixed << std::setprecision(3) << std::setw(11) << torques[i] << " | "
                  << std::fixed << std::setprecision(1) << std::setw(8) << temperatures[i] << std::endl;
    }
    std::cout << "============================\n" << std::endl;
}

bool GripperComponent::test_gripper_functionality() {
    debug_print("Testing gripper functionality");

    if (get_motor_count() != 3) {
        error_print("Gripper does not have 3 motors");
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

    // 测试开合
    debug_print("Testing open/close...");
    if (!enable_all()) {
        error_print("Failed to enable motors for test");
        return false;
    }

    if (!open()) {
        error_print("Failed to open gripper");
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    if (!close()) {
        error_print("Failed to close gripper");
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    if (!disable_all()) {
        error_print("Failed to disable motors");
        return false;
    }

    debug_print("Open/close test passed");

    debug_print("Gripper functionality test passed");
    return true;
}

bool GripperComponent::calibrate_gripper() {
    debug_print("Starting gripper calibration");

    // ⚠️ 仿真：简化校准流程
    // 实际校准需要更复杂的步骤

    // 1. 使能电机
    if (!enable_all()) {
        error_print("Failed to enable motors for calibration");
        return false;
    }

    // 2. 设置零位
    if (!set_zero_all()) {
        error_print("Failed to set zero positions");
        return false;
    }

    // 3. 测试开合范围
    if (!open()) {
        error_print("Failed to open during calibration");
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    if (!close()) {
        error_print("Failed to close during calibration");
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // 4. 回到中间位置
    if (!set_openness(0.5, 0.5, 0.3)) {
        error_print("Failed to set middle position");
        return false;
    }

    debug_print("Gripper calibration completed");
    return true;
}

// ========== 私有方法实现 ==========

std::vector<double> GripperComponent::openness_to_positions(double openness) {
    // ⚠️ 仿真：简化的开合度到位置映射
    // 实际实现需要根据机械结构计算

    std::vector<double> positions(3);

    // 简化映射：开合度0-1映射到位置范围-π/4到π/4
    for (int i = 0; i < 3; ++i) {
        positions[i] = (openness - 0.5) * M_PI_2; // (openness-0.5) * π/2
    }

    return positions;
}

double GripperComponent::positions_to_openness(const std::vector<double>& positions) {
    if (positions.size() != 3) {
        return 0.5; // 默认中间位置
    }

    // ⚠️ 仿真：简化的位置到开合度映射
    // 使用第一个电机的位置作为参考
    double pos = positions[0];
    double openness = (pos / M_PI_2) + 0.5; // pos/(π/2) + 0.5
    return clamp_openness(openness);
}

double GripperComponent::clamp_openness(double openness) {
    if (openness < min_openness_) return min_openness_;
    if (openness > max_openness_) return max_openness_;
    return openness;
}

double GripperComponent::clamp_force(double force) {
    if (force < 0.0) return 0.0;
    if (force > max_force_) return max_force_;
    return force;
}

double GripperComponent::clamp_speed(double speed) {
    if (speed < 0.0) return 0.0;
    if (speed > max_speed_) return max_speed_;
    return speed;
}

void GripperComponent::update_gripper_state() {
    // ⚠️ 仿真：更新夹爪状态
    // 实际实现需要基于真实的传感器数据

    auto current_positions = get_positions();
    double current_openness = positions_to_openness(current_positions);

    // 更新开合度
    current_openness_.store(current_openness);

    // 更新运动状态
    double openness_change = std::abs(current_openness - last_command_.position);
    is_moving_ = (openness_change > 0.01);
}

bool GripperComponent::detect_grasp_state() {
    // ⚠️ 仿真：简化的抓取检测
    // 实际实现需要基于力传感器或电流检测

    auto torques = get_torques();
    double total_torque = 0.0;
    for (double tau : torques) {
        total_torque += std::abs(tau);
    }

    // 如果总力矩超过阈值，认为正在抓取物体
    return total_torque > 1.0; // 简化的阈值
}

bool GripperComponent::wait_for_motion_complete(const std::vector<double>& target_positions,
                                                double tolerance, double timeout) {
    auto start_time = std::chrono::steady_clock::now();

    while (std::chrono::duration_cast<std::chrono::seconds>(
           std::chrono::steady_clock::now() - start_time).count() < timeout) {

        auto current_positions = get_positions();
        bool all_reached = true;

        for (size_t i = 0; i < current_positions.size(); ++i) {
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