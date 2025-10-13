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
#include <ic_can/motors/dm_motor_real.hpp>
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <sched.h>
#include <sys/mman.h>

namespace ic_can {

IC_CAN::IC_CAN(const std::string& device_sn, bool enable_debug)
    : device_sn_(device_sn), debug_enabled_(enable_debug),
      initialized_(false), emergency_stop_flag_(false),
      control_frequency_(500.0), control_thread_running_(false),
      control_period_us_(2000) { // 500Hz = 2000微秒周期 {

    // 初始化 CAN 适配器
    can_adapter_ = std::make_unique<USB2CANAdapter>(device_sn_, debug_enabled_);

    // 初始化组件
    arm_ = std::make_unique<ArmComponent>(*can_adapter_);
    gripper_ = std::make_unique<GripperComponent>(*can_adapter_);
    safety_ = std::make_unique<SafetyModule>(9); // 9个电机

    // 初始化时间戳
    last_control_time_ = std::chrono::steady_clock::now();

    debug_print("IC_CAN controller created with 500Hz control target");
}

IC_CAN::~IC_CAN() {
    if (control_thread_running_) {
        stop_high_frequency_control();
    }
}

bool IC_CAN::initialize() {
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

bool IC_CAN::start_high_frequency_control() {
    if (control_thread_running_) {
        debug_print("High-frequency control already running");
        return true;
    }

    if (!initialized_) {
        error_print("Cannot start high-frequency control: system not initialized");
        return false;
    }

    try {
        stop_control_thread_ = false;

        // 创建高优先级控制线程
        control_thread_ = std::thread(&IC_CAN::control_thread_main, this);

        // 设置线程优先级 (需要root权限)
        struct sched_param param;
        param.sched_priority = 99; // 最高优先级
        if (pthread_setschedparam(control_thread_.native_handle(), SCHED_FIFO, &param) != 0) {
            warn_print("Failed to set real-time priority, using normal priority");
        }

        control_thread_running_ = true;
        info_print("500Hz high-frequency control started");
        return true;

    } catch (const std::exception& e) {
        error_print("Failed to start high-frequency control: " + std::string(e.what()));
        return false;
    }
}

bool IC_CAN::stop_high_frequency_control() {
    if (!control_thread_running_) {
        debug_print("High-frequency control not running");
        return true;
    }

    debug_print("Stopping high-frequency control");

    stop_control_thread_ = true;

    if (control_thread_.joinable()) {
        control_thread_.join();
    }

    control_thread_running_ = false;
    info_print("High-frequency control stopped");
    return true;
}

// ========== 私有方法实现 ==========

void IC_CAN::control_thread_main() {
    debug_print("500Hz control thread started");

    // 锁定内存页，避免页面交换
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        warn_print("Failed to lock memory pages");
    }

    auto control_start_time = std::chrono::steady_clock::now();
    uint64_t loop_count = 0;

    while (!stop_control_thread_) {
        auto loop_start = std::chrono::high_resolution_clock::now();

        // 检查紧急停止
        if (emergency_stop_flag_) {
            emergency_stop();
            continue;
        }

        // ========== 500Hz 控制循环 ==========

        // 1. 批量发送所有电机控制指令 (约0.2ms)
        send_commands_all();

        // 2. 批量接收所有电机状态 (约0.3ms)
        receive_data_all(500); // 500us超时

        // 3. 更新所有电机状态 (约0.1ms)
        update_all_states();

        // 4. 执行安全检查 (约0.1ms)
        safety_->feed_watchdog();

        // 5. 执行控制算法 (约0.8ms)
        execute_control_loop();

        // ========== 时间控制 ==========
        auto loop_end = std::chrono::high_resolution_clock::now();
        auto loop_duration = std::chrono::duration_cast<std::chrono::microseconds>(loop_end - loop_start);

        // 计算剩余时间并等待
        int period_us = static_cast<int>(1000000.0 / control_frequency_);
        if (loop_duration < std::chrono::microseconds(period_us)) {
            auto sleep_time = std::chrono::microseconds(period_us) - loop_duration;
            std::this_thread::sleep_for(sleep_time);
        } else {
            // 警告：控制循环超时
            if (loop_count % 100 == 0) { // 每100次循环打印一次警告
                warn_print("Control loop timeout: " + std::to_string(loop_duration.count()) + "us");
            }
        }

        loop_count++;

        // 性能统计
        if (loop_count % 1000 == 0) { // 每1000次循环统计一次
            auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - control_start_time);
            double actual_freq = 1000.0 * loop_count / total_time.count();

            if (std::abs(actual_freq - control_frequency_) > 1.0) {
                warn_print("Control frequency deviation: actual=" + std::to_string(actual_freq) +
                          "Hz, target=" + std::to_string(control_frequency_) + "Hz");
            }
        }
    }

    debug_print("500Hz control thread stopped");
}

void IC_CAN::send_commands_all() {
    std::lock_guard<std::mutex> lock(motors_mutex_);

    // 批量发送手臂电机指令
    for (const auto& [id, motor] : motors_) {
        if (id <= 6 && motor->is_enabled()) { // m1-m6
            auto command_data = motor->get_command_data();
            if (!command_data.empty()) {
                can_adapter_->send_frame(motor->get_can_send_id(), command_data);
            }
        }
    }

    // 批量发送夹爪电机指令
    for (const auto& [id, motor] : motors_) {
        if (id > 6 && id <= 9 && motor->is_enabled()) { // m7-m9
            auto command_data = motor->get_command_data();
            if (!command_data.empty()) {
                can_adapter_->send_frame(motor->get_can_send_id(), command_data);
            }
        }
    }
}

void IC_CAN::receive_data_all(int timeout_us) {
    // 使用批量接收提高效率
    std::vector<CANFrame> frames;
    can_adapter_->receive_frames_batch(frames, 100, timeout_us);

    // 处理接收到的帧
    for (const auto& frame : frames) {
        handle_can_frame(frame.can_id, frame.data);
    }
}

void IC_CAN::execute_control_loop() {
    // 这里实现具体的控制算法
    // 可以是轨迹跟踪、阻抗控制等

    // 示例：简单的位置控制
    static int counter = 0;
    counter++;

    if (counter % 250 == 0) { // 每0.5秒执行一次位置更新
        // 这里可以添加你的控制逻辑
        debug_print("Executing control algorithm");
    }
}

void IC_CAN::handle_can_frame(uint32_t can_id, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(motors_mutex_);

    // 根据CAN ID找到对应的电机
    for (const auto& [id, motor] : motors_) {
        if (motor->get_can_recv_id() == can_id) {
            motor->process_response(data);

            // 更新安全监控
            safety_->report_communication_success(id);
            safety_->check_position_limits(id, motor->get_position());
            safety_->check_velocity_limits(id, motor->get_velocity());
            safety_->check_torque_limits(id, motor->get_torque());
            safety_->check_temperature_limits(id, motor->get_temperature());

            break;
        }
    }
}

bool IC_CAN::emergency_stop() {
    debug_print("Executing emergency stop");

    // 禁用所有电机
    disable_all();

    // 触发安全模块紧急停止
    safety_->trigger_emergency_stop("Control thread emergency stop");

    return true;
}

void IC_CAN::warn_print(const std::string& message) {
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    std::cout << "[" << timestamp << "] [IC_CAN-WARN] " << message << std::endl;
}

// ========== Missing Method Implementations ==========

bool IC_CAN::enable_all() {
    debug_print("Enabling all motors");

    std::lock_guard<std::mutex> lock(motors_mutex_);

    bool all_success = true;
    for (auto& [id, motor] : motors_) {
        if (!motor->enable()) {
            error_print("Failed to enable motor " + std::to_string(id));
            all_success = false;
        }
    }

    if (all_success) {
        info_print("All motors enabled successfully");
    }

    return all_success;
}

bool IC_CAN::disable_all() {
    debug_print("Disabling all motors");

    std::lock_guard<std::mutex> lock(motors_mutex_);

    bool all_success = true;
    for (auto& [id, motor] : motors_) {
        if (!motor->disable()) {
            error_print("Failed to disable motor " + std::to_string(id));
            all_success = false;
        }
    }

    if (all_success) {
        info_print("All motors disabled successfully");
    }

    return all_success;
}

bool IC_CAN::reset_emergency_stop() {
    debug_print("Resetting emergency stop");
    emergency_stop_flag_ = false;
    safety_->reset_emergency_stop();
    return true;
}

void IC_CAN::initialize_motor_configs() {
    debug_print("Initializing motor configurations with actual motor types from project");

    motor_configs_.clear();

    // Actual motor configuration from project requirements:
    // 1. DM 10010l - position_min/max: ±12.5 rad, max_velocity: 25.0 rad/s, max_torque: 200.0 Nm
    motor_configs_.emplace_back(1, MotorType::DM_DAMIAO, 0x01, 0x01, 50.0, 1.0, 200.0, 25.0, -12.5, 12.5);

    // 2. DM 6248 - position_min/max: ±12.566 rad, max_velocity: 20.0 rad/s, max_torque: 120.0 Nm
    motor_configs_.emplace_back(2, MotorType::DM_DAMIAO, 0x02, 0x02, 50.0, 1.0, 120.0, 20.0, -12.566, 12.566);

    // 3. DM 6248 - position_min/max: ±12.566 rad, max_velocity: 20.0 rad/s, max_torque: 120.0 Nm
    motor_configs_.emplace_back(3, MotorType::DM_DAMIAO, 0x03, 0x03, 50.0, 1.0, 120.0, 20.0, -12.566, 12.566);

    // 4. DM 4340 - position_min/max: ±12.5 rad, max_velocity: 10.0 rad/s, max_torque: 28.0 Nm
    motor_configs_.emplace_back(4, MotorType::DM_DAMIAO, 0x04, 0x04, 50.0, 1.0, 28.0, 10.0, -12.5, 12.5);

    // 5. DM 4340 - position_min/max: ±12.5 rad, max_velocity: 10.0 rad/s, max_torque: 28.0 Nm
    motor_configs_.emplace_back(5, MotorType::DM_DAMIAO, 0x05, 0x05, 50.0, 1.0, 28.0, 10.0, -12.5, 12.5);

    // 6. DM 4310 - position_min/max: ±12.5 rad, max_velocity: 30.0 rad/s, max_torque: 10.0 Nm
    motor_configs_.emplace_back(6, MotorType::DM_DAMIAO, 0x06, 0x06, 50.0, 1.0, 10.0, 30.0, -12.5, 12.5);

    // 7. HT 4438 - TODO: implement actual HT motor, using DM for now
    motor_configs_.emplace_back(7, MotorType::HT_HIGH_TORQUE, 0x07, 0x07, 40.0, 0.8, 18.0, 15.0, -12.5, 12.5);

    // 8. HT 4438 - TODO: implement actual HT motor, using DM for now
    motor_configs_.emplace_back(8, MotorType::HT_HIGH_TORQUE, 0x08, 0x08, 40.0, 0.8, 18.0, 15.0, -12.5, 12.5);

    // 9. Servo motor - TODO: implement actual servo motor, using DM for now
    motor_configs_.emplace_back(9, MotorType::SERVO, 0x09, 0x09, 30.0, 0.5, 5.0, 3.0, -12.5, 12.5);

    debug_print("Created " + std::to_string(motor_configs_.size()) + " motor configurations with actual motor types");
    debug_print("Motors: DM10010L(x1), DM6248(x2), DM4340(x2), DM4310(x1), HT4438(x2), Servo(x1)");
}

void IC_CAN::create_motors() {
    debug_print("Creating motor instances with correct motor type limits");

    std::lock_guard<std::mutex> lock(motors_mutex_);
    motors_.clear();

    for (const auto& config : motor_configs_) {
        std::shared_ptr<BaseMotor> motor;

        switch (config.type) {
            case MotorType::DM_DAMIAO:
                motor = std::make_shared<DMMotorReal>(config.motor_id, config.can_send_id, config.can_recv_id,
                                                     config.kp, config.kd, config.max_torque, debug_enabled_);

                // Set correct motor type limits based on motor ID
                if (config.motor_id == 1) {
                    // DM 10010L
                    std::static_pointer_cast<DMMotorReal>(motor)->set_motor_type_limits("DM10010L");
                    debug_print("Motor 1 set to DM10010L limits: ±12.5 rad, 25 rad/s, 200 Nm");
                } else if (config.motor_id == 2 || config.motor_id == 3) {
                    // DM 6248
                    std::static_pointer_cast<DMMotorReal>(motor)->set_motor_type_limits("DM6248");
                    debug_print("Motor " + std::to_string(config.motor_id) + " set to DM6248 limits: ±12.566 rad, 20 rad/s, 120 Nm");
                } else if (config.motor_id == 4 || config.motor_id == 5) {
                    // DM 4340
                    std::static_pointer_cast<DMMotorReal>(motor)->set_motor_type_limits("DM4340");
                    debug_print("Motor " + std::to_string(config.motor_id) + " set to DM4340 limits: ±12.5 rad, 10 rad/s, 28 Nm");
                } else if (config.motor_id == 6) {
                    // DM 4310
                    std::static_pointer_cast<DMMotorReal>(motor)->set_motor_type_limits("DM4310");
                    debug_print("Motor 6 set to DM4310 limits: ±12.5 rad, 30 rad/s, 10 Nm");
                }
                break;

            case MotorType::HT_HIGH_TORQUE:
                // TODO: Implement HT motor class when available
                debug_print("HT motor not yet implemented, using DM motor for ID " + std::to_string(config.motor_id));
                motor = std::make_shared<DMMotorReal>(config.motor_id, config.can_send_id, config.can_recv_id,
                                                     config.kp, config.kd, config.max_torque, debug_enabled_);
                // Set DM4340 limits as placeholder for HT motors
                std::static_pointer_cast<DMMotorReal>(motor)->set_motor_type_limits("DM4340");
                break;

            case MotorType::SERVO:
                // TODO: Implement servo motor class when available
                debug_print("Servo motor not yet implemented, using DM motor for ID " + std::to_string(config.motor_id));
                motor = std::make_shared<DMMotorReal>(config.motor_id, config.can_send_id, config.can_recv_id,
                                                     config.kp, config.kd, config.max_torque, debug_enabled_);
                // Set DM4340 limits as placeholder for servo motor
                std::static_pointer_cast<DMMotorReal>(motor)->set_motor_type_limits("DM4340");
                break;
        }

        if (motor) {
            motors_[config.motor_id] = motor;
            debug_print("Created motor " + std::to_string(config.motor_id) + " with CAN IDs 0x" +
                       std::to_string(config.can_send_id) + "/0x" + std::to_string(config.can_recv_id));
        }
    }

    debug_print("Created " + std::to_string(motors_.size()) + " motor instances with correct type limits");
}

void IC_CAN::update_all_states() {
    std::lock_guard<std::mutex> lock(motors_mutex_);

    for (auto& [id, motor] : motors_) {
        motor->update_state();
    }
}

void IC_CAN::refresh_all() {
    update_all_states();
    safety_->feed_watchdog();
}

std::vector<double> IC_CAN::get_joint_positions() {
    std::lock_guard<std::mutex> lock(motors_mutex_);
    std::vector<double> positions(9, 0.0);

    for (const auto& [id, motor] : motors_) {
        if (id >= 1 && id <= 9) {
            positions[id-1] = motor->get_position();
        }
    }

    return positions;
}

std::vector<double> IC_CAN::get_joint_velocities() {
    std::lock_guard<std::mutex> lock(motors_mutex_);
    std::vector<double> velocities(9, 0.0);

    for (const auto& [id, motor] : motors_) {
        if (id >= 1 && id <= 9) {
            velocities[id-1] = motor->get_velocity();
        }
    }

    return velocities;
}

std::vector<double> IC_CAN::get_joint_torques() {
    std::lock_guard<std::mutex> lock(motors_mutex_);
    std::vector<double> torques(9, 0.0);

    for (const auto& [id, motor] : motors_) {
        if (id >= 1 && id <= 9) {
            torques[id-1] = motor->get_torque();
        }
    }

    return torques;
}

bool IC_CAN::set_joint_positions(const std::vector<double>& positions,
                                const std::vector<double>& velocities,
                                const std::vector<double>& torques) {
    std::lock_guard<std::mutex> lock(motors_mutex_);

    if (positions.size() < 9) {
        error_print("Position vector size insufficient: " + std::to_string(positions.size()));
        return false;
    }

    for (int i = 0; i < 9; i++) {
        auto it = motors_.find(i + 1);
        if (it != motors_.end()) {
            double target_vel = (velocities.size() > i) ? velocities[i] : 0.0;
            double target_tau = (torques.size() > i) ? torques[i] : 0.0;

            MotorCommand cmd;
            cmd.position = positions[i];
            cmd.velocity = target_vel;
            cmd.torque = target_tau;
            cmd.use_position = true;
            it->second->set_command(cmd);
        }
    }

    return true;
}

bool IC_CAN::set_joint_torques(const std::vector<double>& torques,
                               const std::vector<double>& positions,
                               const std::vector<double>& velocities) {
    std::lock_guard<std::mutex> lock(motors_mutex_);

    if (torques.size() < 9) {
        error_print("Torque vector size insufficient: " + std::to_string(torques.size()));
        return false;
    }

    for (int i = 0; i < 9; i++) {
        auto it = motors_.find(i + 1);
        if (it != motors_.end()) {
            double target_pos = (positions.size() > i) ? positions[i] : it->second->get_position();
            double target_vel = (velocities.size() > i) ? velocities[i] : 0.0;

            MotorCommand cmd;
            cmd.position = target_pos;
            cmd.velocity = target_vel;
            cmd.torque = torques[i];
            cmd.use_torque = true;
            it->second->set_command(cmd);
        }
    }

    return true;
}

void IC_CAN::set_control_frequency(double frequency_hz) {
    if (frequency_hz > 0 && frequency_hz <= 2000) {
        control_frequency_ = frequency_hz;
        debug_print("Control frequency set to " + std::to_string(frequency_hz) + "Hz");
    } else {
        warn_print("Invalid control frequency: " + std::to_string(frequency_hz) + "Hz");
    }
}

std::map<std::string, std::string> IC_CAN::get_system_status() {
    std::map<std::string, std::string> status;

    status["initialized"] = initialized_ ? "true" : "false";
    status["emergency_stop"] = emergency_stop_flag_ ? "true" : "false";
    status["control_running"] = control_thread_running_ ? "true" : "false";
    status["control_frequency"] = std::to_string(control_frequency_) + "Hz";
    status["motor_count"] = std::to_string(motors_.size());
    status["error_count"] = std::to_string(error_count_.load());
    status["device_sn"] = device_sn_;

    if (can_adapter_) {
        status["can_connected"] = can_adapter_->is_connected() ? "true" : "false";
        status["sent_frames"] = std::to_string(can_adapter_->get_sent_frames_count());
        status["received_frames"] = std::to_string(can_adapter_->get_received_frames_count());
    }

    return status;
}

void IC_CAN::print_system_info() {
    auto status = get_system_status();

    std::cout << "\n=== IC_CAN System Information ===" << std::endl;
    for (const auto& [key, value] : status) {
        std::cout << "  " << key << ": " << value << std::endl;
    }

    std::cout << "\nMotor Configurations:" << std::endl;
    for (const auto& config : motor_configs_) {
        std::cout << "  Motor " << config.motor_id
                  << " (Type " << static_cast<int>(config.type) << ")"
                  << " CAN IDs: 0x" << std::hex << config.can_send_id
                  << "/0x" << config.can_recv_id << std::dec
                  << " Limits: [" << config.position_min << ", " << config.position_max << "] rad" << std::endl;
    }
    std::cout << "================================" << std::endl;
}

void IC_CAN::debug_print(const std::string& message) {
    if (debug_enabled_) {
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        std::cout << "[" << timestamp << "] [IC_CAN-DEBUG] " << message << std::endl;
    }
}

void IC_CAN::increment_error_count() {
    error_count_++;
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

} // namespace ic_can