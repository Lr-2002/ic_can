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
#include <cstring>

namespace ic_can {

#pragma pack(push, 1)
/**
 * @brief HT Motor Frame Structure (48 bytes total)
 *
 * New communication protocol for HT motors with structured frame format
 * Motor 7: Send ID 0x8007, Receive ID 0x700
 * Motor 8: Send ID 0x8008, Receive ID 0x800
 */
struct HTMotorFrame {
  // Header (4 bytes)
  uint8_t head[4] = {0x01, 0x00, 0x0A, 0x0F};

  // Mode and control parameters (1 byte)
  uint8_t mode = 0x20;

  // Control parameters (12 bytes)
  float pos = 0.0f;    // Position in revolutions
  float vel = 0.0f;    // Velocity in rev/s
  float torque = 0.0f; // Torque in Nm

  // Magic bytes (2 bytes)
  uint8_t magic[2] = {0x0E, 0x2B};

  // Gains (8 bytes)
  float kp = 50.0f; // Position gain
  float kd = 0.8f;  // Velocity gain

  // Tail (6 bytes)
  uint8_t tail[2] = {17, 01};

  // Padding to reach 48 bytes total (15 bytes)
  uint8_t pad[3];

  /**
   * @brief Constructor - initializes padding
   */
  HTMotorFrame() { std::memset(pad, 0x50, sizeof(pad)); }

  /**
   * @brief Set motor control parameters
   * @param position Position in revolutions
   * @param velocity Velocity in rev/s
   * @param torque Torque in Nm
   * @param kp Position gain
   * @param kd Velocity gain
   */
  void set_control_params(float position, float velocity, float torque,
                          float kp_gain, float kd_gain) {
    pos = position;
    vel = velocity;
    torque = torque;
    kp = kp_gain;
    kd = kd_gain;
  }

  /**
   * @brief Convert radians to revolutions for position
   * @param position_rad Position in radians
   * @return Position in revolutions
   */
  static float rad_to_rev(float position_rad) {
    return position_rad / (2.0f * 3.14159265f);
  }

  /**
   * @brief Convert rev/s to rad/s for velocity
   * @param vel_rev Velocity in rev/s
   * @return Velocity in rad/s
   */
  static float rev_to_rad(float vel_rev) {
    return vel_rev * 2.0f * 3.14159265f;
  }

  /**
   * @brief Get raw frame data as byte array
   * @return Pointer to frame data
   */
  const uint8_t *get_data() const {
    return reinterpret_cast<const uint8_t *>(this);
  }

  /**
   * @brief Get frame size
   * @return Frame size in bytes (always 48)
   */
  static constexpr size_t get_size() { return sizeof(HTMotorFrame); }
};
#pragma pack(pop)

/**
 * @brief HT Motor CAN ID mappings
 */
struct HTMotorCANIDs {
  // Motor 7 (pitch)
  static constexpr uint32_t MOTOR_7_SEND = 0x8007;
  static constexpr uint32_t MOTOR_7_RECV = 0x700;

  // Motor 8 (roll)
  static constexpr uint32_t MOTOR_8_SEND = 0x8008;
  static constexpr uint32_t MOTOR_8_RECV = 0x800;
};

} // namespace ic_can
