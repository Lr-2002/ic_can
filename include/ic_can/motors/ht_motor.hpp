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

#pragma once

#include "base_motor.hpp"
#include <cstdint>
#include <vector>
#include <iostream>

namespace ic_can {

// HT电机控制模式枚举
enum class HTControlMode {
    POSITION_CONTROL,    // 位置控制模式
    FORCE_CONTROL,       // 力控制模式
    VELOCITY_CONTROL     // 速度控制模式
};

// HT motor constants
constexpr double RAD_TO_TURN = 1.0 / (2.0 * M_PI);  // Convert radians to turns
constexpr double TURN_TO_RAD = 2.0 * M_PI;          // Convert turns to radians

// Debug function for HT motor
static inline void debug_print(const std::string& message) {
    std::cout << "[DEBUG] HTMotor: " << message << std::endl;
}

/**
 * @brief 高力矩电机类 (HT Motor)
 *
 * HT4438 高力矩电机控制类，支持位置控制和力控制
 */
class HTMotor : public BaseMotor {
public:
    /**
     * @brief 构造函数
     * @param motor_id 电机ID
     * @param can_send_id 发送CAN ID
     * @param can_recv_id 接收CAN ID
     * @param kp 比例增益
     * @param kd 微分增益
     * @param max_torque 最大力矩
     * @param debug 调试模式
     */
    HTMotor(int motor_id, uint32_t can_send_id, uint32_t can_recv_id,
             double kp, double kd, double max_torque, bool debug = false);

    /**
     * @brief 析构函数
     */
    virtual ~HTMotor() = default;

    // ========== 基础控制接口 ==========

    bool enable() override;
    bool disable() override;
    bool set_zero() override;
    bool set_command(const MotorCommand& command) override;

    // ========== HT电机专用控制接口 ==========

    /**
     * @brief 设置位置控制
     * @param position 目标位置 (弧度)
     * @param velocity 最大速度 (弧度/秒)
     * @return 操作是否成功
     */
    bool set_position_control(double position, double velocity = 2.0);

    /**
     * @brief 设置力控制
     * @param force 目标力 (牛顿)
     * @return 操作是否成功
     */
    bool set_force_control(double force);

    /**
     * @brief 设置速度控制
     * @param velocity 目标速度 (弧度/秒)
     * @return 操作是否成功
     */
    bool set_velocity_control(double velocity);

    // ========== Override BaseMotor virtual methods ==========

    bool set_position(double position, double velocity = 0.0, double torque = 0.0) override;
    bool set_velocity(double velocity, double torque = 0.0) override;
    bool set_torque(double torque) override;
    bool mit_control(double position, double velocity, double torque, double kp, double kd) override;
    double get_position() const override;
    double get_velocity() const override;
    double get_torque() const override;
    double get_temperature() const override;
    void update_state() override;
    MotorState get_state() const override;
    bool is_enabled() const override;
    void set_limits(const MotorLimits& limits) override;
    void set_max_position(double max_position) override;
    void set_min_position(double min_position) override;
    void set_max_velocity(double max_velocity) override;
    void set_max_torque(double max_torque) override;
    bool is_position_safe(double position) const override;
    bool is_velocity_safe(double velocity) const override;
    bool is_torque_safe(double torque) const override;
    bool is_temperature_safe(double temperature) const override;
    void reset_error() override;
    int get_motor_id() const override;
    uint32_t get_can_send_id() const override;
    uint32_t get_can_recv_id() const override;
    std::vector<uint8_t> get_command_data() const override;
    bool process_response(const std::vector<uint8_t>& data) override;

    /**
     * @brief 获取当前力值
     * @return 当前力 (牛顿)
     */
    double get_current_force() const { return force_.load(); }

    
    /**
     * @brief 获取错误状态
     * @return 错误代码
     */
    uint8_t get_error_code() const { return error_code_.load(); }

    // ========== 配置接口 ==========

    /**
     * @brief 设置位置限制
     * @param min_pos 最小位置
     * @param max_pos 最大位置
     */
    void set_position_limits(double min_pos, double max_pos);

    /**
     * @brief 设置力限制
     * @param max_force 最大力 (牛顿)
     */
    void set_force_limit(double max_force);

    /**
     * @brief 设置控制模式
     * @param mode 控制模式
     */
    void set_control_mode(HTControlMode mode);

    /**
     * @brief Set HT motor parameters
     * @param kp 比例增益
     * @param kd 微分增益
     * @param max_torque 最大力矩
     */
    void set_ht_params(double kp, double kd, double max_torque);

private:
    // ========== 私有成员变量 ==========

    // Motor identification
    int motor_id_;
    uint32_t can_send_id_;
    uint32_t can_recv_id_;

    // Control parameters
    double kp_, kd_, max_torque_;
    HTControlMode control_mode_;

    // State variables
    double position_, velocity_, torque_;
    std::atomic<double> temperature_{25.0};   // 温度
    std::atomic<double> force_{0.0};         // 当前力值
    std::atomic<double> target_force_{0.0};   // 目标力值
    std::atomic<uint8_t> error_code_{0};      // 错误代码
    uint8_t error_;

    // Motor limits
    double position_limit_min_, position_limit_max_;
    double velocity_limit_max_, torque_limit_max_;
    double max_force_;

    // HT motor specific constants
    double torque_k_, torque_d_;
    double RAD_TO_TURN_, TURN_TO_RAD_;

    // Control flags
    bool enabled_, debug_enabled_;
    bool has_pending_command_;                // 是否有待发送的命令

    // Command data
    std::vector<uint8_t> command_data_;

    // ========== 私有方法 ==========

    /**
     * @brief Pack HT MIT command data
     * @param position 目标位置
     * @param velocity 目标速度
     * @param torque 目标力矩
     * @param kp 比例增益
     * @param kd 微分增益
     * @return 命令数据
     */
    std::vector<uint8_t> pack_ht_mit_command(double position, double velocity,
                                           double torque, double kp, double kd);

    /**
     * @brief Unpack HT response data
     * @param data 响应数据
     */
    void unpack_ht_response(const std::vector<uint8_t>& data);

    /**
     * @brief 浮点数转换为整数 (HT协议格式)
     */
    uint16_t float_to_ht_format(float value, float min_val, float max_val);

    /**
     * @brief HT格式转换为浮点数
     */
    float ht_format_to_float(uint16_t raw_value, float min_val, float max_val);

    /**
     * @brief Debug print function
     * @param message Debug message
     */
    void debug_print(const std::string& message);
};

} // namespace ic_can