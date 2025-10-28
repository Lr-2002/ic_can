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
#include <atomic>
#include "../motors/base_motor.hpp"

namespace ic_can {

/**
 * @brief 手腕组件类
 *
 * 管理手腕的2个高扭矩电机（m7-m8），提供统一的手腕控制接口
 * m7: HT4438 (高扭矩电机 - 手腕俯仰)
 * m8: HT4438 (高扭矩电机 - 手腕旋转)
 */
class WristComponent {
public:
    /**
     * @brief 构造函数
     */
    WristComponent();

    /**
     * @brief 析构函数
     */
    ~WristComponent() = default;

    // 禁用拷贝构造和赋值
    WristComponent(const WristComponent&) = delete;
    WristComponent& operator=(const WristComponent&) = delete;

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
    std::shared_ptr<BaseMotor> get_motor(int motor_id) const;

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
     * @brief 使能所有手腕电机
     * @return 操作是否成功
     */
    bool enable_all();

    /**
     * @brief 禁用所有手腕电机
     * @return 操作是否成功
     */
    bool disable_all();

    /**
     * @brief 设置所有手腕电机为零位
     * @return 操作是否成功
     */
    bool set_zero_all();

    /**
     * @brief 更新所有手腕电机状态
     */
    void update_all_states();

    /**
     * @brief 发送所有手腕电机命令
     * @return 操作是否成功
     */
    bool send_all_commands();

    // ========== 手腕控制接口 ==========

    /**
     * @brief 设置手腕俯仰角度（m7电机）
     * @param pitch_angle 目标俯仰角度（弧度）
     * @param velocity 速度限制（弧度/秒）
     * @return 操作是否成功
     */
    bool set_pitch_angle(double pitch_angle, double velocity = 1.0);

    /**
     * @brief 设置手腕旋转角度（m8电机）
     * @param roll_angle 目标旋转角度（弧度）
     * @param velocity 速度限制（弧度/秒）
     * @return 操作是否成功
     */
    bool set_roll_angle(double roll_angle, double velocity = 1.0);

    /**
     * @brief 同时设置俯仰和旋转角度
     * @param pitch_angle 目标俯仰角度（弧度）
     * @param roll_angle 目标旋转角度（弧度）
     * @param velocity 速度限制（弧度/秒）
     * @return 操作是否成功
     */
    bool set_wrist_pose(double pitch_angle, double roll_angle, double velocity = 1.0);

    /**
     * @brief 获取当前俯仰角度
     * @return 当前俯仰角度（弧度）
     */
    double get_pitch_angle() const;

    /**
     * @brief 获取当前旋转角度
     * @return 当前旋转角度（弧度）
     */
    double get_roll_angle() const;

    // ========== 位置控制接口 ==========

    /**
     * @brief 设置手腕位置（弧度）
     * @param positions 目标位置数组 [俯仰, 旋转]（弧度）
     * @param velocities 目标速度数组（弧度/秒）
     * @return 操作是否成功
     */
    bool set_positions(const std::vector<double>& positions,
                      const std::vector<double>& velocities = {});

    /**
     * @brief 获取手腕位置（弧度）
     * @return 位置数组 [俯仰, 旋转]（弧度）
     */
    std::vector<double> get_positions() const;

    // ========== 速度控制接口 ==========

    /**
     * @brief 设置手腕速度（弧度/秒）
     * @param velocities 目标速度数组 [俯仰速度, 旋转速度]（弧度/秒）
     * @return 操作是否成功
     */
    bool set_velocities(const std::vector<double>& velocities);

    /**
     * @brief 获取手腕速度（弧度/秒）
     * @return 速度数组 [俯仰速度, 旋转速度]（弧度/秒）
     */
    std::vector<double> get_velocities() const;

    // ========== 力矩控制接口 ==========

    /**
     * @brief 设置手腕力矩（牛顿·米）
     * @param torques 目标力矩数组 [俯仰力矩, 旋转力矩]（牛顿·米）
     * @return 操作是否成功
     */
    bool set_torques(const std::vector<double>& torques);

    /**
     * @brief 获取手腕力矩（牛顿·米）
     * @return 力矩数组 [俯仰力矩, 旋转力矩]（牛顿·米）
     */
    std::vector<double> get_torques();

    // ========== 力控制接口 ==========

    /**
     * @brief 设置手腕关节力
     * @param forces 目标力数组 [俯仰力, 旋转力]（相对值 0.0-1.0）
     * @return 操作是否成功
     */
    bool set_forces(const std::vector<double>& forces);

    /**
     * @brief 获取当前关节力
     * @return 当前力数组 [俯仰力, 旋转力]（相对值 0.0-1.0）
     */
    std::vector<double> get_forces();

    // ========== 状态查询接口 ==========

    /**
     * @brief 检查手腕是否在运动
     * @return 是否在运动
     */
    bool is_moving() const;

    /**
     * @brief 检查手腕是否在目标位置
     * @param tolerance 位置容差（弧度）
     * @return 是否在目标位置
     */
    bool at_target_position(double tolerance = 0.01) const;

    /**
     * @brief 获取所有电机温度
     * @return 温度数组 [俯仰电机温度, 旋转电机温度]（摄氏度）
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
     * @return 状态映射表
     */
    std::map<std::string, std::vector<double>> get_complete_state();

    // ========== 安全和限制接口 ==========

    /**
     * @brief 设置俯仰角度限制
     * @param min_pitch 最小俯仰角度（弧度）
     * @param max_pitch 最大俯仰角度（弧度）
     * @return 设置是否成功
     */
    bool set_pitch_limits(double min_pitch, double max_pitch);

    /**
     * @brief 设置旋转角度限制
     * @param min_roll 最小旋转角度（弧度）
     * @param max_roll 最大旋转角度（弧度）
     * @return 设置是否成功
     */
    bool set_roll_limits(double min_roll, double max_roll);

    /**
     * @brief 设置最大速度限制
     * @param max_velocity 最大速度（弧度/秒）
     * @return 设置是否成功
     */
    bool set_max_velocity(double max_velocity);

    /**
     * @brief 设置最大力矩限制
     * @param max_torque 最大力矩（牛顿·米）
     * @return 设置是否成功
     */
    bool set_max_torque(double max_torque);

    // ========== 高级功能接口 ==========

    /**
     * @brief 执行平滑运动到目标位置
     * @param target_pitch 目标俯仰角度（弧度）
     * @param target_roll 目标旋转角度（弧度）
     * @param duration 运动时间（秒）
     * @return 操作是否成功
     */
    bool smooth_move_to(double target_pitch, double target_roll, double duration);

    /**
     * @brief 执行圆形轨迹运动
     * @param center_pitch_center 圆心俯仰角度（弧度）
     * @param center_roll_center 圆心旋转角度（弧度）
     * @param radius 运动半径（弧度）
     * @param frequency 运动频率（Hz）
     * @param duration 运动时间（秒）
     * @return 操作是否成功
     */
    bool circular_motion(double center_pitch, double center_roll, double radius,
                        double frequency, double duration);

    /**
     * @brief 停止所有手腕运动
     * @return 操作是否成功
     */
    bool stop_motion();

    // ========== 诊断接口 ==========

    /**
     * @brief 打印手腕状态信息
     */
    void print_wrist_state();

    /**
     * @brief 测试手腕功能
     * @return 测试是否通过
     */
    bool test_wrist_functionality();

    /**
     * @brief 校准手腕零位
     * @return 校准是否成功
     */
    bool calibrate_wrist();

private:
    // ========== 私有成员变量 ==========

    std::map<int, std::shared_ptr<BaseMotor>> motors_; // 电机映射表
    mutable std::mutex motors_mutex_;           // 电机映射表互斥锁

    // 手腕状态
    std::atomic<bool> is_moving_{false};        // 运动状态
    std::vector<double> target_position_{0.0, 0.0}; // 目标位置

    // 限制参数
    double min_pitch_ = -3.14159;               // 最小俯仰角度（-180度）
    double max_pitch_ = 3.14159;                // 最大俯仰角度（180度）
    double min_roll_ = -3.14159;                // 最小旋转角度（-180度）
    double max_roll_ = 3.14159;                 // 最大旋转角度（180度）
    double max_velocity_ = 2.0;                 // 最大速度（弧度/秒）
    double max_torque_ = 20.0;                  // 最大力矩（牛顿·米）

    // 最后的命令状态
    MotorCommand last_command_;

    // 预定义的手腕电机ID
    static const std::vector<int> WRIST_MOTOR_IDS; // [7, 8]

    // ========== 私有方法 ==========

    /**
     * @brief 限制角度到安全范围
     * @param angle 原始角度
     * @param min_angle 最小角度
     * @param max_angle 最大角度
     * @return 限制后的角度
     */
    double clamp_angle(double angle, double min_angle, double max_angle);

    /**
     * @brief 限制速度到安全范围
     * @param velocity 原始速度
     * @return 限制后的速度
     */
    double clamp_velocity(double velocity);

    /**
     * @brief 限制力矩到安全范围
     * @param torque 原始力矩
     * @return 限制后的力矩
     */
    double clamp_torque(double torque);

    /**
     * @brief 更新手腕状态
     */
    void update_wrist_state();

    /**
     * @brief 检查运动是否完成
     * @return 是否到达目标位置
     */
    bool check_motion_complete();

    /**
     * @brief 等待运动完成
     * @param tolerance 误差容限
     * @param timeout 超时时间
     * @return 是否成功到达目标
     */
    bool wait_for_motion_complete(double tolerance = 0.01, double timeout = 5.0);

    /**
     * @brief 生成插值轨迹点
     * @param start_pos 起始位置
     * @param end_pos 结束位置
     * @param duration 运动时间
     * @param steps 插值步数
     * @return 轨迹点数组
     */
    std::vector<std::vector<double>> generate_trajectory(
        const std::vector<double>& start_pos,
        const std::vector<double>& end_pos,
        double duration, int steps);
};

} // namespace ic_can