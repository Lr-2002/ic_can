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

#include "ic_can/core/ic_can.hpp"
#include <ic_can/motors/dm_motor.hpp>
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>

namespace ic_can {

IC_CAN::IC_CAN(const std::string& device_sn, bool enable_debug)
    : device_sn_(device_sn), debug_enabled_(enable_debug),
      initialized_(false), emergency_stop_flag_(false),
      control_frequency_(500.0), control_thread_running_(false) {

    // 初始化 CAN 适配器
    can_adapter_ = std::make_unique<USB2CANAdapter>(device_sn_, debug_enabled_);

    // 初始化组件
    arm_ = std::make_unique<ArmComponent>(*can_adapter_);
    gripper_ = std::make_unique<GripperComponent>(*can_adapter_);
    safety_ = std::make_unique<SafetyModule>(9); // 9个电机

    // 初始化时间戳
    last_control_time_ = std::chrono::steady_clock::now();

    debug_print("IC_CAN controller created");
}

IC_CAN::~IC_CAN() {
    if (control_thread_running_) {
        stop_high_frequency_control();
    }

    if (initialized_) {
        disable_all();
    }

    debug_print("IC_CAN controller destroyed");
}

bool IC_CAN::initialize() {
    if (initialized_) {
        debug_print("Already initialized");
        return true;
    }

    debug_print("Initializing IC_CAN system");

    // 1. 连接 USB2CAN 适配器
    if (!can_adapter_->connect()) {
        error_print("Failed to connect USB2CAN adapter");
        return false;
    }

    // 2. 初始化电机配置
    initialize_motor_configs();

    // 3. 创建电机实例
    create_motors();

    // 4. 设置 CAN 帧回调
    can_adapter_->set_frame_callback([this](const CANFrame& frame) {
        handle_can_frame(frame.can_id, frame.data);
    });

    // 5. 启动异步接收
    if (!can_adapter_->start_async_receive()) {
        error_print("Failed to start async receive");
        can_adapter_->disconnect();
        return false;
    }

    // 6. 设置安全模块回调
    safety_->set_emergency_stop_callback([this](const std::string& reason) {
        debug_print("Emergency stop triggered: " + reason);
        emergency_stop_flag_ = true;
    });

    safety_->set_violation_callback([this](const SafetyViolation& violation) {
        debug_print("Safety violation detected: " + violation.description);
    });

    // 7. 启动安全监控
    if (!safety_->start_monitoring()) {
        error_print("Failed to start safety monitoring");
        // 不返回失败，继续初始化
    }

    initialized_ = true;
    info_print("IC_CAN system initialized successfully");

    // 打印系统信息
    print_system_info();

    return true;
}

bool IC_CAN::enable_all() {
    if (!initialized_) {
        error_print("System not initialized");
        return false;
    }

    if (emergency_stop_flag_) {
        error_print("Cannot enable motors: emergency stop active");
        return false;
    }

    debug_print("Enabling all motors");

    bool success = true;

    // 使能手臂电机
    if (!arm_->enable_all()) {
        error_print("Failed to enable arm motors");
        success = false;
    }

    // 使能夹爪电机
    if (!gripper_->enable_all()) {
        error_print("Failed to enable gripper motors");
        success = false;
    }

    if (success) {
        info_print("All motors enabled successfully");
        // 等待电机稳定
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } else {
        error_print("Failed to enable some motors");
    }

    return success;
}

bool IC_CAN::disable_all() {
    if (!initialized_) {
        debug_print("System not initialized");
        return true;
    }

    debug_print("Disabling all motors");

    bool success = true;

    // 禁用手臂电机
    if (!arm_->disable_all()) {
        error_print("Failed to disable arm motors");
        success = false;
    }

    // 禁用夹爪电机
    if (!gripper_->disable_all()) {
        error_print("Failed to disable gripper motors");
        success = false;
    }

    if (success) {
        info_print("All motors disabled successfully");
    } else {
        error_print("Failed to disable some motors");
    }

    return success;
}

bool IC_CAN::emergency_stop() {
    debug_print("EMERGENCY STOP triggered!");

    // 设置紧急停止标志
    emergency_stop_flag_ = true;

    // 触发安全模块紧急停止
    safety_->trigger_emergency_stop("Software emergency stop");

    // 立即停止所有电机
    return disable_all();
}

bool IC_CAN::reset_emergency_stop() {
    debug_print("Resetting emergency stop");

    // 重置紧急停止标志
    emergency_stop_flag_ = false;

    // 重置安全模块
    return safety_->reset_emergency_stop();
}

void IC_CAN::update_all_states() {
    if (!initialized_) {
        debug_print("System not initialized");
        return;
    }

    if (emergency_stop_flag_) {
        debug_print("Emergency stop active, skipping state update");
        return;
    }

    // 更新手臂状态
    arm_->update_all_states();

    // 更新夹爪状态
    gripper_->update_all_states();

    // 喂安全看门狗
    safety_->feed_watchdog();
}

void IC_CAN::refresh_all() {
    update_all_states();

    // 检查安全状态
    if (safety_->has_active_violations()) {
        auto violations = safety_->get_active_violations();
        for (const auto& violation : violations) {
            debug_print("Active safety violation: " + violation.description);
        }
    }
}

std::vector<double> IC_CAN::get_joint_positions() {
    if (!initialized_) {
        error_print("System not initialized");
        return {};
    }

    std::vector<double> positions;

    // 获取手臂位置 (6个关节)
    auto arm_positions = arm_->get_positions();
    positions.insert(positions.end(), arm_positions.begin(), arm_positions.end());

    // 获取夹爪位置 (3个关节)
    auto gripper_positions = gripper_->get_positions();
    positions.insert(positions.end(), gripper_positions.begin(), gripper_positions.end());

    return positions;
}

std::vector<double> IC_CAN::get_joint_velocities() {
    if (!initialized_) {
        error_print("System not initialized");
        return {};
    }

    std::vector<double> velocities;

    // 获取手臂速度
    auto arm_velocities = arm_->get_velocities();
    velocities.insert(velocities.end(), arm_velocities.begin(), arm_velocities.end());

    // 获取夹爪速度
    auto gripper_velocities = gripper_->get_velocities();
    velocities.insert(velocities.end(), gripper_velocities.begin(), gripper_velocities.end());

    return velocities;
}

std::vector<double> IC_CAN::get_joint_torques() {
    if (!initialized_) {
        error_print("System not initialized");
        return {};
    }

    std::vector<double> torques;

    // 获取手臂力矩
    auto arm_torques = arm_->get_torques();
    torques.insert(torques.end(), arm_torques.begin(), arm_torques.end());

    // 获取夹爪力矩
    auto gripper_torques = gripper_->get_torques();
    torques.insert(torques.end(), gripper_torques.begin(), gripper_torques.end());

    return torques;
}

bool IC_CAN::set_joint_positions(const std::vector<double>& positions,
                                const std::vector<double>& velocities,
                                const std::vector<double>& torques) {
    if (!initialized_) {
        error_print("System not initialized");
        return false;
    }

    if (emergency_stop_flag_) {
        error_print("Cannot set positions: emergency stop active");
        return false;
    }

    if (positions.size() != 9) {
        error_print("Invalid positions size: expected 9, got " + std::to_string(positions.size()));
        return false;
    }

    // 安全检查
    for (size_t i = 0; i < positions.size(); ++i) {
        int motor_id = i + 1;
        if (!safety_->check_position_limits(motor_id, positions[i])) {
            error_print("Position limit violation for motor " + std::to_string(motor_id) +
                       ": " + std::to_string(positions[i]));
            return false;
        }
    }

    bool success = true;

    // 设置手臂位置 (前6个关节)
    std::vector<double> arm_pos(positions.begin(), positions.begin() + 6);
    std::vector<double> arm_vel, arm_tau;
    if (velocities.size() >= 6) {
        arm_vel.assign(velocities.begin(), velocities.begin() + 6);
    }
    if (torques.size() >= 6) {
        arm_tau.assign(torques.begin(), torques.begin() + 6);
    }

    if (!arm_->set_positions(arm_pos, arm_vel, arm_tau)) {
        error_print("Failed to set arm positions");
        success = false;
    }

    // 设置夹爪位置 (后3个关节)
    std::vector<double> gripper_pos(positions.begin() + 6, positions.end());
    std::vector<double> gripper_vel, gripper_tau;
    if (velocities.size() >= 9) {
        gripper_vel.assign(velocities.begin() + 6, velocities.end());
    }
    if (torques.size() >= 9) {
        gripper_tau.assign(torques.begin() + 6, torques.end());
    }

    if (!gripper_->set_positions(gripper_pos, gripper_vel)) {
        error_print("Failed to set gripper positions");
        success = false;
    }

    return success;
}

bool IC_CAN::set_joint_torques(const std::vector<double>& torques,
                               const std::vector<double>& positions,
                               const std::vector<double>& velocities) {
    if (!initialized_) {
        error_print("System not initialized");
        return false;
    }

    if (emergency_stop_flag_) {
        error_print("Cannot set torques: emergency stop active");
        return false;
    }

    if (torques.size() != 9) {
        error_print("Invalid torques size: expected 9, got " + std::to_string(torques.size()));
        return false;
    }

    // 安全检查
    for (size_t i = 0; i < torques.size(); ++i) {
        int motor_id = i + 1;
        if (!safety_->check_torque_limits(motor_id, torques[i])) {
            error_print("Torque limit violation for motor " + std::to_string(motor_id) +
                       ": " + std::to_string(torques[i]));
            return false;
        }
    }

    bool success = true;

    // 设置手臂力矩
    std::vector<double> arm_tau(torques.begin(), torques.begin() + 6);
    if (!arm_->set_torques(arm_tau)) {
        error_print("Failed to set arm torques");
        success = false;
    }

    // 设置夹爪力矩
    std::vector<double> gripper_tau(torques.begin() + 6, torques.end());
    if (!gripper_->set_torques(gripper_tau)) {
        error_print("Failed to set gripper torques");
        success = false;
    }

    return success;
}

bool IC_CAN::start_high_frequency_control() {
    if (control_thread_running_) {
        debug_print("High frequency control already running");
        return true;
    }

    if (!initialized_) {
        error_print("System not initialized");
        return false;
    }

    debug_print("Starting high frequency control at " + std::to_string(control_frequency_) + " Hz");

    try {
        stop_control_thread_ = false;
        control_thread_ = std::thread(&IC_CAN::control_thread_main, this);
        control_thread_running_ = true;

        info_print("High frequency control started successfully");
        return true;

    } catch (const std::exception& e) {
        error_print("Failed to start high frequency control: " + std::string(e.what()));
        return false;
    }
}

bool IC_CAN::stop_high_frequency_control() {
    if (!control_thread_running_) {
        debug_print("High frequency control not running");
        return true;
    }

    debug_print("Stopping high frequency control");

    stop_control_thread_ = true;

    if (control_thread_.joinable()) {
        control_thread_.join();
    }

    control_thread_running_ = false;
    info_print("High frequency control stopped");
    return true;
}

void IC_CAN::set_control_frequency(double frequency_hz) {
    if (frequency_hz <= 0 || frequency_hz > 1000) {
        error_print("Invalid control frequency: " + std::to_string(frequency_hz));
        return;
    }

    control_frequency_ = frequency_hz;
    debug_print("Control frequency set to " + std::to_string(frequency_hz) + " Hz");
}

std::map<std::string, std::string> IC_CAN::get_system_status() {
    std::map<std::string, std::string> status;

    status["initialized"] = initialized_ ? "true" : "false";
    status["emergency_stop"] = emergency_stop_flag_ ? "true" : "false";
    status["control_frequency"] = std::to_string(control_frequency_);
    status["hf_control_running"] = control_thread_running_ ? "true" : "false";

    // 获取设备信息
    auto device_info = can_adapter_->get_device_info();
    status.insert(device_info.begin(), device_info.end());

    // 获取安全状态
    auto safety_status = safety_->get_safety_status_summary();
    status.insert(safety_status.begin(), safety_status.end());

    return status;
}

void IC_CAN::print_system_info() {
    auto status = get_system_status();

    std::cout << "\n=== IC_CAN System Status ===" << std::endl;
    for (const auto& pair : status) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
    std::cout << "==========================\n" << std::endl;
}

void IC_CAN::send_commands_all() {
    if (!initialized_) {
        return;
    }

    // 发送手臂命令
    arm_->send_all_commands();

    // 发送夹爪命令
    gripper_->send_all_commands();

    // 通过 USB2CAN 适配器批量发送
    // 注意：实际实现中，各组件会将命令添加到适配器的发送队列
}

void IC_CAN::receive_data_all(int timeout_us) {
    if (!initialized_) {
        return;
    }

    // 通过适配器接收数据
    std::vector<CANFrame> frames;
    size_t received = can_adapter_->receive_frames_batch(frames, 100, timeout_us);

    if (received > 0 && debug_enabled_) {
        debug_print("Received " + std::to_string(received) + " CAN frames");
    }
}

// ========== 私有方法实现 ==========

void IC_CAN::initialize_motor_configs() {
    debug_print("Initializing motor configurations");

    // 手臂电机配置 (m1-m6)
    motor_configs_.emplace_back(1, MotorType::DM_DAMIAO, 0x01, 0x11, 250.0, 5.0, 15.0, 15.0, -52.36, 52.36); // DM10010L
    motor_configs_.emplace_back(2, MotorType::DM_DAMIAO, 0x02, 0x12, 120.0, 2.0, 8.0, 10.0, -10.47, 10.47);   // DM6248
    motor_configs_.emplace_back(3, MotorType::DM_DAMIAO, 0x03, 0x13, 120.0, 2.0, 8.0, 10.0, -10.47, 10.47);   // DM6248
    motor_configs_.emplace_back(4, MotorType::DM_DAMIAO, 0x04, 0x14, 40.0, 1.0, 4.0, 8.0, -6.28, 6.28);      // DM4340
    motor_configs_.emplace_back(5, MotorType::DM_DAMIAO, 0x05, 0x15, 40.0, 1.0, 4.0, 8.0, -6.28, 6.28);      // DM4340
    motor_configs_.emplace_back(6, MotorType::DM_DAMIAO, 0x06, 0x16, 30.0, 1.0, 3.0, 5.0, -3.14, 3.14);      // DM4310

    // 夹爪电机配置 (m7-m9)
    motor_configs_.emplace_back(7, MotorType::HT_HIGH_TORQUE, 0x07, 0x17, 8.0, 1.2, 20.0, 2.0, -6.28, 6.28);   // HT4438
    motor_configs_.emplace_back(8, MotorType::HT_HIGH_TORQUE, 0x08, 0x18, 8.0, 1.2, 20.0, 2.0, -6.28, 6.28);   // HT4438
    motor_configs_.emplace_back(9, MotorType::SERVO, 0x09, 0x19, 0.0, 0.0, 5.0, 10.0, -3.14, 3.14);        // Servo

    debug_print("Created " + std::to_string(motor_configs_.size()) + " motor configurations");
}

void IC_CAN::create_motors() {
    debug_print("Creating motor instances");

    std::lock_guard<std::mutex> lock(motors_mutex_);
    motors_.clear();

    for (const auto& config : motor_configs_) {
        std::shared_ptr<BaseMotor> motor;

        switch (config.type) {
            case MotorType::DM_DAMIAO:
                motor = std::make_shared<DMMotor>(config.motor_id,
                                                 DMMotorType::DM10010L, // ⚠️ 简化，实际需要根据具体型号
                                                 config.can_send_id,
                                                 config.can_recv_id);
                break;

            case MotorType::HT_HIGH_TORQUE:
                // ⚠️ 仿真：HT电机暂时用达妙电机代替
                motor = std::make_shared<DMMotor>(config.motor_id,
                                                 DMMotorType::DM4310,
                                                 config.can_send_id,
                                                 config.can_recv_id);
                break;

            case MotorType::SERVO:
                // ⚠️ 仿真：舵机暂时用达妙电机代替
                motor = std::make_shared<DMMotor>(config.motor_id,
                                                 DMMotorType::DM4310,
                                                 config.can_send_id,
                                                 config.can_recv_id);
                break;
        }

        if (motor) {
            // 设置电机限制
            MotorLimits limits;
            limits.max_position = config.position_max;
            limits.min_position = config.position_min;
            limits.max_velocity = config.max_velocity;
            limits.max_torque = config.max_torque;
            motor->set_limits(limits);

            motors_[config.motor_id] = motor;

            // 添加到对应组件
            if (config.motor_id <= 6) {
                arm_->add_motor(motor);
            } else {
                gripper_->add_motor(motor);
            }

            debug_print("Created motor " + std::to_string(config.motor_id) +
                        " (type: " + std::to_string(static_cast<int>(config.type)) + ")");
        }
    }

    debug_print("Created " + std::to_string(motors_.size()) + " motor instances");
}

void IC_CAN::control_thread_main() {
    debug_print("High frequency control thread started");

    auto target_duration = std::chrono::duration<double>(1.0 / control_frequency_);

    while (!stop_control_thread_) {
        auto loop_start = std::chrono::high_resolution_clock::now();

        try {
            // 1. 检查紧急停止
            if (emergency_stop_flag_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            // 2. 执行控制循环
            execute_control_loop();

            // 3. 精确延时控制
            auto loop_end = std::chrono::high_resolution_clock::now();
            auto elapsed = loop_end - loop_start;

            if (elapsed < target_duration) {
                auto sleep_time = target_duration - elapsed;
                std::this_thread::sleep_for(sleep_time);
            } else {
                // 警告：控制循环超时
                auto overtime_us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed - target_duration);
                debug_print("Control loop overtime: " + std::to_string(overtime_us.count()) + " us");
            }

            last_control_time_ = loop_end;

        } catch (const std::exception& e) {
            error_print("Control thread error: " + std::string(e.what()));
            increment_error_count();
        }
    }

    debug_print("High frequency control thread stopped");
}

void IC_CAN::execute_control_loop() {
    // 1. 更新所有电机状态
    update_all_states();

    // 2. 发送所有命令
    send_commands_all();

    // 3. 接收数据
    receive_data_all(500); // 500us 超时

    // 4. 安全检查
    if (safety_->has_active_violations()) {
        // ⚠️ 仿真：简化安全处理
        auto violations = safety_->get_active_violations();
        for (const auto& violation : violations) {
            if (violation.type == SafetyViolationType::EMERGENCY_STOP) {
                emergency_stop_flag_ = true;
                break;
            }
        }
    }
}

void IC_CAN::handle_can_frame(uint32_t can_id, const std::vector<uint8_t>& data) {
    // 查找对应的电机
    std::lock_guard<std::mutex> lock(motors_mutex_);
    for (auto& pair : motors_) {
        auto motor = pair.second;
        if (motor->get_can_recv_id() == can_id) {
            if (motor->process_response(data)) {
                // 成功处理响应
            } else {
                debug_print("Failed to process response for motor " + std::to_string(pair.first));
            }
            break;
        }
    }
}

void IC_CAN::debug_print(const std::string& message) {
    if (debug_enabled_) {
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        std::cout << "[" << timestamp << "] [IC_CAN-DEBUG] " << message << std::endl;
    }
}

void IC_CAN::error_print(const std::string& message) {
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    std::cerr << "[" << timestamp << "] [IC_CAN-ERROR] " << message << std::endl;
}

void IC_CAN::info_print(const std::string& message) {
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    std::cout << "[" << timestamp << "] [IC_CAN-INFO] " << message << std::endl;
}

void IC_CAN::increment_error_count() {
    // 可以添加到统计信息中
}

} // namespace ic_can