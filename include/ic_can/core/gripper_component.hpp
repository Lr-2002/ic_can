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

#include "ic_can/core/can_protocol_interface.hpp"
#include "ic_can/core/motor_protocol_base.hpp"
#include "ic_can/motors/base_motor.hpp"
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace ic_can {

/**
 * @brief 夹爪组件类 with proper dependency inversion
 *
 * 管理夹爪的1个伺服电机（m9），提供统一的夹爪控制接口
 * m9: Servo (舵机) - 实际的夹爪执行器
 *
 * 注意：手腕电机（m7-m8, HT4438）已分离到独立的WristComponent中
 * 继承自MotorProtocolBase，使用依赖反转模式接收CAN通信服务
 */
class GripperComponent : public MotorProtocolBase {
public:
  /**
   * @brief 构造函数
   */
  GripperComponent();

  /**
   * @brief 析构函数
   */
  virtual ~GripperComponent();

  // 禁用拷贝构造和赋值
  GripperComponent(const GripperComponent &) = delete;
  GripperComponent &operator=(const GripperComponent &) = delete;

  // ========== CANProtocolInterface Implementation ==========

  /**
   * @brief Process incoming CAN frame for servo motor
   * @param frame Received CAN frame
   * @return True if frame was processed successfully
   */
  bool process_can_frame(const CANFrame &frame) override;
  /**
   * @brief Read servo position
   * @return Current position (1000-2100)
   */
  uint16_t servo_read_position();

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
   * @brief Control servo position
   * @param position Target position (1000-2100)
   * @param velocity Velocity (0-100)
   * @return True if successful
   */
  bool set_position(double position);

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

  /**
   * @brief 获取夹爪状态摘要
   * @return String with state information
   */
  std::string get_state_summary() const;

  // ========== 辅助方法 ==========

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
   * @brief Read servo position (read-only, no control commands)
   * @return Current position in range 1000-2100, or 0 if failed
   */
  uint16_t read_servo_position();

  /**
   * @brief Check if servo position data is fresh
   * @param max_age_ms Maximum age in milliseconds before data is considered
   * stale
   * @return True if position data is fresh (within max_age_ms), false otherwise
   */
  bool is_position_fresh(int max_age_ms = 100) const;

  /**
   * @brief 设置夹爪开合限制
   * @param min_openness 最小开合度 (0.0-1.0)
   * @param max_openness 最大开合度 (0.0-1.0)
   * @return 设置是否成功
   */
  bool set_openness_limits(double min_openness, double max_openness);

protected:
  // ========== Helper Methods ==========

  /**
   * @brief Check if motor ID is valid for gripper component
   * @param motor_id Motor ID to check
   * @return True if valid gripper motor ID
   */
  bool is_valid_motor_id(int motor_id) const override;

  /**
   * @brief Clamp angle to limits
   * @param angle Angle to clamp
   * @param min_angle Minimum allowed angle
   * @param max_angle Maximum allowed angle
   * @return Clamped angle
   */
  double clamp_angle(double angle, double min_angle, double max_angle) const;

  /**
   * @brief Clamp velocity to safe limits
   * @param velocity Velocity to clamp
   * @return Clamped velocity
   */
  double clamp_velocity(double velocity) const;

private:
  // ========== Constants ==========

  static constexpr double DEFAULT_MIN_OPENNESS = 0.0; // Fully closed
  static constexpr double DEFAULT_MAX_OPENNESS = 1.0; // Fully open
  static constexpr double DEFAULT_MAX_SPEED = 1.0;    // Maximum speed
  static constexpr double DEFAULT_MAX_FORCE = 0.8;    // Maximum force

  // Servo motor constants
  static constexpr double SERVO_MIN_PULSE_US = 1000.0;    // 1ms pulse
  static constexpr double SERVO_MAX_PULSE_US = 2000.0;    // 2ms pulse
  static constexpr double SERVO_CENTER_PULSE_US = 1500.0; // 1.5ms center
  static constexpr double SERVO_ANGLE_RANGE = 180.0;      // 180 degrees range

  // CAN ID ranges for servo motor
  static constexpr uint32_t SERVO_MIN_CAN_ID = 0x19; // m9 receive
  static constexpr uint32_t SERVO_MAX_CAN_ID = 0x19; // m9 receive

  // ========== State Variables ==========

  std::atomic<double> current_openness_{0.5}; // Current openness (0.0-1.0)
  std::atomic<bool> is_moving_{false};        // Motion status
  std::atomic<bool> is_grasping_{false};      // Grasping status

  // Limits
  double min_openness_ = DEFAULT_MIN_OPENNESS;
  double max_openness_ = DEFAULT_MAX_OPENNESS;
  double max_speed_ = DEFAULT_MAX_SPEED;
  double max_force_ = DEFAULT_MAX_FORCE;

  // USB Communication
  int usb_fd_;           // USB file descriptor
  std::string usb_port_; // USB port path

  // ========== USB Servo Threading ==========

  struct USBServoCommand {
    enum Type {
      READ_POSITION,
      SET_POSITION,
      ENABLE_TORQUE,
      DISABLE_TORQUE
    } type;
    uint16_t position = 0;
    uint16_t velocity = 0;
    std::promise<bool> promise;

    USBServoCommand(Type cmd_type) : type(cmd_type) {}
  };

  // Thread control
  std::thread usb_thread_;
  std::atomic<bool> usb_thread_running_{false};
  std::queue<std::shared_ptr<USBServoCommand>> command_queue_;
  std::mutex command_mutex_;
  std::condition_variable command_cv_;

  // Thread state
  std::atomic<uint16_t> latest_position_{0};
  std::atomic<bool> position_valid_{false};
  std::chrono::steady_clock::time_point last_position_update_;

  // ========== Private Methods ==========

  /**
   * @brief Convert openness to servo angle
   * @param openness Openness (0.0-1.0)
   * @return Servo angle in degrees
   */
  double openness_to_angle(double openness);

  /**
   * @brief Convert servo angle to openness
   * @param angle Servo angle in degrees
   * @return Openness (0.0-1.0)
   */
  double angle_to_openness(double angle);

  /**
   * @brief Convert angle to pulse width
   * @param angle Angle in degrees
   * @return Pulse width in microseconds
   */
  double angle_to_pulse_width(double angle);

  /**
   * @brief Convert pulse width to angle
   * @param pulse_width Pulse width in microseconds
   * @return Angle in degrees
   */
  double pulse_width_to_angle(double pulse_width);

  /**
   * @brief Update gripper state from motor state
   */
  void update_gripper_state();

  /**
   * @brief Debug print function
   * @param message Debug message
   */
  void debug_print(const std::string &message);

  // ========== USB Servo Threading Methods ==========

  /**
   * @brief USB servo thread main function
   */
  void usb_servo_thread_main();

  /**
   * @brief Execute USB servo command
   * @param command Command to execute
   * @return True if successful
   */
  bool execute_usb_command(std::shared_ptr<USBServoCommand> command);

  /**
   * @brief Enqueue USB servo command
   * @param command Command to enqueue
   * @return Future for command result
   */
  std::future<bool>
  enqueue_usb_command(std::shared_ptr<USBServoCommand> command);

  /**
   * @brief Get latest servo position (thread-safe, non-blocking)
   * @return Latest position or 0 if invalid
   */
  uint16_t get_latest_position() const;

  /**
   * @brief Check if position data is fresh
   * @param max_age_ms Maximum age in milliseconds
   * @return True if position is fresh
   */
  bool is_position_fresh(uint64_t max_age_ms = 100) const;

  // ========== USB Servo Protocol Methods ==========

  /**
   * @brief Connect to USB servo
   * @return True if successful
   */
  bool usb_connect();

  /**
   * @brief Disconnect from USB servo
   * @return True if successful
   */
  bool usb_disconnect();

  /**
   * @brief Send command to USB servo
   * @param command Command bytes (without checksum)
   * @return True if successful
   */
  bool usb_send_command(const std::vector<uint8_t> &command);

  /**
   * @brief Read response from USB servo
   * @param response Response buffer
   * @param expected_size Expected response size
   * @return True if successful
   */
  bool usb_read_response(std::vector<uint8_t> &response, size_t expected_size);

  /**
   * @brief Enable servo torque
   * @return True if successful
   */
  bool servo_enable_torque();

  /**
   * @brief Disable servo torque
   * @return True if successful
   */
  bool servo_disable_torque();

  /**
   * @brief Control servo position
   * @param position Target position (1000-2100)
   * @param velocity Velocity (0-100)
   * @return True if successful
   */
  bool servo_position_control(uint16_t position, uint16_t velocity);
};

} // namespace ic_can
