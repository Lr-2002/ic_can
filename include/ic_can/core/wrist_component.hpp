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
#include <cmath>

namespace ic_can {

/**
 * @brief Wrist Component with proper dependency inversion
 *
 * This wrist component inherits from MotorProtocolBase and receives CAN communication
 * services from the upper layer instead of creating its own communication infrastructure.
 * It manages 2 HT motors (m7-m8) for wrist pitch and roll control.
 *
 * HT Motor ID mapping:
 * - m7 (0x700): Pitch control
 * - m8 (0x800): Roll control
 */
class WristComponent : public MotorProtocolBase {
public:
    /**
     * @brief Constructor
     */
    WristComponent();

    /**
     * @brief Destructor
     */
    virtual ~WristComponent() = default;

    // Disable copy constructor and assignment
    WristComponent(const WristComponent&) = delete;
    WristComponent& operator=(const WristComponent&) = delete;

    // ========== CANProtocolInterface Implementation ==========

    /**
     * @brief Process incoming CAN frame for HT motors
     * @param frame Received CAN frame
     * @return True if frame was processed successfully
     */
    bool process_can_frame(const CANFrame& frame) override;

    // ========== Wrist-Specific Control Interface ==========

    /**
     * @brief Set wrist pitch angle (motor m7)
     * @param pitch_angle Target pitch angle in radians
     * @param velocity Maximum velocity in rad/s
     * @return True if command set successfully
     */
    bool set_pitch_angle(double pitch_angle, double velocity = 2.0);

    /**
     * @brief Set wrist roll angle (motor m8)
     * @param roll_angle Target roll angle in radians
     * @param velocity Maximum velocity in rad/s
     * @return True if command set successfully
     */
    bool set_roll_angle(double roll_angle, double velocity = 2.0);

    /**
     * @brief Set both pitch and roll angles simultaneously
     * @param pitch_angle Target pitch angle in radians
     * @param roll_angle Target roll angle in radians
     * @param velocity Maximum velocity in rad/s
     * @return True if commands set successfully
     */
    bool set_wrist_pose(double pitch_angle, double roll_angle, double velocity = 2.0);

    /**
     * @brief Get current pitch angle
     * @return Pitch angle in radians
     */
    double get_pitch_angle() const;

    /**
     * @brief Get current roll angle
     * @return Roll angle in radians
     */
    double get_roll_angle() const;

    /**
     * @brief Get current wrist pose
     * @return Vector [pitch, roll] in radians
     */
    std::vector<double> get_wrist_pose() const;

    /**
     * @brief Get all motor positions (ordered by motor ID)
     * @return Vector of positions in radians [m7, m8]
     */
    std::vector<double> get_positions() const;

    /**
     * @brief Get all motor velocities
     * @return Vector of velocities in rad/s [m7, m8]
     */
    std::vector<double> get_velocities() const;

    /**
     * @brief Get all motor torques
     * @return Vector of torques in Nm [m7, m8]
     */
    std::vector<double> get_torques() const;

    /**
     * @brief Get all motor temperatures
     * @return Vector of temperatures in Celsius [m7, m8]
     */
    std::vector<double> get_temperatures() const;

    /**
     * @brief Check if wrist has reached target position
     * @param tolerance Position tolerance in radians
     * @return True if at target position
     */
    bool at_target_position(double tolerance = 0.01) const;

    /**
     * @brief Check if any motors have errors
     * @return True if there are motor errors
     */
    bool has_motor_errors() const;

    /**
     * @brief Get IDs of motors with errors
     * @return Vector of motor IDs with errors
     */
    std::vector<int> get_error_motor_ids() const;

    // ========== HT Motor Specific Methods ==========

    /**
     * @brief Set HT motor control parameters
     * @param motor_id Motor ID (7 or 8)
     * @param kp Position gain
     * @param kd Derivative gain
     * @param max_torque Maximum torque in Nm
     * @return True if parameters set successfully
     */
    bool set_ht_motor_params(int motor_id, double kp, double kd, double max_torque);

    /**
     * @brief Send MIT control command to HT motor
     * @param motor_id Motor ID (7 or 8)
     * @param position Target position in radians
     * @param velocity Target velocity in rad/s
     * @param torque Feedforward torque in Nm
     * @param kp Position gain
     * @param kd Derivative gain
     * @return True if command sent successfully
     */
    bool send_mit_command(int motor_id, double position, double velocity,
                         double torque, double kp, double kd);

    // ========== Configuration ==========

    /**
     * @brief Set pitch angle limits
     * @param min_angle Minimum pitch angle in radians
     * @param max_angle Maximum pitch angle in radians
     */
    void set_pitch_limits(double min_angle, double max_angle);

    /**
     * @brief Set roll angle limits
     * @param min_angle Minimum roll angle in radians
     * @param max_angle Maximum roll angle in radians
     */
    void set_roll_limits(double min_angle, double max_angle);

    /**
     * @brief Get pitch limits
     * @return Pair [min_angle, max_angle] in radians
     */
    std::pair<double, double> get_pitch_limits() const;

    /**
     * @brief Get roll limits
     * @return Pair [min_angle, max_angle] in radians
     */
    std::pair<double, double> get_roll_limits() const;

    /**
     * @brief Get wrist component state summary
     * @return String with state information
     */
    std::string get_state_summary() const;

protected:
    // ========== Helper Methods ==========

    /**
     * @brief Check if motor ID is valid for wrist component
     * @param motor_id Motor ID to check
     * @return True if valid wrist motor ID
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

    /**
     * @brief Update internal wrist state from motor states
     */
    void update_wrist_state();

private:
    // Wrist-specific configuration
    static constexpr double DEFAULT_PITCH_MIN = -M_PI;      // -180 degrees
    static constexpr double DEFAULT_PITCH_MAX = M_PI;       // 180 degrees
    static constexpr double DEFAULT_ROLL_MIN = -M_PI;       // -180 degrees
    static constexpr double DEFAULT_ROLL_MAX = M_PI;        // 180 degrees
    static constexpr double DEFAULT_MAX_VELOCITY = 5.0;     // rad/s
    static constexpr double DEFAULT_POSITION_TOLERANCE = 0.01; // rad

    // HT Motor CAN ID ranges (based on Python implementation)
    static constexpr uint32_t HT_MIN_CAN_ID = 0x700;
    static constexpr uint32_t HT_MAX_CAN_ID = 0x8FF;

    // State variables
    std::vector<double> target_position_;    // [pitch, roll] in radians
    mutable std::mutex state_mutex_;         // Protect state variables

    // Angle limits
    double pitch_min_, pitch_max_;
    double roll_min_, roll_max_;
    double max_velocity_;

    // Status flags
    mutable std::mutex status_mutex_;
    bool initialized_;
    std::string last_error_;
};

/**
 * @brief Factory function to create wrist component
 */
std::unique_ptr<WristComponent> create_wrist_component();

} // namespace ic_can