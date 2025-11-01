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
#include <vector>

namespace ic_can {

/**
 * @brief Servo motor control modes
 */
enum class ServoControlMode {
    POSITION_PWM = 0,   // Position control via PWM
    VELOCITY_PWM = 1,   // Velocity control via PWM ramping
    TORQUE_PWM = 2      // Torque control via PWM modulation
};

/**
 * @brief Servo motor commands
 */
enum class ServoCommand : uint8_t {
    ENABLE = 0xFC,      // Enable servo
    DISABLE = 0xFD,     // Disable servo
    SET_ZERO = 0xFE,    // Set zero position
    SET_POSITION = 0xA0, // Set position command
    REFRESH = 0xFF      // Refresh status
};

/**
 * @brief Servo motor class
 *
 * Implements servo motor control with PWM-based position control.
 * Supports standard servo PWM protocol (1000-2000μs pulse width)
 * for precise position control typically used in gripper applications.
 */
class ServoMotor : public BaseMotor {
public:
    /**
     * @brief Constructor
     * @param motor_id Motor ID (should be 9 for gripper servo)
     * @param can_send_id CAN send ID (0x19 for motor 9)
     * @param can_recv_id CAN receive ID (0x19 for motor 9)
     * @param min_pwm_us Minimum PWM pulse width in microseconds (default: 1000)
     * @param max_pwm_us Maximum PWM pulse width in microseconds (default: 2000)
     */
    ServoMotor(int motor_id, uint32_t can_send_id, uint32_t can_recv_id,
               double min_pwm_us = 1000.0, double max_pwm_us = 2000.0);

    /**
     * @brief Destructor
     */
    virtual ~ServoMotor() = default;

    // ========== Basic Control Interface ==========

    /**
     * @brief Enable servo motor
     * @return Operation success
     */
    bool enable() override;

    /**
     * @brief Disable servo motor
     * @return Operation success
     */
    bool disable() override;

    /**
     * @brief Set zero position
     * @return Operation success
     */
    bool set_zero() override;

    /**
     * @brief Check if motor is enabled
     * @return True if enabled
     */
    bool is_enabled() const override;

    /**
     * @brief Set motor command
     * @param command Motor command
     * @return Operation success
     */
    bool set_command(const MotorCommand& command) override;

    // ========== Position/Velocity/Torque Control ==========

    /**
     * @brief Set position command
     * @param position Target position (0.0 to 1.0, normalized)
     * @param velocity Target velocity (not used in basic servo mode)
     * @param torque Target torque (not used in basic servo mode)
     * @return Operation success
     */
    bool set_position(double position, double velocity = 0.0, double torque = 0.0) override;

    /**
     * @brief Set velocity command
     * @param velocity Target velocity (ramping support)
     * @param torque Target torque (not used)
     * @return Operation success
     */
    bool set_velocity(double velocity, double torque = 0.0) override;

    /**
     * @brief Set torque command
     * @param torque Target torque (PWM-based torque simulation)
     * @return Operation success
     */
    bool set_torque(double torque) override;

    /**
     * @brief MIT control (simplified for servo)
     * @param position Target position
     * @param velocity Target velocity
     * @param torque Target torque
     * @param kp Position gain (PWM scaling)
     * @param kd Velocity gain (ramping speed)
     * @return Operation success
     */
    bool mit_control(double position, double velocity, double torque,
                   double kp, double kd) override;

    // ========== Servo-Specific Methods ==========

    /**
     * @brief Set PWM pulse width directly
     * @param pwm_us PWM pulse width in microseconds (1000-2000)
     * @return Operation success
     */
    bool set_pwm_pulse_width(double pwm_us);

    /**
     * @brief Set angle in degrees
     * @param angle_deg Angle in degrees (typically 0-180 for standard servos)
     * @return Operation success
     */
    bool set_angle_degrees(double angle_degrees);

    /**
     * @brief Get current PWM pulse width
     * @return PWM pulse width in microseconds
     */
    double get_pwm_pulse_width() const;

    /**
     * @brief Get current angle in degrees
     * @return Angle in degrees
     */
    double get_angle_degrees() const;

    /**
     * @brief Set PWM limits
     * @param min_pwm_us Minimum PWM (microseconds)
     * @param max_pwm_us Maximum PWM (microseconds)
     */
    void set_pwm_limits(double min_pwm_us, double max_pwm_us);

    /**
     * @brief Enable/disable velocity ramping
     * @param enable Enable ramping
     * @param ramp_time_ms Ramping time in milliseconds
     */
    void enable_velocity_ramping(bool enable, double ramp_time_ms = 100.0);

    // ========== State and Feedback ==========

    /**
     * @brief Get current position
     * @return Position (0.0 to 1.0 normalized)
     */
    double get_position() const override;

    /**
     * @brief Get current velocity
     * @return Velocity (normalized units)
     */
    double get_velocity() const override;

    /**
     * @brief Get current torque
     * @return Torque (normalized units)
     */
    double get_torque() const override;

    /**
     * @brief Get temperature
     * @return Temperature (simulated or estimated)
     */
    double get_temperature() const override;

    /**
     * @brief Get motor state
     * @return Complete motor state
     */
    MotorState get_state() const override;

    /**
     * @brief Update motor state
     */
    void update_state() override;

    /**
     * @brief Process received CAN data
     * @param data Received data
     * @return Processing success
     */
    bool process_response(const std::vector<uint8_t>& data) override;

    /**
     * @brief Get command data for CAN transmission
     * @return Command data
     */
    std::vector<uint8_t> get_command_data() const override;

    // ========== Configuration ==========

    /**
     * @brief Set control mode
     * @param mode Control mode
     * @return Operation success
     */
    bool set_control_mode(ServoControlMode mode);

    /**
     * @brief Get control mode
     * @return Current control mode
     */
    ServoControlMode get_control_mode() const { return control_mode_; }

    /**
     * @brief Reset error state
     */
    void reset_error() override;

    // ========== Static Utility Methods ==========

    /**
     * @brief Convert normalized position to PWM pulse width
     * @param position Normalized position (0.0 to 1.0)
     * @param min_pwm_us Minimum PWM
     * @param max_pwm_us Maximum PWM
     * @return PWM pulse width in microseconds
     */
    static double position_to_pwm(double position, double min_pwm_us = 1000.0, double max_pwm_us = 2000.0);

    /**
     * @brief Convert PWM pulse width to normalized position
     * @param pwm_us PWM pulse width in microseconds
     * @param min_pwm_us Minimum PWM
     * @param max_pwm_us Maximum PWM
     * @return Normalized position (0.0 to 1.0)
     */
    static double pwm_to_position(double pwm_us, double min_pwm_us = 1000.0, double max_pwm_us = 2000.0);

    /**
     * @brief Convert degrees to normalized position
     * @param angle_degrees Angle in degrees (0-180)
     * @return Normalized position (0.0 to 1.0)
     */
    static double degrees_to_position(double angle_degrees);

    /**
     * @brief Convert normalized position to degrees
     * @param position Normalized position (0.0 to 1.0)
     * @return Angle in degrees (0-180)
     */
    static double position_to_degrees(double position);

private:
    // ========== Member Variables ==========

    // Servo parameters
    double min_pwm_us_;           // Minimum PWM pulse width (μs)
    double max_pwm_us_;           // Maximum PWM pulse width (μs)
    ServoControlMode control_mode_; // Current control mode

    // Current state
    double current_pwm_us_;       // Current PWM pulse width
    double target_pwm_us_;        // Target PWM pulse width
    double current_velocity_;     // Current velocity (for ramping)
    double pwm_ramp_rate_;        // PWM ramping rate (μs/ms)

    // Velocity ramping
    bool velocity_ramping_enabled_; // Enable velocity ramping
    double ramp_time_ms_;         // Time for full-range ramp (ms)

    // Command state
    bool has_pending_command_;    // Whether there's a pending command
    std::vector<uint8_t> command_buffer_; // Command data buffer

    // ========== Private Methods ==========

    /**
     * @brief Create enable command
     * @return Command data
     */
    std::vector<uint8_t> create_enable_command();

    /**
     * @brief Create disable command
     * @return Command data
     */
    std::vector<uint8_t> create_disable_command();

    /**
     * @brief Create set zero command
     * @return Command data
     */
    std::vector<uint8_t> create_set_zero_command();

    /**
     * @brief Create set position command
     * @param position Target position (0.0 to 1.0)
     * @return Command data
     */
    std::vector<uint8_t> create_set_position_command(double position);

    /**
     * @brief Create refresh status command
     * @return Command data
     */
    std::vector<uint8_t> create_refresh_command();

    /**
     * @brief Update PWM with velocity ramping
     * @param dt_ms Time delta in milliseconds
     */
    void update_pwm_with_ramping(double dt_ms);

    /**
     * @brief Clamp PWM value to valid range
     * @param pwm_us PWM value
     * @return Clamped PWM value
     */
    double clamp_pwm(double pwm_us);

    /**
     * @brief Get last update timestamp for ramping
     * @return Timestamp
     */
    std::chrono::steady_clock::time_point get_last_update_time() const;

    /**
     * @brief Set last update timestamp
     */
    void set_last_update_time();

    // Timing for velocity ramping
    mutable std::chrono::steady_clock::time_point last_update_time_;
    mutable std::mutex timing_mutex_;
};

} // namespace ic_can