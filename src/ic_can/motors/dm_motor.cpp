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

#include "ic_can/motors/dm_motor.hpp"
#include <iostream>
#include <cmath>
#include <cstring>
#include <chrono>

namespace ic_can {

DMMotor::DMMotor(int motor_id, DMMotorType motor_type,
                 uint32_t can_send_id, uint32_t can_recv_id)
    : BaseMotor(motor_id, MotorType::DM_DAMIAO, can_send_id, can_recv_id),
      dm_motor_type_(motor_type), control_mode_(DMControlMode::MIT_MODE),
      mit_kp_(0.0), mit_kd_(0.0), has_pending_command_(false),
      parameter_cache_valid_(false) {

    // 根据电机类型设置默认限制参数
    MotorLimits default_limits = get_default_limits(motor_type);
    set_limits(default_limits);

    // 初始化参数缓存
    parameter_cache_[DMRegisterID::MST_ID] = static_cast<float>(motor_id);
    parameter_cache_[DMRegisterID::ESC_ID] = static_cast<float>(can_recv_id);
    parameter_cache_valid_ = true;

    debug_print("DMMotor initialized: ID=" + std::to_string(motor_id) +
                ", Type=" + std::to_string(static_cast<int>(motor_type)));
}

bool DMMotor::enable() {
    std::lock_guard<std::mutex> lock(command_mutex_);
    command_buffer_ = create_enable_command();
    has_pending_command_ = true;
    set_enabled_state(true);
    debug_print("Enable command queued");
    return true;
}

bool DMMotor::disable() {
    std::lock_guard<std::mutex> lock(command_mutex_);
    command_buffer_ = create_disable_command();
    has_pending_command_ = true;
    set_enabled_state(false);
    debug_print("Disable command queued");
    return true;
}

bool DMMotor::set_zero() {
    std::lock_guard<std::mutex> lock(command_mutex_);
    command_buffer_ = create_set_zero_command();
    has_pending_command_ = true;
    debug_print("Set zero command queued");
    return true;
}

bool DMMotor::set_command(const MotorCommand& command) {
    if (!is_command_safe(command)) {
        debug_print("Command safety check failed");
        return false;
    }

    std::lock_guard<std::mutex> lock(command_mutex_);
    last_command_ = command;

    if (command.use_position || command.use_velocity || command.use_torque) {
        return mit_control(command.position, command.velocity, command.torque,
                         command.kp, command.kd);
    }

    debug_print("No valid control mode in command");
    return false;
}

bool DMMotor::set_position(double position, double velocity, double torque) {
    if (!is_position_safe(position)) {
        debug_print("Position out of limits: " + std::to_string(position));
        return false;
    }

    MotorCommand cmd;
    cmd.position = position;
    cmd.velocity = velocity;
    cmd.torque = torque;
    cmd.use_position = true;
    cmd.use_velocity = true;
    cmd.use_torque = true;
    cmd.kp = mit_kp_;
    cmd.kd = mit_kd_;

    return set_command(cmd);
}

bool DMMotor::set_velocity(double velocity, double torque) {
    if (!is_velocity_safe(velocity)) {
        debug_print("Velocity out of limits: " + std::to_string(velocity));
        return false;
    }

    MotorCommand cmd;
    cmd.position = get_position(); // 保持当前位置
    cmd.velocity = velocity;
    cmd.torque = torque;
    cmd.use_position = false;
    cmd.use_velocity = true;
    cmd.use_torque = true;
    cmd.kp = 0.0;
    cmd.kd = mit_kd_;

    return set_command(cmd);
}

bool DMMotor::set_torque(double torque) {
    if (!is_torque_safe(torque)) {
        debug_print("Torque out of limits: " + std::to_string(torque));
        return false;
    }

    MotorCommand cmd;
    cmd.position = get_position(); // 保持当前位置
    cmd.velocity = 0.0;
    cmd.torque = torque;
    cmd.use_position = false;
    cmd.use_velocity = false;
    cmd.use_torque = true;
    cmd.kp = 0.0;
    cmd.kd = 0.0;

    return set_command(cmd);
}

bool DMMotor::mit_control(double position, double velocity, double torque,
                        double kp, double kd) {
    std::lock_guard<std::mutex> lock(command_mutex_);
    command_buffer_ = create_mit_command(position, velocity, torque, kp, kd);
    has_pending_command_ = true;
    mit_kp_ = kp;
    mit_kd_ = kd;
    return true;
}

double DMMotor::read_parameter(DMRegisterID register_id) {
    // 检查缓存
    if (parameter_cache_valid_ && parameter_cache_.find(register_id) != parameter_cache_.end()) {
        return parameter_cache_[register_id];
    }

    // 发送查询命令
    std::lock_guard<std::mutex> lock(command_mutex_);
    command_buffer_ = create_read_param_command(register_id);
    has_pending_command_ = true;

    // ⚠️ 仿真：返回默认值，实际应该等待响应
    debug_print("⚠️ Reading parameter with simulated response");
    switch (register_id) {
        case DMRegisterID::MST_ID:
            return static_cast<double>(get_motor_id());
        case DMRegisterID::ESC_ID:
            return static_cast<double>(get_can_recv_id());
        case DMRegisterID::HW_VER:
            return 1.0;
        case DMRegisterID::SW_VER:
            return 1.0;
        default:
            return 0.0;
    }
}

bool DMMotor::write_parameter(DMRegisterID register_id, float value) {
    std::lock_guard<std::mutex> lock(command_mutex_);
    command_buffer_ = create_write_param_command(register_id, value);
    has_pending_command_ = true;

    // ⚠️ 仿真：直接更新缓存
    parameter_cache_[register_id] = value;
    parameter_cache_valid_ = true;

    debug_print("⚠️ Writing parameter with simulated success");
    return true;
}

void DMMotor::refresh() {
    std::lock_guard<std::mutex> lock(command_mutex_);
    command_buffer_ = create_refresh_command();
    has_pending_command_ = true;
}

void DMMotor::update_state() {
    // ⚠️ 仿真：模拟状态更新
    auto current_time = std::chrono::steady_clock::now();
    auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(
        current_time - get_state_snapshot().timestamp).count() / 1000.0;

    if (dt > 0) {
        double current_pos = get_position();
        double current_vel = get_velocity();
        double current_tau = get_torque();

        // 简单的物理仿真
        double new_pos = current_pos + current_vel * dt;
        double new_vel = current_vel * 0.99; // 阻尼
        double new_tau = current_tau * 0.95; // 力矩衰减

        // 添加一些噪声
        new_pos += (std::rand() % 100 - 50) * 0.00001;
        new_vel += (std::rand() % 100 - 50) * 0.0001;

        update_internal_state(new_pos, new_vel, new_tau, get_temperature());
    }
}

bool DMMotor::process_response(const std::vector<uint8_t>& data) {
    if (data.size() < 6) {
        debug_print("Invalid response data size: " + std::to_string(data.size()));
        return false;
    }

    // 检查数据包类型 (假设第一个字节是数据类型)
    uint8_t data_type = data[0];

    if (data_type == 0x01) {
        // 状态响应
        return parse_state_response(data);
    } else if (data_type == 0x02) {
        // 参数响应
        float value;
        bool success = parse_parameter_response(data, value);
        if (success) {
            // ⚠️ 仿真：简单解析，实际需要根据寄存器ID处理
            parameter_cache_[static_cast<DMRegisterID>(data[1])] = value;
            parameter_cache_valid_ = true;
        }
        return success;
    }

    debug_print("Unknown response data type: " + std::to_string(data_type));
    return false;
}

std::vector<uint8_t> DMMotor::get_command_data() const {
    std::lock_guard<std::mutex> lock(command_mutex_);
    if (has_pending_command_) {
        return command_buffer_;
    }
    return {};
}

MotorLimits DMMotor::get_default_limits(DMMotorType motor_type) {
    MotorLimits limits;

    switch (motor_type) {
        case DMMotorType::DM4310:
            limits.max_position = 3.14159;   // ±180度
            limits.min_position = -3.14159;
            limits.max_velocity = 5.0;       // 5 rad/s
            limits.max_torque = 3.0;         // 3 Nm
            limits.max_temperature = 80.0;    // 80°C
            break;

        case DMMotorType::DM6248:
            limits.max_position = 10.47198;  // ±600度
            limits.min_position = -10.47198;
            limits.max_velocity = 10.0;      // 10 rad/s
            limits.max_torque = 8.0;         // 8 Nm
            limits.max_temperature = 85.0;    // 85°C
            break;

        case DMMotorType::DM4340:
            limits.max_position = 6.28319;   // ±360度
            limits.min_position = -6.28319;
            limits.max_velocity = 8.0;       // 8 rad/s
            limits.max_torque = 4.0;         // 4 Nm
            limits.max_temperature = 80.0;    // 80°C
            break;

        case DMMotorType::DM10010L:
            limits.max_position = 52.35988;  // ±3000度
            limits.min_position = -52.35988;
            limits.max_velocity = 15.0;      // 15 rad/s
            limits.max_torque = 15.0;        // 15 Nm
            limits.max_temperature = 90.0;    // 90°C
            break;

        default:
            // 使用默认值
            break;
    }

    return limits;
}

double DMMotor::angle_to_internal_units(double angle_rad) {
    // ⚠️ 仿真：假设内部单位与弧度相同
    return angle_rad;
}

double DMMotor::internal_units_to_angle(double internal_units) {
    // ⚠️ 仿真：假设内部单位与弧度相同
    return internal_units;
}

// ========== 私有方法实现 ==========

std::vector<uint8_t> DMMotor::create_enable_command() {
    std::vector<uint8_t> data = {static_cast<uint8_t>(DMCommand::ENABLE)};
    debug_print("Creating enable command");
    return data;
}

std::vector<uint8_t> DMMotor::create_disable_command() {
    std::vector<uint8_t> data = {static_cast<uint8_t>(DMCommand::DISABLE)};
    debug_print("Creating disable command");
    return data;
}

std::vector<uint8_t> DMMotor::create_set_zero_command() {
    std::vector<uint8_t> data = {static_cast<uint8_t>(DMCommand::SET_ZERO)};
    debug_print("Creating set zero command");
    return data;
}

std::vector<uint8_t> DMMotor::create_refresh_command() {
    std::vector<uint8_t> data = {static_cast<uint8_t>(DMCommand::REFRESH)};
    debug_print("Creating refresh command");
    return data;
}

std::vector<uint8_t> DMMotor::create_mit_command(double position, double velocity,
                                                double torque, double kp, double kd) {
    std::vector<uint8_t> data(8);

    // 获取电机参数范围
    double p_max, v_max, t_max;
    get_motor_limits(dm_motor_type_, p_max, v_max, t_max);

    // 转换为内部格式
    uint16_t p_int = float_to_uint(position, -p_max, p_max, 16);
    uint16_t v_int = float_to_uint(velocity, -v_max, v_max, 16);
    uint16_t kp_int = float_to_uint(kp, 0, 500, 12);
    uint16_t kd_int = float_to_uint(kd, 0, 5, 12);
    uint16_t t_int = float_to_uint(torque, -t_max, t_max, 12);

    // 组装数据包 (参考达妙协议)
    data[0] = p_int & 0xFF;
    data[1] = (p_int >> 8) & 0xFF;
    data[2] = v_int & 0xFF;
    data[3] = (v_int >> 8) & 0xFF;
    data[4] = kp_int & 0xFF;
    data[5] = ((kp_int >> 8) & 0x0F) | ((kd_int & 0x0F) << 4);
    data[6] = (kd_int >> 4) & 0xFF;
    data[7] = t_int & 0xFF;

    debug_print("Creating MIT command: p=" + std::to_string(position) +
                ", v=" + std::to_string(velocity) + ", t=" + std::to_string(torque));

    return data;
}

std::vector<uint8_t> DMMotor::create_read_param_command(DMRegisterID register_id) {
    std::vector<uint8_t> data(3);
    data[0] = 0x7F; // 广播地址
    data[1] = static_cast<uint8_t>(register_id);
    data[2] = static_cast<uint8_t>(get_motor_id());

    debug_print("Creating read param command: reg=" + std::to_string(static_cast<int>(register_id)));
    return data;
}

std::vector<uint8_t> DMMotor::create_write_param_command(DMRegisterID register_id, float value) {
    std::vector<uint8_t> data(7);
    data[0] = static_cast<uint8_t>(get_can_send_id());
    data[1] = static_cast<uint8_t>(register_id);

    // 将浮点数转换为字节数组
    uint32_t* int_ptr = reinterpret_cast<uint32_t*>(&value);
    uint32_t int_val = *int_ptr;

    data[2] = int_val & 0xFF;
    data[3] = (int_val >> 8) & 0xFF;
    data[4] = (int_val >> 16) & 0xFF;
    data[5] = (int_val >> 24) & 0xFF;
    data[6] = 0x00; // 校验位 (简化)

    debug_print("Creating write param command: reg=" + std::to_string(static_cast<int>(register_id)) +
                ", val=" + std::to_string(value));

    return data;
}

bool DMMotor::parse_state_response(const std::vector<uint8_t>& data) {
    if (data.size() < 6) {
        return false;
    }

    // ⚠️ 仿真：简化解析，实际需要根据达妙协议解析
    // 假设数据格式：[type, p_low, p_high, v_low, v_high, t_low, t_high, temp]
    if (data.size() >= 7) {
        uint16_t p_int = (static_cast<uint16_t>(data[2]) << 8) | data[1];
        uint16_t v_int = (static_cast<uint16_t>(data[4]) << 8) | data[3];
        uint16_t t_int = (static_cast<uint16_t>(data[6]) << 8) | data[5];

        double p_max, v_max, t_max;
        get_motor_limits(dm_motor_type_, p_max, v_max, t_max);

        double position = uint_to_float(p_int, -p_max, p_max, 16);
        double velocity = uint_to_float(v_int, -v_max, v_max, 16);
        double torque = uint_to_float(t_int, -t_max, t_max, 16);

        double temperature = 25.0; // ⚠️ 仿真：固定温度
        if (data.size() > 7) {
            temperature = static_cast<double>(data[7]);
        }

        update_internal_state(position, velocity, torque, temperature);
        set_error_state(false);

        debug_print("State updated: p=" + std::to_string(position) +
                    ", v=" + std::to_string(velocity) + ", t=" + std::to_string(torque));

        return true;
    }

    return false;
}

bool DMMotor::parse_parameter_response(const std::vector<uint8_t>& data, float& value) {
    if (data.size() < 6) {
        return false;
    }

    // ⚠️ 仿真：简化参数解析
    if (data.size() >= 6) {
        uint32_t int_val = (static_cast<uint32_t>(data[5]) << 24) |
                           (static_cast<uint32_t>(data[4]) << 16) |
                           (static_cast<uint32_t>(data[3]) << 8) |
                           static_cast<uint32_t>(data[2]);

        value = *reinterpret_cast<float*>(&int_val);
        return true;
    }

    return false;
}

uint16_t DMMotor::float_to_uint(double x, double min, double max, int bits) {
    double span = max - min;
    if (x < min) x = min;
    if (x > max) x = max;
    return static_cast<uint16_t>((x - min) * ((1 << bits) - 1) / span);
}

double DMMotor::uint_to_float(uint16_t x, double min, double max, int bits) {
    double span = max - min;
    return min + static_cast<double>(x) * span / ((1 << bits) - 1);
}

void DMMotor::get_motor_limits(DMMotorType motor_type,
                              double& p_max, double& v_max, double& t_max) {
    MotorLimits limits = get_default_limits(motor_type);
    p_max = std::max(std::abs(limits.max_position), std::abs(limits.min_position));
    v_max = limits.max_velocity;
    t_max = limits.max_torque;
}

void DMMotor::debug_print(const std::string& message) {
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    std::cout << "[" << timestamp << "] [DM_Motor-" << get_motor_id() << "] "
              << message << std::endl;
}

} // namespace ic_can