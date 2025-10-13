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

#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>

#include "../motors/base_motor.hpp"
#include "../protocols/usb2can_adapter.hpp"
#include "../safety/safety_module.hpp"
#include "arm_component.hpp"
#include "gripper_component.hpp"

namespace ic_can {

// 电机配置结构
struct MotorConfig {
    int motor_id;              // 电机ID (1-9)
    MotorType type;            // 电机类型
    uint32_t can_send_id;      // 发送CAN ID
    uint32_t can_recv_id;      // 接收CAN ID
    double kp;                 // 位置增益
    double kd;                 // 速度增益
    double max_torque;         // 最大扭矩
    double max_velocity;       // 最大速度
    double position_min;       // 最小位置限制
    double position_max;       // 最大位置限制

    MotorConfig(int id, MotorType t, uint32_t send_id, uint32_t recv_id,
                double kp_val, double kd_val, double max_tau, double max_vel,
                double pos_min, double pos_max)
        : motor_id(id), type(t), can_send_id(send_id), can_recv_id(recv_id),
          kp(kp_val), kd(kd_val), max_torque(max_tau), max_velocity(max_vel),
          position_min(pos_min), position_max(pos_max) {}
};

// 主控制类
class IC_CAN {
public:
    /**
     * @brief 构造函数
     * @param device_sn USB2CAN设备序列号
     * @param enable_debug 是否启用调试输出
     */
    explicit IC_CAN(const std::string& device_sn = "F561E08C892274DB09496BCC1102DBC5",
                    bool enable_debug = false);

    /**
     * @brief 析构函数
     */
    ~IC_CAN();

    // 禁用拷贝构造和赋值
    IC_CAN(const IC_CAN&) = delete;
    IC_CAN& operator=(const IC_CAN&) = delete;

    // ========== 系统控制接口 ==========

    /**
     * @brief 初始化系统
     * @return 初始化是否成功
     */
    bool initialize();

    /**
     * @brief 检查系统是否已初始化
     * @return 是否已初始化
     */
    bool is_initialized() const { return initialized_; }

    /**
     * @brief 使能所有电机
     * @return 操作是否成功
     */
    bool enable_all();

    /**
     * @brief 禁用所有电机
     * @return 操作是否成功
     */
    bool disable_all();

    /**
     * @brief 紧急停止
     * @return 操作是否成功
     */
    bool emergency_stop();

    /**
     * @brief 警告输出
     * @param message 警告信息
     */
    void warn_print(const std::string& message);

    /**
     * @brief 重置紧急停止状态
     * @return 操作是否成功
     */
    bool reset_emergency_stop();

    // ========== 组件访问接口 ==========

    /**
     * @brief 获取手臂组件
     * @return 手臂组件引用
     */
    ArmComponent& get_arm() { return *arm_; }

    /**
     * @brief 获取夹爪组件
     * @return 夹爪组件引用
     */
    GripperComponent& get_gripper() { return *gripper_; }

    /**
     * @brief 获取安全模块
     * @return 安全模块引用
     */
    SafetyModule& get_safety() { return *safety_; }

    // ========== 状态读取接口 ==========

    /**
     * @brief 更新所有电机状态
     */
    void update_all_states();

    /**
     * @brief 刷新所有状态（包含安全检查）
     */
    void refresh_all();

    /**
     * @brief 获取所有关节位置（弧度）
     * @return 位置数组
     */
    std::vector<double> get_joint_positions();

    /**
     * @brief 获取所有关节速度（弧度/秒）
     * @return 速度数组
     */
    std::vector<double> get_joint_velocities();

    /**
     * @brief 获取所有关节力矩（牛顿·米）
     * @return 力矩数组
     */
    std::vector<double> get_joint_torques();

    // ========== 控制接口 ==========

    /**
     * @brief 设置关节位置
     * @param positions 目标位置数组（弧度）
     * @param velocities 目标速度数组（弧度/秒），可选
     * @param torques 目标力矩数组（牛顿·米），可选
     * @return 操作是否成功
     */
    bool set_joint_positions(const std::vector<double>& positions,
                           const std::vector<double>& velocities = {},
                           const std::vector<double>& torques = {});

    /**
     * @brief 设置关节力矩
     * @param torques 目标力矩数组（牛顿·米）
     * @param positions 位置数组（弧度），可选
     * @param velocities 速度数组（弧度/秒），可选
     * @return 操作是否成功
     */
    bool set_joint_torques(const std::vector<double>& torques,
                          const std::vector<double>& positions = {},
                          const std::vector<double>& velocities = {});

    // ========== 高频控制接口 ==========

    /**
     * @brief 启动高频控制线程（500Hz）
     * @return 启动是否成功
     */
    bool start_high_frequency_control();

    /**
     * @brief 停止高频控制线程
     * @return 停止是否成功
     */
    bool stop_high_frequency_control();

    /**
     * @brief 检查高频控制是否正在运行
     * @return 是否正在运行
     */
    bool is_high_frequency_control_running() const { return control_thread_running_; }

    // ========== 批量操作接口 ==========

    /**
     * @brief 批量发送命令
     */
    void send_commands_all();

    /**
     * @brief 批量接收数据
     * @param timeout_us 超时时间（微秒）
     */
    void receive_data_all(int timeout_us = 500);

    // ========== 配置接口 ==========

    /**
     * @brief 设置控制频率
     * @param frequency_hz 频率（Hz）
     */
    void set_control_frequency(double frequency_hz);

    /**
     * @brief 获取控制频率
     * @return 当前控制频率（Hz）
     */
    double get_control_frequency() const { return control_frequency_; }

    // ========== 诊断接口 ==========

    /**
     * @brief 获取系统状态信息
     * @return 状态信息字典
     */
    std::map<std::string, std::string> get_system_status();

    /**
     * @brief 打印系统信息
     */
    void print_system_info();

private:
    // ========== 私有成员变量 ==========

    std::string device_sn_;                       // USB2CAN设备序列号
    bool debug_enabled_;                          // 调试输出开关
    bool initialized_;                            // 初始化标志
    std::atomic<bool> emergency_stop_flag_;       // 紧急停止标志

    // 核心组件
    std::unique_ptr<USB2CANAdapter> can_adapter_;  // CAN适配器
    std::unique_ptr<ArmComponent> arm_;            // 手臂组件
    std::unique_ptr<GripperComponent> gripper_;     // 夹爪组件
    std::unique_ptr<SafetyModule> safety_;         // 安全模块

    // 电机配置
    std::vector<MotorConfig> motor_configs_;       // 电机配置列表
    std::map<int, std::shared_ptr<BaseMotor>> motors_; // 电机映射表
    std::mutex motors_mutex_;                      // 电机映射表互斥锁

    // 高频控制
    double control_frequency_;                     // 控制频率（Hz）
    std::thread control_thread_;                   // 控制线程
    std::atomic<bool> control_thread_running_;     // 控制线程运行标志
    std::atomic<bool> stop_control_thread_{false}; // 停止控制线程标志
    std::chrono::steady_clock::time_point last_control_time_; // 上次控制时间
    int control_period_us_;                          // 控制周期（微秒）

    // 命令缓存
    std::vector<MotorCommand> command_buffer_;     // 命令缓冲区
    std::mutex command_buffer_mutex_;              // 命令缓冲区互斥锁

    // 统计信息
    std::atomic<int> error_count_{0};              // 错误计数

    // ========== 私有方法 ==========

    /**
     * @brief 初始化电机配置
     */
    void initialize_motor_configs();

    /**
     * @brief 创建电机实例
     */
    void create_motors();

    /**
     * @brief 控制线程主函数
     */
    void control_thread_main();

    /**
     * @brief 执行控制循环
     */
    void execute_control_loop();

    /**
     * @brief 处理接收到的CAN帧
     * @param can_id CAN ID
     * @param data 数据
     */
    void handle_can_frame(uint32_t can_id, const std::vector<uint8_t>& data);

    /**
     * @brief 调试输出
     * @param message 调试信息
     */
    void debug_print(const std::string& message);

    /**
     * @brief 增加错误计数
     */
    void increment_error_count();

    /**
     * @brief 错误输出
     * @param message 错误信息
     */
    void error_print(const std::string& message);

    /**
     * @brief 信息输出
     * @param message 信息
     */
    void info_print(const std::string& message);
};

} // namespace ic_can