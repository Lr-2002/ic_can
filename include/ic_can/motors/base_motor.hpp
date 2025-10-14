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

#include <memory>
#include <vector>
#include <chrono>
#include <functional>
#include <cmath>

namespace ic_can {

/**
 * @brief Motor type enumeration
 */
enum class MotorType {
    DM_DAMIAO,
    HT_HIGH_TORQUE,
    SERVO
};

/**
 * @brief Motor command structure
 */
struct MotorCommand {
    bool use_position = false;
    bool use_velocity = false;
    bool use_torque = false;
    double position = 0.0;
    double velocity = 0.0;
    double torque = 0.0;
    double kp = 0.0;
    double kd = 0.0;
};

/**
 * @brief Motor limits structure
 */
struct MotorLimits {
    double min_position = -12.5;
    double max_position = 12.5;
    double max_velocity = 10.0;
    double max_torque = 10.0;
    double max_temperature = 80.0;
};

/**
 * @brief Motor state structure
 */
struct MotorState {
    double position = 0.0;
    double velocity = 0.0;
    double torque = 0.0;
    double temperature = 25.0;
    bool enabled = false;
    bool error = false;
    std::chrono::steady_clock::time_point timestamp;
};

/**
 * @brief Base motor interface
 *
 * Abstract base class for all motor types (Damiao, HT, Servo)
 */
class BaseMotor {
public:
    /**
     * @brief Constructor
     * @param motor_id Motor ID (1-9)
     * @param type Motor type
     * @param can_send_id CAN send ID
     * @param can_recv_id CAN receive ID
     */
    BaseMotor(int motor_id, MotorType type, uint32_t can_send_id, uint32_t can_recv_id);

    /**
     * @brief Destructor
     */
    virtual ~BaseMotor() = default;

    // Motor control interface
    virtual bool enable() = 0;
    virtual bool disable() = 0;
    virtual bool set_zero() = 0;
    virtual bool is_enabled() const = 0;

    // Position/Velocity/Torque control
    virtual bool set_position(double position, double velocity = 0.0, double torque = 0.0) = 0;
    virtual bool set_velocity(double velocity, double torque = 0.0) = 0;
    virtual bool set_torque(double torque) = 0;
    virtual bool mit_control(double position, double velocity, double torque, double kp, double kd) = 0;
    virtual bool set_command(const MotorCommand& command) = 0;

    // State feedback
    virtual double get_position() const = 0;
    virtual double get_velocity() const = 0;
    virtual double get_torque() const = 0;
    virtual double get_temperature() const = 0;
    virtual MotorState get_state() const = 0;
    virtual void update_state() = 0;

    // Limits and safety
    virtual void set_limits(const MotorLimits& limits);
    virtual void set_max_position(double max_position);
    virtual void set_min_position(double min_position);
    virtual void set_max_velocity(double max_velocity);
    virtual void set_max_torque(double max_torque);
    virtual bool is_position_safe(double position) const;
    virtual bool is_velocity_safe(double velocity) const;
    virtual bool is_torque_safe(double torque) const;
    virtual bool is_temperature_safe(double temperature) const;

    // Error handling
    virtual void reset_error() = 0;

    // Motor information
    virtual int get_motor_id() const;
    virtual MotorType get_motor_type() const;
    virtual uint32_t get_can_send_id() const;
    virtual uint32_t get_can_recv_id() const;
    virtual std::vector<uint8_t> get_command_data() const;
    virtual bool process_response(const std::vector<uint8_t>& data) = 0;

protected:
    int motor_id_;
    MotorType motor_type_;
    uint32_t can_send_id_;
    uint32_t can_recv_id_;

    // Motor limits
    double position_limit_min_;
    double position_limit_max_;
    double velocity_limit_max_;
    double torque_limit_max_;

    // Command data
    std::vector<uint8_t> command_data_;
};

} // namespace ic_can