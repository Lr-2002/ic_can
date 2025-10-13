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
 * @brief 夹爪组件类
 *
 * 管理夹爪的3个电机（m7-m9），提供统一的夹爪控制接口
 * m7: HT4438 (高扭矩电机)
 * m8: HT4438 (高扭矩电机)
 * m9: Servo (舵机)
 */
class GripperComponent {
public:
    /**
     * @brief 构造函数
     * @param can_adapter CAN适配器引用
     */
    explicit GripperComponent(USB2CANAdapter& can_adapter);

    /**
     * @brief 析构函数
     */
    ~GripperComponent() = default;

    // 禁用拷贝构造和赋值
    GripperComponent(const GripperComponent&) = delete;
    GripperComponent& operator=(const GripperComponent&) = delete;

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

    // ========== 夹爪控制接口 ==========

    /**
     * @brief 打开夹爪
     * @param speed 速度 (0.0-1.0)
     * @param force 力度 (0.0-1.0)
     * @return 操作是否成功
     */
    bool open(double speed = 0.5, double force = 0.3);

    /**
     * @brief 关闭夹爪
     * @param speed 速度 (0.0-1.0)
     * @param force 力度 (0.0-1.0)
     * @return 操作是否成功
     */
    bool close(double speed = 0.5, double force = 0.5);

    /**
     * @brief 停止夹爪运动
     * @return 操作是否成功
     */
    bool stop();

    /**
     * @brief 设置夹爪开合度
     * @param openness 开合度 (0.0-1.0, 0.0为完全关闭, 1.0为完全打开)
     * @param speed 速度 (0.0-1.0)
     * @param force 力度 (0.0-1.0)
     * @return 操作是否成功
     */
    bool set_openness(double openness, double speed = 0.5, double force = 0.3);

    /**
     * @brief 获取当前开合度
     * @return 开合度 (0.0-1.0)
     */
    double get_openness() const;

    // ========== 力控制接口 ==========

    /**
     * @brief 设置夹爪力
     * @param force 目标力 (0.0-1.0)
     * @return 操作是否成功
     */
    bool set_force(double force);

    /**
     * @brief 获取当前力
     * @return 当前力 (0.0-1.0)
     */
    double get_current_force() const;

    /**
     * @brief 力控制模式
     * @param target_force 目标力 (0.0-1.0)
     * @return 操作是否成功
     */
    bool force_control(double target_force);

    // ========== 位置控制接口 ==========

    /**
     * @brief 设置夹爪位置（弧度）
     * @param positions 目标位置数组（弧度）
     * @param velocities 目标速度数组（弧度/秒）
     * @return 操作是否成功
     */
    bool set_positions(const std::vector<double>& positions,
                      const std::vector<double>& velocities = {});

    /**
     * @brief 获取夹爪位置（弧度）
     * @return 位置数组（弧度）
     */
    std::vector<double> get_positions();

    // ========== 速度控制接口 ==========

    /**
     * @brief 设置夹爪速度（弧度/秒）
     * @param velocities 目标速度数组（弧度/秒）
     * @return 操作是否成功
     */
    bool set_velocities(const std::vector<double>& velocities);

    /**
     * @brief 获取夹爪速度（弧度/秒）
     * @return 速度数组（弧度/秒）
     */
    std::vector<double> get_velocities();

    // ========== 力矩控制接口 ==========

    /**
     * @brief 设置夹爪力矩（牛顿·米）
     * @param torques 目标力矩数组（牛顿·米）
     * @return 操作是否成功
     */
    bool set_torques(const std::vector<double>& torques);

    /**
     * @brief 获取夹爪力矩（牛顿·米）
     * @return 力矩数组（牛顿·米）
     */
    std::vector<double> get_torques();

    // ========== 舵机专用接口 ==========

    /**
     * @brief 设置舵机角度（度）
     * @param angle 目标角度（度）
     * @param speed 速度 (0.0-1.0)
     * @return 操作是否成功
     */
    bool set_servo_angle(double angle, double speed = 0.5);

    /**
     * @brief 获取舵机当前角度（度）
     * @return 当前角度（度）
     */
    double get_servo_angle() const;

    /**
     * @brief 设置舵机脉宽
     * @param pulse_width_us 脉宽（微秒）
     * @return 操作是否成功
     */
    bool set_servo_pulse_width(uint16_t pulse_width_us);

    // ========== 状态查询接口 ==========

    /**
     * @brief 检查夹爪是否完全打开
     * @return 是否完全打开
     */
    bool is_fully_open() const;

    /**
     * @brief 检查夹爪是否完全关闭
     * @return 是否完全关闭
     */
    bool is_fully_closed() const;

    /**
     * @brief 检查夹爪是否在运动
     * @return 是否在运动
     */
    bool is_moving() const;

    /**
     * @brief 检查是否有物体被夹持
     * @return 是否有物体被夹持
     */
    bool is_grasping() const;

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
     * @return 状态映射表
     */
    std::map<std::string, std::vector<double>> get_complete_state();

    // ========== 安全和限制接口 ==========

    /**
     * @brief 设置夹爪开合限制
     * @param min_openness 最小开合度 (0.0-1.0)
     * @param max_openness 最大开合度 (0.0-1.0)
     * @return 设置是否成功
     */
    bool set_openness_limits(double min_openness, double max_openness);

    /**
     * @brief 设置最大力限制
     * @param max_force 最大力 (0.0-1.0)
     * @return 设置是否成功
     */
    bool set_max_force(double max_force);

    /**
     * @brief 设置最大速度限制
     * @param max_speed 最大速度 (0.0-1.0)
     * @return 设置是否成功
     */
    bool set_max_speed(double max_speed);

    // ========== 高级功能接口 ==========

    /**
     * @brief 执行抓取序列
     * @param target_openness 目标开合度 (0.0-1.0)
     * @param approach_speed 接近速度 (0.0-1.0)
     * @param grasp_force 抓取力 (0.0-1.0)
     * @param timeout 超时时间（秒）
     * @return 抓取是否成功
     */
    bool perform_grasp_sequence(double target_openness, double approach_speed,
                               double grasp_force, double timeout = 5.0);

    /**
     * @brief 释放物体
     * @param release_speed 释放速度 (0.0-1.0)
     * @param final_openness 最终开合度 (0.0-1.0)
     * @return 释放是否成功
     */
    bool release_object(double release_speed = 0.8, double final_openness = 1.0);

    /**
     * @brief 自动抓取（基于力反馈）
     * @param max_force 最大抓取力 (0.0-1.0)
     * @param speed 抓取速度 (0.0-1.0)
     * @param timeout 超时时间（秒）
     * @return 抓取是否成功
     */
    bool auto_grasp(double max_force = 0.6, double speed = 0.4, double timeout = 3.0);

    // ========== 诊断接口 ==========

    /**
     * @brief 打印夹爪状态信息
     */
    void print_gripper_state();

    /**
     * @brief 测试夹爪功能
     * @return 测试是否通过
     */
    bool test_gripper_functionality();

    /**
     * @brief 校准夹爪
     * @return 校准是否成功
     */
    bool calibrate_gripper();

private:
    // ========== 私有成员变量 ==========

    USB2CANAdapter& can_adapter_;              // CAN适配器引用
    std::map<int, std::shared_ptr<BaseMotor>> motors_; // 电机映射表
    mutable std::mutex motors_mutex_;           // 电机映射表互斥锁

    // 夹爪状态
    std::atomic<double> current_openness_{0.5}; // 当前开合度
    std::atomic<bool> is_moving_{false};        // 运动状态
    std::atomic<bool> is_grasping_{false};      // 抓取状态

    // 限制参数
    double min_openness_ = 0.0;                 // 最小开合度
    double max_openness_ = 1.0;                 // 最大开合度
    double max_force_ = 0.8;                    // 最大力
    double max_speed_ = 1.0;                    // 最大速度

    // 最后的命令状态
    MotorCommand last_command_;

    // 预定义的夹爪电机ID映射
    static const std::vector<int> GRIPPER_MOTOR_IDS; // [7, 8, 9]

    // ========== 私有方法 ==========

    /**
     * @brief 将开合度转换为电机位置
     * @param openness 开合度 (0.0-1.0)
     * @return 电机位置数组（弧度）
     */
    std::vector<double> openness_to_positions(double openness);

    /**
     * @brief 将电机位置转换为开合度
     * @param positions 电机位置数组（弧度）
     * @return 开合度 (0.0-1.0)
     */
    double positions_to_openness(const std::vector<double>& positions);

    /**
     * @brief 限制开合度到安全范围
     * @param openness 原始开合度
     * @return 限制后的开合度
     */
    double clamp_openness(double openness);

    /**
     * @brief 限制力到安全范围
     * @param force 原始力
     * @return 限制后的力
     */
    double clamp_force(double force);

    /**
     * @brief 限制速度到安全范围
     * @param speed 原始速度
     * @return 限制后的速度
     */
    double clamp_speed(double speed);

    /**
     * @brief 更新夹爪状态
     */
    void update_gripper_state();

    /**
     * @brief 检测抓取状态
     * @return 是否正在抓取物体
     */
    bool detect_grasp_state();

    /**
     * @brief 等待运动完成
     * @param target_positions 目标位置
     * @param tolerance 误差容限
     * @param timeout 超时时间
     * @return 是否成功到达目标
     */
    bool wait_for_motion_complete(const std::vector<double>& target_positions,
                                 double tolerance = 0.01, double timeout = 5.0);
};

} // namespace ic_can