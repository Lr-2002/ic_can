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

#include <cstdint>
#include <atomic>
#include <mutex>
#include <chrono>
#include <vector>
#include <string>

namespace ic_can {

// 电机类型枚举
enum class MotorType {
    DM_DAMIAO,         // 达妙电机
    HT_HIGH_TORQUE,    // 高扭矩电机
    SERVO              // 舵机
};

// 电机状态结构
struct MotorState {
    double position;          // 位置 (弧度)
    double velocity;          // 速度 (弧度/秒)
    double torque;            // 力矩 (牛顿·米)
    double temperature;       // 温度 (摄氏度)
    bool enabled;             // 使能状态
    bool error;               // 错误状态
    std::chrono::steady_clock::time_point timestamp; // 时间戳

    MotorState() : position(0.0), velocity(0.0), torque(0.0),
                   temperature(0.0), enabled(false), error(false),
                   timestamp(std::chrono::steady_clock::now()) {}
};

// 电机命令结构
struct MotorCommand {
    double position;          // 目标位置 (弧度)
    double velocity;          // 目标速度 (弧度/秒)
    double torque;            // 目标力矩 (牛顿·米)
    double kp;                // 位置增益
    double kd;                // 速度增益
    bool use_position;        // 是否使用位置控制
    bool use_velocity;        // 是否使用速度控制
    bool use_torque;          // 是否使用力矩控制

    MotorCommand() : position(0.0), velocity(0.0), torque(0.0),
                     kp(0.0), kd(0.0), use_position(false),
                     use_velocity(false), use_torque(false) {}
};

// 电机限制参数
struct MotorLimits {
    double max_position;      // 最大位置 (弧度)
    double min_position;      // 最小位置 (弧度)
    double max_velocity;      // 最大速度 (弧度/秒)
    double max_torque;        // 最大力矩 (牛顿·米)
    double max_temperature;   // 最大温度 (摄氏度)

    MotorLimits() : max_position(3.14159), min_position(-3.14159),
                    max_velocity(10.0), max_torque(10.0), max_temperature(80.0) {}
};

// 电机基类
class BaseMotor {
public:
    /**
     * @brief 构造函数
     * @param motor_id 电机ID
     * @param type 电机类型
     * @param can_send_id 发送CAN ID
     * @param can_recv_id 接收CAN ID
     */
    BaseMotor(int motor_id, MotorType type, uint32_t can_send_id, uint32_t can_recv_id);

    /**
     * @brief 虚析构函数
     */
    virtual ~BaseMotor() = default;

    // ========== 状态获取接口 ==========

    /**
     * @brief 获取电机ID
     * @return 电机ID
     */
    int get_motor_id() const { return motor_id_; }

    /**
     * @brief 获取电机类型
     * @return 电机类型
     */
    MotorType get_type() const { return type_; }

    /**
     * @brief 获取发送CAN ID
     * @return 发送CAN ID
     */
    uint32_t get_can_send_id() const { return can_send_id_; }

    /**
     * @brief 获取接收CAN ID
     * @return 接收CAN ID
     */
    uint32_t get_can_recv_id() const { return can_recv_id_; }

    /**
     * @brief 获取当前位置
     * @return 当前位置 (弧度)
     */
    double get_position() const { return position_.load(); }

    /**
     * @brief 获取当前速度
     * @return 当前速度 (弧度/秒)
     */
    double get_velocity() const { return velocity_.load(); }

    /**
     * @brief 获取当前力矩
     * @return 当前力矩 (牛顿·米)
     */
    double get_torque() const { return torque_.load(); }

    /**
     * @brief 获取当前温度
     * @return 当前温度 (摄氏度)
     */
    double get_temperature() const { return temperature_.load(); }

    /**
     * @brief 检查电机是否使能
     * @return 是否使能
     */
    bool is_enabled() const { return enabled_.load(); }

    /**
     * @brief 检查电机是否有错误
     * @return 是否有错误
     */
    bool has_error() const { return error_.load(); }

    /**
     * @brief 获取完整状态
     * @return 电机状态结构
     */
    MotorState get_state() const;

    /**
     * @brief 获取电机限制参数
     * @return 限制参数
     */
    MotorLimits get_limits() const { return limits_; }

    // ========== 控制接口 ==========

    /**
     * @brief 使能电机
     * @return 操作是否成功
     */
    virtual bool enable() = 0;

    /**
     * @brief 禁用电机
     * @return 操作是否成功
     */
    virtual bool disable() = 0;

    /**
     * @brief 设置零位
     * @return 操作是否成功
     */
    virtual bool set_zero() = 0;

    /**
     * @brief 设置电机命令
     * @param command 电机命令
     * @return 操作是否成功
     */
    virtual bool set_command(const MotorCommand& command) = 0;

    /**
     * @brief 设置位置命令
     * @param position 目标位置 (弧度)
     * @param velocity 目标速度 (弧度/秒)
     * @param torque 目标力矩 (牛顿·米)
     * @return 操作是否成功
     */
    virtual bool set_position(double position, double velocity = 0.0, double torque = 0.0) = 0;

    /**
     * @brief 设置速度命令
     * @param velocity 目标速度 (弧度/秒)
     * @param torque 目标力矩 (牛顿·米)
     * @return 操作是否成功
     */
    virtual bool set_velocity(double velocity, double torque = 0.0) = 0;

    /**
     * @brief 设置力矩命令
     * @param torque 目标力矩 (牛顿·米)
     * @return 操作是否成功
     */
    virtual bool set_torque(double torque) = 0;

    // ========== MIT控制接口 (达妙电机专用) ==========

    /**
     * @brief MIT控制
     * @param position 目标位置 (弧度)
     * @param velocity 目标速度 (弧度/秒)
     * @param torque 目标力矩 (牛顿·米)
     * @param kp 位置增益
     * @param kd 速度增益
     * @return 操作是否成功
     */
    virtual bool mit_control(double position, double velocity, double torque,
                           double kp, double kd) = 0;

    // ========== 参数设置接口 ==========

    /**
     * @brief 设置电机限制参数
     * @param limits 限制参数
     */
    void set_limits(const MotorLimits& limits);

    /**
     * @brief 设置最大位置
     * @param max_position 最大位置 (弧度)
     */
    void set_max_position(double max_position);

    /**
     * @brief 设置最小位置
     * @param min_position 最小位置 (弧度)
     */
    void set_min_position(double min_position);

    /**
     * @brief 设置最大速度
     * @param max_velocity 最大速度 (弧度/秒)
     */
    void set_max_velocity(double max_velocity);

    /**
     * @brief 设置最大力矩
     * @param max_torque 最大力矩 (牛顿·米)
     */
    void set_max_torque(double max_torque);

    // ========== 安全检查接口 ==========

    /**
     * @brief 检查位置是否在限制范围内
     * @param position 位置值
     * @return 是否在范围内
     */
    bool is_position_safe(double position) const;

    /**
     * @brief 检查速度是否在限制范围内
     * @param velocity 速度值
     * @return 是否在范围内
     */
    bool is_velocity_safe(double velocity) const;

    /**
     * @brief 检查力矩是否在限制范围内
     * @param torque 力矩值
     * @return 是否在范围内
     */
    bool is_torque_safe(double torque) const;

    /**
     * @brief 检查温度是否在限制范围内
     * @param temperature 温度值
     * @return 是否在范围内
     */
    bool is_temperature_safe(double temperature) const;

    /**
     * @brief 检查命令是否安全
     * @param command 电机命令
     * @return 是否安全
     */
    bool is_command_safe(const MotorCommand& command) const;

    // ========== 状态更新接口 ==========

    /**
     * @brief 更新电机状态（由子类实现）
     */
    virtual void update_state() = 0;

    /**
     * @brief 处理接收到的数据（由子类实现）
     * @param data 接收到的数据
     * @return 处理是否成功
     */
    virtual bool process_response(const std::vector<uint8_t>& data) = 0;

    /**
     * @brief 获取需要发送的命令数据（由子类实现）
     * @return CAN帧数据
     */
    virtual std::vector<uint8_t> get_command_data() const = 0;

protected:
    // ========== 受保护的成员变量 ==========

    int motor_id_;                        // 电机ID
    MotorType type_;                      // 电机类型
    uint32_t can_send_id_;                // 发送CAN ID
    uint32_t can_recv_id_;                // 接收CAN ID

    // 状态变量（原子操作保证线程安全）
    std::atomic<double> position_{0.0};   // 当前位置
    std::atomic<double> velocity_{0.0};   // 当前速度
    std::atomic<double> torque_{0.0};     // 当前力矩
    std::atomic<double> temperature_{0.0}; // 当前温度
    std::atomic<bool> enabled_{false};    // 使能状态
    std::atomic<bool> error_{false};      // 错误状态

    // 配置参数
    MotorLimits limits_;                  // 限制参数
    MotorCommand last_command_;           // 最后的命令

    // 同步原语
    mutable std::mutex state_mutex_;      // 状态互斥锁
    mutable std::mutex command_mutex_;    // 命令互斥锁

    // ========== 受保护的方法 ==========

    /**
     * @brief 更新内部状态
     * @param position 位置
     * @param velocity 速度
     * @param torque 力矩
     * @param temperature 温度
     */
    void update_internal_state(double position, double velocity,
                              double torque, double temperature);

    /**
     * @brief 设置使能状态
     * @param enabled 使能状态
     */
    void set_enabled_state(bool enabled);

    /**
     * @brief 设置错误状态
     * @param error 错误状态
     */
    void set_error_state(bool error);

    /**
     * @brief 获取当前状态快照
     * @return 状态快照
     */
    MotorState get_state_snapshot() const;

private:
    // 禁用拷贝构造和赋值
    BaseMotor(const BaseMotor&) = delete;
    BaseMotor& operator=(const BaseMotor&) = delete;

};

} // namespace ic_can