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
#include <ic_can/motors/ht_motor.hpp>
#include <vector>

namespace ic_can {
HTMotor::HTMotor(int motor_id, uint32_t can_send_id, uint32_t can_recv_id)
    : BaseMotor(motor_id, can_send_id, can_recv_id) {}

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
   */
  HTMotor(int motor_id, uint32_t can_send_id, uint32_t can_recv_id);

  /**
   * @brief 析构函数
   */
  virtual ~HTMotor() = default;

  // ========== 基础控制接口 ==========

  bool enable() override;
  bool disable() override;
  bool set_zero() override;
  bool set_command(const MotorCommand &command) override;

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

  // ========== 状态查询接口 ==========

  void update_state() override;
  bool process_response(const std::vector<uint8_t> &data) override;
  std::vector<uint8_t> get_command_data() const override;

  /**
   * @brief 获取当前力值
   * @return 当前力 (牛顿)
   */
  double get_current_force() const { return force_.load(); }

  /**
   * @brief 获取电机温度
   * @return 温度 (摄氏度)
   */
  double get_temperature() const { return temperature_.load(); }

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

private:
  // ========== 私有成员变量 ==========

  HTControlMode control_mode_;            // 控制模式
  std::atomic<double> force_{0.0};        // 当前力值
  std::atomic<double> target_force_{0.0}; // 目标力值
  std::atomic<double> temperature_{25.0}; // 温度
  std::atomic<uint8_t> error_code_{0};    // 错误代码

  double max_force_;         // 最大力限制
  bool has_pending_command_; // 是否有待发送的命令

  // ========== 私有方法 ==========

  /**
   * @brief 创建位置控制命令
   * @param position 目标位置
   * @param velocity 最大速度
   * @return 命令数据
   */
  std::vector<uint8_t> create_position_command(double position,
                                               double velocity);

  /**
   * @brief 创建力控制命令
   * @param force 目标力
   * @return 命令数据
   */
  std::vector<uint8_t> create_force_command(double force);

  /**
   * @brief 创建速度控制命令
   * @param velocity 目标速度
   * @return 命令数据
   */
  std::vector<uint8_t> create_velocity_command(double velocity);

  /**
   * @brief 解析状态响应
   * @param data 响应数据
   * @return 解析是否成功
   */
  bool parse_status_response(const std::vector<uint8_t> &data);

  /**
   * @brief 浮点数转换为整数 (HT协议格式)
   */
  uint16_t float_to_ht_format(float value, float min_val, float max_val);

  /**
   * @brief HT格式转换为浮点数
   */
  float ht_format_to_float(uint16_t raw_value, float min_val, float max_val);
};

// HT电机控制模式
enum class HTControlMode {
  POSITION_CONTROL = 0x01, // 位置控制
  FORCE_CONTROL = 0x02,    // 力控制
  VELOCITY_CONTROL = 0x03, // 速度控制
  DISABLED = 0x00          // 禁用
};

} // namespace ic_can
