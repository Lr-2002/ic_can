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

#include "ic_can/core/motor_protocol_base.hpp"
#include "ic_can/core/can_protocol_interface.hpp"
#include "ic_can/motors/base_motor.hpp"
#include <vector>
#include <memory>
#include <mutex>
#include <map>
#include <atomic>
#include <array>
#include <cmath>

namespace ic_can {

/**
 * @brief 手臂组件类 with proper dependency inversion
 *
 * 管理机械臂的6个DM电机（m1-m6），提供统一的手臂控制接口
 * m1: DM 10010L
 * m2: DM 6248
 * m3: DM 6248
 * m4: DM 4340
 * m5: DM 4340
 * m6: DM 4310
 *
 * 继承自MotorProtocolBase，使用依赖反转模式接收CAN通信服务
 */
class ArmComponent : public MotorProtocolBase {
public:
    /**
     * @brief 构造函数
     */
    ArmComponent();

    /**
     * @brief 析构函数
     */
    virtual ~ArmComponent() = default;

    // 禁用拷贝构造和赋值
    ArmComponent(const ArmComponent&) = delete;
    ArmComponent& operator=(const ArmComponent&) = delete;

    // ========== CANProtocolInterface Implementation ==========

    /**
     * @brief Process incoming CAN frame for DM motors
     * @param frame Received CAN frame
     * @return True if frame was processed successfully
     */
    bool process_can_frame(const CANFrame& frame) override;

    // ========== 批量控制接口 ==========

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
     * @brief 设置所有电机为零位
     * @return 操作是否成功
     */
    bool set_zero_all();

    /**
     * @brief 停止所有关节运动
     * @return 操作是否成功
     */
    bool stop_all_motion();

    // ========== 位置控制接口 ==========

    /**
     * @brief 设置所有关节位置
     * @param positions 目标位置数组（弧度）
     * @param velocities 目标速度数组（弧度/秒），可选
     * @return 操作是否成功
     */
    bool set_positions(const std::vector<double>& positions,
                      const std::vector<double>& velocities = {});

    /**
     * @brief 设置单个关节位置
     * @param joint_index 关节索引 (0-5)，对应电机ID (1-6)
     * @param position 目标位置（弧度）
     * @param velocity 目标速度（弧度/秒）
     * @return 操作是否成功
     */
    bool set_single_position(int joint_index, double position, double velocity = 0.0);

    /**
     * @brief 获取所有关节位置
     * @return 位置数组（弧度）
     */
    std::vector<double> get_positions();

    /**
     * @brief 获取单个关节位置
     * @param joint_index 关节索引 (0-5)
     * @return 关节位置（弧度）
     */
    double get_single_position(int joint_index);

    // ========== 速度控制接口 ==========

    /**
     * @brief 设置所有关节速度
     * @param velocities 目标速度数组（弧度/秒）
     * @return 操作是否成功
     */
    bool set_velocities(const std::vector<double>& velocities);

    /**
     * @brief 获取所有关节速度
     * @return 速度数组（弧度/秒）
     */
    std::vector<double> get_velocities();

    // ========== 力矩控制接口 ==========

    /**
     * @brief 设置所有关节力矩
     * @param torques 目标力矩数组（牛顿·米）
     * @return 操作是否成功
     */
    bool set_torques(const std::vector<double>& torques);

    /**
     * @brief 获取所有关节力矩
     * @return 力矩数组（牛顿·米）
     */
    std::vector<double> get_torques();

    // ========== MIT控制接口 ==========

    /**
     * @brief MIT控制所有关节
     * @param positions 目标位置数组（弧度）
     * @param velocities 目标速度数组（弧度/秒）
     * @param torques 目标力矩数组（牛顿·米）
     * @param kp_array 位置增益数组
     * @param kd_array 速度增益数组
     * @return 操作是否成功
     */
    bool mit_control(const std::vector<double>& positions,
                    const std::vector<double>& velocities,
                    const std::vector<double>& torques,
                    const std::vector<double>& kp_array,
                    const std::vector<double>& kd_array);

    // ========== 运动控制接口 ==========

    /**
     * @brief 回到零位
     * @param speed 回零速度（弧度/秒）
     * @param timeout 超时时间（秒）
     * @return 操作是否成功
     */
    bool home_to_zero(double speed = 0.5, double timeout = 30.0);

    // ========== 状态查询接口 ==========

    /**
     * @brief 获取所有电机温度
     * @return 温度数组（摄氏度）
     */
    std::vector<double> get_temperatures();

    /**
     * @brief 检查是否有电机错误
     * @return 是否有错误
     */
    bool has_motor_errors();

    /**
     * @brief 获取有错误的电机ID列表
     * @return 错误电机ID列表
     */
    std::vector<int> get_error_motor_ids();

    /**
     * @brief 获取手臂状态摘要
     * @return String with state information
     */
    std::string get_state_summary() const;

    // ========== 辅助方法 ==========

    /**
     * @brief 检查是否所有电机都已使能
     * @return 是否全部使能
     */
    bool is_fully_enabled() const;

    /**
     * @brief 检查是否有电机在运动
     * @return 是否有运动
     */
    bool is_any_motor_moving() const;

    /**
     * @brief 获取完整状态信息
     * @return 状态信息字典
     */
    std::map<std::string, std::vector<double>> get_complete_state();

    // ========== 诊断接口 ==========

    /**
     * @brief 打印手臂状态信息
     */
    void print_arm_state();

    /**
     * @brief 测试手臂功能
     * @return 测试是否通过
     */
    bool test_arm_functionality();

protected:
    // ========== Helper Methods ==========

    /**
     * @brief Check if motor ID is valid for arm component
     * @param motor_id Motor ID to check
     * @return True if valid arm motor ID
     */
    bool is_valid_motor_id(int motor_id) const override;

    /**
     * @brief Clamp position to safe limits for specific joint
     * @param position Position to clamp
     * @param joint_index Joint index (0-5)
     * @return Clamped position
     */
    double clamp_position(double position, int joint_index) const;

    /**
     * @brief Clamp velocity to safe limits
     * @param velocity Velocity to clamp
     * @return Clamped velocity
     */
    double clamp_velocity(double velocity) const;

    /**
     * @brief Clamp torque to safe limits
     * @param torque Torque to clamp
     * @return Clamped torque
     */
    double clamp_torque(double torque) const;

private:
    // ========== Constants ==========

    static constexpr size_t NUM_JOINTS = 6;           // Number of arm joints
    static constexpr double DEFAULT_MAX_VELOCITY = 2.0;  // rad/s
    static constexpr double DEFAULT_MAX_TORQUE = 10.0;   // Nm

    // CAN ID ranges for DM motors (receive IDs)
    static constexpr uint32_t DM_MIN_CAN_ID = 0x11;  // m1 receive
    static constexpr uint32_t DM_MAX_CAN_ID = 0x16;  // m6 receive

    // Joint position limits (radians) - conservative defaults
    static constexpr std::array<double, NUM_JOINTS> MIN_POSITIONS = {
        -M_PI,     // Joint 1
        -M_PI/2,   // Joint 2
        -M_PI/2,   // Joint 3
        -M_PI,     // Joint 4
        -M_PI/2,   // Joint 5
        -M_PI      // Joint 6
    };

    static constexpr std::array<double, NUM_JOINTS> MAX_POSITIONS = {
        M_PI,      // Joint 1
        M_PI/2,    // Joint 2
        M_PI/2,    // Joint 3
        M_PI,      // Joint 4
        M_PI/2,    // Joint 5
        M_PI       // Joint 6
    };

    // Predefined joint to motor ID mapping
    static constexpr std::array<int, NUM_JOINTS> JOINT_MOTOR_IDS = {1, 2, 3, 4, 5, 6};

    // ========== State Variables ==========

    std::atomic<bool> is_homing_{false};       // Homing status
    double max_velocity_ = DEFAULT_MAX_VELOCITY;  // Maximum allowed velocity
    double max_torque_ = DEFAULT_MAX_TORQUE;      // Maximum allowed torque

    // ========== Private Methods ==========

    /**
     * @brief Validate joint index
     * @param joint_index Joint index (0-5)
     * @return True if valid
     */
    bool is_valid_joint_index(int joint_index) const;

    /**
     * @brief Validate array length
     * @param array Input array
     * @param expected_length Expected length
     * @return True if length matches
     */
    bool is_valid_array_length(const std::vector<double>& array, size_t expected_length) const;

    /**
     * @brief Convert joint index to motor ID
     * @param joint_index Joint index (0-5)
     * @return Motor ID (1-6)
     */
    int joint_to_motor_id(int joint_index) const;

    /**
     * @brief Convert motor ID to joint index
     * @param motor_id Motor ID (1-6)
     * @return Joint index (0-5)
     */
    int motor_to_joint_index(int motor_id) const;

    /**
     * @brief Update arm component state
     */
    void update_arm_state();

    /**
     * @brief Debug print function
     * @param message Debug message
     */
    void debug_print(const std::string& message);

    /**
     * @brief Wait for motion completion
     * @param target_positions Target positions
     * @param tolerance Position tolerance
     * @param timeout Timeout in seconds
     * @return True if motion completed successfully
     */
    bool wait_for_motion_complete(const std::vector<double>& target_positions,
                                 double tolerance = 0.01, double timeout = 10.0);
};

} // namespace ic_can