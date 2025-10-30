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

#include <vector>
#include <memory>
#include <mutex>
#include <map>
#include "../motors/base_motor.hpp"
#include "../protocols/usb2can_adapter.hpp"

namespace ic_can {

/**
 * @brief 手臂组件类
 *
 * 管理机械臂的6个电机（m1-m6），提供统一的控制接口
 * m1: DM10010L
 * m2: DM6248
 * m3: DM6248
 * m4: DM4340
 * m5: DM4340
 * m6: DM4310
 */
class ArmComponent {
public:
    /**
     * @brief 构造函数
     * @param can_adapter CAN适配器引用
     */
    explicit ArmComponent(USB2CANAdapter& can_adapter);

    /**
     * @brief 析构函数
     */
    ~ArmComponent() = default;

    // 禁用拷贝构造和赋值
    ArmComponent(const ArmComponent&) = delete;
    ArmComponent& operator=(const ArmComponent&) = delete;

    // ========== 电机管理接口 ==========

    /**
     * @brief 添加电机到组件
     * @param motor 电机智能指针
     * @return 添加是否成功
     */
    bool add_motor(std::shared_ptr<BaseMotor> motor);

    /**
     * @brief 移除电机
     * @param motor_id 电机ID
     * @return 移除是否成功
     */
    bool remove_motor(int motor_id);

    /**
     * @brief 获取电机
     * @param motor_id 电机ID
     * @return 电机智能指针
     */
    std::shared_ptr<BaseMotor> get_motor(int motor_id);

    /**
     * @brief 获取所有电机
     * @return 电机映射表
     */
    std::map<int, std::shared_ptr<BaseMotor>> get_all_motors() const;

    /**
     * @brief 获取电机数量
     * @return 电机数量
     */
    size_t get_motor_count() const;

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
     * @brief 更新所有电机状态
     */
    void update_all_states();

    /**
     * @brief 发送所有电机命令
     * @return 操作是否成功
     */
    bool send_all_commands();

    // ========== 位置控制接口 ==========

    /**
     * @brief 设置所有关节位置
     * @param positions 目标位置数组（弧度）
     * @param velocities 目标速度数组（弧度/秒），可选
     * @param torques 目标力矩数组（牛顿·米），可选
     * @return 操作是否成功
     */
    bool set_positions(const std::vector<double>& positions,
                      const std::vector<double>& velocities = {},
                      const std::vector<double>& torques = {});

    /**
     * @brief 设置单个关节位置
     * @param joint_index 关节索引 (0-5)
     * @param position 目标位置（弧度）
     * @param velocity 目标速度（弧度/秒）
     * @param torque 目标力矩（牛顿·米）
     * @return 操作是否成功
     */
    bool set_single_position(int joint_index, double position,
                            double velocity = 0.0, double torque = 0.0);

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
     * @param torques 目标力矩数组（牛顿·米），可选
     * @return 操作是否成功
     */
    bool set_velocities(const std::vector<double>& velocities,
                       const std::vector<double>& torques = {});

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

    /**
     * @brief 停止所有关节运动
     * @return 操作是否成功
     */
    bool stop_all_motion();

    // ========== 安全检查接口 ==========

    /**
     * @brief 检查位置限制
     * @param positions 位置数组
     * @return 是否都在限制范围内
     */
    bool check_position_limits(const std::vector<double>& positions);

    /**
     * @brief 检查速度限制
     * @param velocities 速度数组
     * @return 是否都在限制范围内
     */
    bool check_velocity_limits(const std::vector<double>& velocities);

    /**
     * @brief 检查力矩限制
     * @param torques 力矩数组
     * @return 是否都在限制范围内
     */
    bool check_torque_limits(const std::vector<double>& torques);

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
     * @brief 获取完整状态信息
     * @return 状态信息字典
     */
    std::map<std::string, std::vector<double>> get_complete_state();

    // ========== 配置接口 ==========

    /**
     * @brief 设置关节增益
     * @param joint_index 关节索引 (0-5)
     * @param kp 位置增益
     * @param kd 速度增益
     * @return 设置是否成功
     */
    bool set_joint_gains(int joint_index, double kp, double kd);

    /**
     * @brief 设置所有关节增益
     * @param kp_array 位置增益数组
     * @param kd_array 速度增益数组
     * @return 设置是否成功
     */
    bool set_all_gains(const std::vector<double>& kp_array,
                      const std::vector<double>& kd_array);

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

private:
    // ========== 私有成员变量 ==========

    USB2CANAdapter& can_adapter_;              // CAN适配器引用
    std::map<int, std::shared_ptr<BaseMotor>> motors_; // 电机映射表
    mutable std::mutex motors_mutex_;           // 电机映射表互斥锁

    // 预定义的关节电机ID映射
    static const std::vector<int> ARM_MOTOR_IDS; // [1, 2, 3, 4, 5, 6]

    // ========== 私有方法 ==========

    /**
     * @brief 验证关节索引
     * @param joint_index 关节索引
     * @return 是否有效
     */
    bool is_valid_joint_index(int joint_index) const;

    /**
     * @brief 验证数组长度
     * @param array 数组
     * @param expected_length 期望长度
     * @return 长度是否正确
     */
    bool is_valid_array_length(const std::vector<double>& array,
                               size_t expected_length) const;

    /**
     * @brief 限制位置到安全范围
     * @param position 原始位置
     * @param joint_index 关节索引
     * @return 限制后的位置
     */
    double clamp_position(double position, int joint_index);

    /**
     * @brief 限制速度到安全范围
     * @param velocity 原始速度
     * @param joint_index 关节索引
     * @return 限制后的速度
     */
    double clamp_velocity(double velocity, int joint_index);

    /**
     * @brief 限制力矩到安全范围
     * @param torque 原始力矩
     * @param joint_index 关节索引
     * @return 限制后的力矩
     */
    double clamp_torque(double torque, int joint_index);

    /**
     * @brief 执行平滑轨迹
     * @param start_positions 起始位置
     * @param end_positions 结束位置
     * @param duration 持续时间（秒）
     * @param frequency 控制频率（Hz）
     * @return 执行是否成功
     */
    bool execute_smooth_trajectory(const std::vector<double>& start_positions,
                                  const std::vector<double>& end_positions,
                                  double duration, double frequency = 100.0);

    /**
     * @brief 等待运动完成
     * @param target_positions 目标位置
     * @param tolerance 误差容限（弧度）
     * @param timeout 超时时间（秒）
     * @return 是否成功到达目标
     */
    bool wait_for_motion_complete(const std::vector<double>& target_positions,
                                 double tolerance = 0.01, double timeout = 10.0);
};

} // namespace ic_can