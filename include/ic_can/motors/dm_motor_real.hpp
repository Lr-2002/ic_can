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
#include <vector>
#include <array>
#include <chrono>

namespace ic_can {

/**
 * @brief Real Damiao Motor Implementation
 *
 * This class implements real communication with Damiao motors using the MIT protocol.
 * It replaces simulation with actual CAN communication based on the openarm repository.
 */
class DMMotorReal : public BaseMotor {
public:
    /**
     * @brief Constructor for DMMotorReal
     * @param motor_id Motor ID (1-9)
     * @param can_send_id CAN send ID
     * @param can_recv_id CAN receive ID
     * @param kp Position gain
     * @param kd Velocity gain
     * @param max_torque Maximum torque
     * @param debug Enable debug output
     */
    DMMotorReal(int motor_id, uint32_t can_send_id, uint32_t can_recv_id,
                double kp = 50.0, double kd = 1.0, double max_torque = 10.0,
                bool debug = false);

    /**
     * @brief Destructor
     */
    virtual ~DMMotorReal() = default;

    // ========== BaseMotor Interface Implementation ==========

    bool enable() override;
    bool disable() override;
    bool set_zero() override;
    virtual bool is_enabled() const;

    bool set_position(double position, double velocity = 0.0, double torque = 0.0) override;
    bool set_velocity(double velocity, double torque = 0.0) override;
    bool set_torque(double torque) override;
    bool mit_control(double position, double velocity, double torque, double kp, double kd) override;
    bool set_command(const MotorCommand& command) override;

    virtual double get_position() const;
    virtual double get_velocity() const;
    virtual double get_torque() const;
    virtual double get_temperature() const;

    void update_state() override;

    // Additional methods from BaseMotor (non-virtual, regular implementations)
    void set_limits(const MotorLimits& limits);
    void set_max_position(double max_position);
    void set_min_position(double min_position);
    void set_max_velocity(double max_velocity);
    void set_max_torque(double max_torque);
    bool is_position_safe(double position) const;
    bool is_velocity_safe(double velocity) const;
    bool is_torque_safe(double torque) const;
    bool is_temperature_safe(double temperature) const;
    MotorState get_state() const;
    void reset_error();

    virtual int get_motor_id() const;
    virtual uint32_t get_can_send_id() const;
    virtual uint32_t get_can_recv_id() const;

    std::vector<uint8_t> get_command_data() const override;
    bool process_response(const std::vector<uint8_t>& data) override;

    // ========== Damiao Motor Specific Methods ==========

    /**
     * @brief Set MIT control parameters
     * @param kp Position gain
     * @param kd Velocity gain
     * @param max_torque Maximum torque
     */
    void set_mit_params(double kp, double kd, double max_torque);

    /**
     * @brief Enter MIT control mode
     */
    void enter_mit_mode();

    /**
     * @brief Exit MIT control mode
     */
    void exit_mit_mode();

    /**
     * @brief Zero position setting
     */
    void set_zero_position();

private:
    // ========== Motor Configuration ==========
    int motor_id_;                ///< Motor ID
    uint32_t can_send_id_;        ///< CAN send ID
    uint32_t can_recv_id_;        ///< CAN receive ID

    // Control parameters
    double kp_;                   ///< Position gain
    double kd_;                   ///< Velocity gain
    double max_torque_;           ///< Maximum torque

    // State variables
    double position_;             ///< Current position (rad)
    double velocity_;             ///< Current velocity (rad/s)
    double torque_;               ///< Current torque (Nm)
    double temperature_;          ///< Current temperature (°C)

    // Control limits
    double position_limit_min_;   ///< Minimum position limit
    double position_limit_max_;   ///< Maximum position limit
    double velocity_limit_max_;   ///< Maximum velocity limit
    double torque_limit_max_;     ///< Maximum torque limit

    // Control state
    bool enabled_;                ///< Motor enabled flag
    bool debug_enabled_;          ///< Debug output flag

    // Command data
    std::vector<uint8_t> command_data_;  ///< Current command data

    // ========== Private Methods ==========

    /**
     * @brief Pack MIT command into CAN data
     * @param position Target position
     * @param velocity Target velocity
     * @param torque Target torque
     * @return Packed CAN data
     */
    std::vector<uint8_t> pack_mit_command(double position, double velocity, double torque);

    /**
     * @brief Unpack MIT response from CAN data
     * @param data CAN response data
     */
    void unpack_mit_response(const std::vector<uint8_t>& data);

    /**
     * @brief Float to uint32 conversion with bit manipulation
     * @param value Float value
     * @return Uint32 representation
     */
    uint32_t float_to_uint32(float value);

    /**
     * @brief Uint32 to float conversion with bit manipulation
     * @param value Uint32 value
     * @return Float representation
     */
    float uint32_to_float(uint32_t value);

    /**
     * @brief Debug output
     * @param message Debug message
     */
    void debug_print(const std::string& message);
};

} // namespace ic_can