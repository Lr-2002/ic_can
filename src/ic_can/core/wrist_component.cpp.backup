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

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ic_can/core/wrist_component.hpp>
#include <iomanip>
#include <iostream>
#include <thread>

namespace ic_can {

// 静态成员定义
const std::vector<int> WristComponent::WRIST_MOTOR_IDS = {7, 8};

WristComponent::WristComponent() {
  // 初始化目标位置
  target_position_.resize(2, 0.0);
}

// ========== 电机管理接口 ==========

bool WristComponent::add_motor(std::shared_ptr<BaseMotor> motor) {
  if (!motor) {
    std::cerr << "❌ WristComponent: Cannot add null motor" << std::endl;
    return false;
  }

  int motor_id = motor->get_motor_id();

  // 检查是否为手腕电机ID
  if (std::find(WRIST_MOTOR_IDS.begin(), WRIST_MOTOR_IDS.end(), motor_id) ==
      WRIST_MOTOR_IDS.end()) {
    std::cerr << "❌ WristComponent: Motor ID " << motor_id
              << " is not a wrist motor" << std::endl;
    return false;
  }

  std::lock_guard<std::mutex> lock(motors_mutex_);

  // 检查电机是否已存在
  if (motors_.find(motor_id) != motors_.end()) {
    std::cerr << "⚠️  WristComponent: Motor " << motor_id << " already exists"
              << std::endl;
    return false;
  }

  motors_[motor_id] = motor;
  std::cout << "✅ WristComponent: Added motor " << motor_id << std::endl;
  return true;
}

bool WristComponent::remove_motor(int motor_id) {
  std::lock_guard<std::mutex> lock(motors_mutex_);

  auto it = motors_.find(motor_id);
  if (it == motors_.end()) {
    std::cerr << "❌ WristComponent: Motor " << motor_id << " not found"
              << std::endl;
    return false;
  }

  motors_.erase(it);
  std::cout << "✅ WristComponent: Removed motor " << motor_id << std::endl;
  return true;
}

std::shared_ptr<BaseMotor> WristComponent::get_motor(int motor_id) const {
  std::lock_guard<std::mutex> lock(motors_mutex_);

  auto it = motors_.find(motor_id);
  if (it != motors_.end()) {
    return it->second;
  }
  return nullptr;
}

std::map<int, std::shared_ptr<BaseMotor>>
WristComponent::get_all_motors() const {
  std::lock_guard<std::mutex> lock(motors_mutex_);
  return motors_;
}

size_t WristComponent::get_motor_count() const {
  std::lock_guard<std::mutex> lock(motors_mutex_);
  return motors_.size();
}

// ========== 批量控制接口 ==========

bool WristComponent::enable_all() {
  std::lock_guard<std::mutex> lock(motors_mutex_);
  bool all_success = true;

  for (auto &[motor_id, motor] : motors_) {
    if (!motor->enable()) {
      std::cerr << "❌ WristComponent: Failed to enable motor " << motor_id
                << std::endl;
      all_success = false;
    }
  }

  if (all_success) {
    std::cout << "✅ WristComponent: All motors enabled" << std::endl;
  }
  return all_success;
}

bool WristComponent::disable_all() {
  std::lock_guard<std::mutex> lock(motors_mutex_);
  bool all_success = true;

  for (auto &[motor_id, motor] : motors_) {
    if (!motor->disable()) {
      std::cerr << "❌ WristComponent: Failed to disable motor " << motor_id
                << std::endl;
      all_success = false;
    }
  }

  if (all_success) {
    std::cout << "✅ WristComponent: All motors disabled" << std::endl;
  }
  return all_success;
}

bool WristComponent::set_zero_all() {
  std::lock_guard<std::mutex> lock(motors_mutex_);
  bool all_success = true;

  for (auto &[motor_id, motor] : motors_) {
    if (!motor->set_zero()) {
      std::cerr << "❌ WristComponent: Failed to set zero for motor "
                << motor_id << std::endl;
      all_success = false;
    }
  }

  if (all_success) {
    std::cout << "✅ WristComponent: All motors set to zero" << std::endl;
  }
  return all_success;
}

void WristComponent::update_all_states() {
  std::lock_guard<std::mutex> lock(motors_mutex_);

  for (auto &[motor_id, motor] : motors_) {
    motor->update_state();
  }

  update_wrist_state();
}

bool WristComponent::send_all_commands() {
  std::lock_guard<std::mutex> lock(motors_mutex_);
  bool all_success = true;

  for (auto &[motor_id, motor] : motors_) {
    // TODO: Implement command sending via IC_CAN system
    // For now, simulate successful sending
    std::cout << "WristComponent: Sending command to motor " << motor_id
              << std::endl;
  }

  return all_success;
}

// ========== 手腕控制接口 ==========

bool WristComponent::set_pitch_angle(double pitch_angle, double velocity) {
  auto motor7 = get_motor(7);
  if (!motor7) {
    std::cerr << "❌ WristComponent: Motor 7 (pitch) not found" << std::endl;
    return false;
  }

  // 限制角度和速度
  double clamped_angle = clamp_angle(pitch_angle, min_pitch_, max_pitch_);
  double clamped_velocity = clamp_velocity(velocity);

  target_position_[0] = clamped_angle;

  return motor7->set_position(clamped_angle, clamped_velocity);
}

bool WristComponent::set_roll_angle(double roll_angle, double velocity) {
  auto motor8 = get_motor(8);
  if (!motor8) {
    std::cerr << "❌ WristComponent: Motor 8 (roll) not found" << std::endl;
    return false;
  }

  // 限制角度和速度
  double clamped_angle = clamp_angle(roll_angle, min_roll_, max_roll_);
  double clamped_velocity = clamp_velocity(velocity);

  target_position_[1] = clamped_angle;

  return motor8->set_position(clamped_angle, clamped_velocity);
}

bool WristComponent::set_wrist_pose(double pitch_angle, double roll_angle,
                                    double velocity) {
  std::vector<double> positions = {pitch_angle, roll_angle};
  std::vector<double> velocities = {velocity, velocity};
  return set_positions(positions, velocities);
}

double WristComponent::get_pitch_angle() const {
  auto motor7 = get_motor(7);
  if (!motor7) {
    return 0.0;
  }

  auto state = motor7->get_state();
  return state.position;
}

double WristComponent::get_roll_angle() const {
  auto motor8 = get_motor(8);
  if (!motor8) {
    return 0.0;
  }

  auto state = motor8->get_state();
  return state.position;
}

// ========== 位置控制接口 ==========

bool WristComponent::set_positions(const std::vector<double> &positions,
                                   const std::vector<double> &velocities) {
  if (positions.size() != 2) {
    std::cerr << "❌ WristComponent: Invalid positions array size, expected 2"
              << std::endl;
    return false;
  }

  bool success = true;

  // 设置俯仰角 (m7)
  if (!set_pitch_angle(positions[0],
                       velocities.empty() ? max_velocity_ : velocities[0])) {
    success = false;
  }

  // 设置旋转角 (m8)
  if (!set_roll_angle(positions[1],
                      velocities.empty() ? max_velocity_ : velocities[1])) {
    success = false;
  }

  return success;
}

std::vector<double> WristComponent::get_positions() const {
  std::vector<double> positions(2, 0.0);

  auto motor7 = get_motor(7);
  if (motor7) {
    auto state = motor7->get_state();
    positions[0] = state.position;
  }

  auto motor8 = get_motor(8);
  if (motor8) {
    auto state = motor8->get_state();
    positions[1] = state.position;
  }

  return positions;
}

// ========== 速度控制接口 ==========

bool WristComponent::set_velocities(const std::vector<double> &velocities) {
  if (velocities.size() != 2) {
    std::cerr << "❌ WristComponent: Invalid velocities array size, expected 2"
              << std::endl;
    return false;
  }

  std::lock_guard<std::mutex> lock(motors_mutex_);
  bool success = true;

  for (size_t i = 0; i < velocities.size(); ++i) {
    int motor_id = WRIST_MOTOR_IDS[i];
    auto it = motors_.find(motor_id);
    if (it != motors_.end()) {
      double clamped_velocity = clamp_velocity(velocities[i]);
      if (!it->second->set_velocity(clamped_velocity)) {
        success = false;
      }
    }
  }

  return success;
}

std::vector<double> WristComponent::get_velocities() const {
  std::vector<double> velocities(2, 0.0);

  std::lock_guard<std::mutex> lock(motors_mutex_);

  for (size_t i = 0; i < WRIST_MOTOR_IDS.size(); ++i) {
    int motor_id = WRIST_MOTOR_IDS[i];
    auto it = motors_.find(motor_id);
    if (it != motors_.end()) {
      auto state = it->second->get_state();
      velocities[i] = state.velocity;
    }
  }

  return velocities;
}

// ========== 力矩控制接口 ==========

bool WristComponent::set_torques(const std::vector<double> &torques) {
  if (torques.size() != 2) {
    std::cerr << "❌ WristComponent: Invalid torques array size, expected 2"
              << std::endl;
    return false;
  }

  std::lock_guard<std::mutex> lock(motors_mutex_);
  bool success = true;

  for (size_t i = 0; i < torques.size(); ++i) {
    int motor_id = WRIST_MOTOR_IDS[i];
    auto it = motors_.find(motor_id);
    if (it != motors_.end()) {
      double clamped_torque = clamp_torque(torques[i]);
      if (!it->second->set_torque(clamped_torque)) {
        success = false;
      }
    }
  }

  return success;
}

std::vector<double> WristComponent::get_torques() {
  std::vector<double> torques(2, 0.0);

  std::lock_guard<std::mutex> lock(motors_mutex_);

  for (size_t i = 0; i < WRIST_MOTOR_IDS.size(); ++i) {
    int motor_id = WRIST_MOTOR_IDS[i];
    auto it = motors_.find(motor_id);
    if (it != motors_.end()) {
      auto state = it->second->get_state();
      torques[i] = state.torque;
    }
  }

  return torques;
}

// ========== 力控制接口 ==========

bool WristComponent::set_forces(const std::vector<double> &forces) {
  // 将力转换为力矩（简化实现）
  std::vector<double> torques(2);
  for (size_t i = 0; i < forces.size(); ++i) {
    torques[i] = forces[i] * max_torque_;
  }
  return set_torques(torques);
}

std::vector<double> WristComponent::get_forces() {
  auto torques = get_torques();
  std::vector<double> forces(2);
  for (size_t i = 0; i < torques.size(); ++i) {
    forces[i] = torques[i] / max_torque_;
  }
  return forces;
}

// ========== 状态查询接口 ==========

bool WristComponent::is_moving() const {
  auto velocities = get_velocities();
  const double velocity_threshold = 0.01; // rad/s

  for (double vel : velocities) {
    if (std::abs(vel) > velocity_threshold) {
      return true;
    }
  }
  return false;
}

bool WristComponent::at_target_position(double tolerance) const {
  auto current_pos = get_positions();

  for (size_t i = 0; i < current_pos.size(); ++i) {
    if (std::abs(current_pos[i] - target_position_[i]) > tolerance) {
      return false;
    }
  }
  return true;
}

std::vector<double> WristComponent::get_temperatures() {
  std::vector<double> temperatures(2, 0.0);

  std::lock_guard<std::mutex> lock(motors_mutex_);

  for (size_t i = 0; i < WRIST_MOTOR_IDS.size(); ++i) {
    int motor_id = WRIST_MOTOR_IDS[i];
    auto it = motors_.find(motor_id);
    if (it != motors_.end()) {
      auto state = it->second->get_state();
      temperatures[i] = state.temperature;
    }
  }

  return temperatures;
}

bool WristComponent::has_motor_errors() {
  std::lock_guard<std::mutex> lock(motors_mutex_);

  for (auto &[motor_id, motor] : motors_) {
    if (motor->has_error()) {
      return true;
    }
  }
  return false;
}

std::vector<int> WristComponent::get_error_motor_ids() {
  std::vector<int> error_ids;
  std::lock_guard<std::mutex> lock(motors_mutex_);

  for (auto &[motor_id, motor] : motors_) {
    if (motor->has_error()) {
      error_ids.push_back(motor_id);
    }
  }

  return error_ids;
}

std::map<std::string, std::vector<double>>
WristComponent::get_complete_state() {
  std::map<std::string, std::vector<double>> state;

  state["positions"] = get_positions();
  state["velocities"] = get_velocities();
  state["torques"] = get_torques();
  state["temperatures"] = get_temperatures();
  state["target_positions"] = target_position_;

  return state;
}

// ========== 安全和限制接口 ==========

bool WristComponent::set_pitch_limits(double min_pitch, double max_pitch) {
  if (min_pitch >= max_pitch) {
    std::cerr << "❌ WristComponent: Invalid pitch limits" << std::endl;
    return false;
  }

  min_pitch_ = min_pitch;
  max_pitch_ = max_pitch;
  std::cout << "✅ WristComponent: Set pitch limits [" << min_pitch_ << ", "
            << max_pitch_ << "]" << std::endl;
  return true;
}

bool WristComponent::set_roll_limits(double min_roll, double max_roll) {
  if (min_roll >= max_roll) {
    std::cerr << "❌ WristComponent: Invalid roll limits" << std::endl;
    return false;
  }

  min_roll_ = min_roll;
  max_roll_ = max_roll;
  std::cout << "✅ WristComponent: Set roll limits [" << min_roll_ << ", "
            << max_roll_ << "]" << std::endl;
  return true;
}

bool WristComponent::set_max_velocity(double max_velocity) {
  if (max_velocity <= 0) {
    std::cerr << "❌ WristComponent: Invalid max velocity" << std::endl;
    return false;
  }

  max_velocity_ = max_velocity;
  std::cout << "✅ WristComponent: Set max velocity " << max_velocity_
            << " rad/s" << std::endl;
  return true;
}

bool WristComponent::set_max_torque(double max_torque) {
  if (max_torque <= 0) {
    std::cerr << "❌ WristComponent: Invalid max torque" << std::endl;
    return false;
  }

  max_torque_ = max_torque;
  std::cout << "✅ WristComponent: Set max torque " << max_torque_ << " Nm"
            << std::endl;
  return true;
}

// ========== 高级功能接口 ==========

bool WristComponent::smooth_move_to(double target_pitch, double target_roll,
                                    double duration) {
  auto current_pos = get_positions();
  std::vector<double> start_pos = current_pos;
  std::vector<double> end_pos = {target_pitch, target_roll};

  // 生成轨迹
  int steps = static_cast<int>(duration * 100); // 100Hz
  auto trajectory = generate_trajectory(start_pos, end_pos, duration, steps);

  // 执行轨迹
  for (const auto &point : trajectory) {
    if (!set_positions(point)) {
      std::cerr << "❌ WristComponent: Failed to execute smooth move"
                << std::endl;
      return false;
    }

    if (!send_all_commands()) {
      std::cerr
          << "❌ WristComponent: Failed to send commands during smooth move"
          << std::endl;
      return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return true;
}

bool WristComponent::circular_motion(double center_pitch, double center_roll,
                                     double radius, double frequency,
                                     double duration) {
  int steps = static_cast<int>(duration * 100); // 100Hz
  double dt = 1.0 / 100.0;                      // 10ms

  for (int i = 0; i < steps; ++i) {
    double t = i * dt;
    double angle = 2.0 * M_PI * frequency * t;

    double pitch = center_pitch + radius * std::cos(angle);
    double roll = center_roll + radius * std::sin(angle);

    if (!set_wrist_pose(pitch, roll)) {
      std::cerr << "❌ WristComponent: Failed to execute circular motion"
                << std::endl;
      return false;
    }

    if (!send_all_commands()) {
      std::cerr
          << "❌ WristComponent: Failed to send commands during circular motion"
          << std::endl;
      return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return true;
}

bool WristComponent::stop_motion() {
  std::lock_guard<std::mutex> lock(motors_mutex_);
  bool success = true;

  for (auto &[motor_id, motor] : motors_) {
    if (!motor->set_velocity(0.0)) {
      std::cerr << "❌ WristComponent: Failed to stop motor " << motor_id
                << std::endl;
      success = false;
    }
  }

  is_moving_ = false;
  return success;
}

// ========== 诊断接口 ==========

void WristComponent::print_wrist_state() {
  auto state = get_complete_state();

  std::cout << "\n=== Wrist Component State ===" << std::endl;
  std::cout << "Pitch Angle: " << std::fixed << std::setprecision(3)
            << state["positions"][0] * 180.0 / M_PI << " deg" << std::endl;
  std::cout << "Roll Angle: " << std::fixed << std::setprecision(3)
            << state["positions"][1] * 180.0 / M_PI << " deg" << std::endl;
  std::cout << "Pitch Velocity: " << std::fixed << std::setprecision(3)
            << state["velocities"][0] << " rad/s" << std::endl;
  std::cout << "Roll Velocity: " << std::fixed << std::setprecision(3)
            << state["velocities"][1] << " rad/s" << std::endl;
  std::cout << "Pitch Torque: " << std::fixed << std::setprecision(3)
            << state["torques"][0] << " Nm" << std::endl;
  std::cout << "Roll Torque: " << std::fixed << std::setprecision(3)
            << state["torques"][1] << " Nm" << std::endl;
  std::cout << "Temperatures: [" << std::fixed << std::setprecision(1)
            << state["temperatures"][0] << ", " << state["temperatures"][1]
            << "] °C" << std::endl;
  std::cout << "Is Moving: " << (is_moving() ? "Yes" : "No") << std::endl;
  std::cout << "Has Errors: " << (has_motor_errors() ? "Yes" : "No")
            << std::endl;
}

bool WristComponent::test_wrist_functionality() {
  std::cout << "\n🔧 Testing wrist component functionality..." << std::endl;

  // 测试电机连接
  if (get_motor_count() != 2) {
    std::cerr << "❌ WristComponent: Expected 2 motors, found "
              << get_motor_count() << std::endl;
    return false;
  }

  // 测试电机使能
  if (!enable_all()) {
    std::cerr << "❌ WristComponent: Failed to enable all motors" << std::endl;
    return false;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // 测试位置控制
  double test_pitch = 0.5; // ~28.6 degrees
  double test_roll = -0.3; // ~-17.2 degrees

  if (!set_wrist_pose(test_pitch, test_roll)) {
    std::cerr << "❌ WristComponent: Failed to set test pose" << std::endl;
    return false;
  }

  // 等待运动完成
  if (!wait_for_motion_complete(0.01, 3.0)) {
    std::cerr << "❌ WristComponent: Failed to reach target position"
              << std::endl;
    return false;
  }

  // 测试零位
  if (!set_wrist_pose(0.0, 0.0)) {
    std::cerr << "❌ WristComponent: Failed to return to zero position"
              << std::endl;
    return false;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  std::cout << "✅ WristComponent: All tests passed" << std::endl;
  return true;
}

bool WristComponent::calibrate_wrist() {
  std::cout << "\n🔧 Calibrating wrist..." << std::endl;

  if (!set_zero_all()) {
    std::cerr << "❌ WristComponent: Failed to set zero positions" << std::endl;
    return false;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  target_position_ = {0.0, 0.0};

  std::cout << "✅ WristComponent: Calibration completed" << std::endl;
  return true;
}

// ========== 私有方法 ==========

double WristComponent::clamp_angle(double angle, double min_angle,
                                   double max_angle) {
  return std::max(min_angle, std::min(max_angle, angle));
}

double WristComponent::clamp_velocity(double velocity) {
  return std::max(-max_velocity_, std::min(max_velocity_, std::abs(velocity))) *
         (velocity >= 0 ? 1 : -1);
}

double WristComponent::clamp_torque(double torque) {
  return std::max(-max_torque_, std::min(max_torque_, std::abs(torque))) *
         (torque >= 0 ? 1 : -1);
}

void WristComponent::update_wrist_state() { is_moving_ = is_moving(); }

bool WristComponent::check_motion_complete() { return at_target_position(); }

bool WristComponent::wait_for_motion_complete(double tolerance,
                                              double timeout) {
  auto start_time = std::chrono::steady_clock::now();

  while (std::chrono::steady_clock::now() - start_time <
         std::chrono::duration<double>(timeout)) {
    update_all_states();

    if (check_motion_complete()) {
      return true;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  std::cerr << "⚠️  WristComponent: Motion timeout after " << timeout
            << " seconds" << std::endl;
  return false;
}

std::vector<std::vector<double>>
WristComponent::generate_trajectory(const std::vector<double> &start_pos,
                                    const std::vector<double> &end_pos,
                                    double duration, int steps) {

  std::vector<std::vector<double>> trajectory;
  trajectory.reserve(steps);

  for (int i = 0; i <= steps; ++i) {
    double alpha = static_cast<double>(i) / steps;

    // 使用平滑插值（三次样条）
    double smooth_alpha = alpha * alpha * (3.0 - 2.0 * alpha);

    std::vector<double> point(2);
    point[0] = start_pos[0] + smooth_alpha * (end_pos[0] - start_pos[0]);
    point[1] = start_pos[1] + smooth_alpha * (end_pos[1] - start_pos[1]);

    trajectory.push_back(point);
  }

  return trajectory;
}

} // namespace ic_can
