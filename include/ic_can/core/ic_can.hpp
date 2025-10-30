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
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <functional>

namespace ic_can {

// Forward declarations
class BaseMotor;
class ArmComponent;
class GripperComponent;
class WristComponent;
class SafetyModule;
class CANFrameDispatcher;
class USB2CANCommunicationAdapter;

/**
 * @brief Main IC_CAN controller class
 *
 * Provides high-level interface for controlling the IC_ARM system
 * with 9 motors (6 arm joints + 2 wrist + 1 gripper servo) at 500Hz.
 */
class IC_CAN {
public:
    /**
     * @brief Constructor
     * @param device_sn USB2CAN device serial number
     * @param debug Enable debug output
     */
    explicit IC_CAN(const std::string& device_sn, bool debug = false);

    /**
     * @brief Destructor
     */
    ~IC_CAN();

    /**
     * @brief Initialize the IC_CAN system
     * @return true if successful
     */
    bool initialize();

    /**
     * @brief Shutdown the system
     */
    void shutdown();

    /**
     * @brief Enable all motors
     * @return true if successful
     */
    bool enable_all();

    /**
     * @brief Disable all motors
     * @return true if successful
     */
    bool disable_all();

    /**
     * @brief Set all motors to zero position
     * @return true if successful
     */
    bool set_zero_all();

    /**
     * @brief Set motor zero position calibration
     * @param motor_id Motor ID (1-9)
     * @return true if successful
     */
    bool set_motor_zero_calibration(int motor_id);

    /**
     * @brief Refresh all motor states
     * @return true if successful
     */
    bool refresh_all();

    /**
     * @brief Get joint positions for all motors
     * @return Vector of 9 joint positions in radians
     */
    std::vector<double> get_joint_positions();

    /**
     * @brief Get joint velocities for all motors
     * @return Vector of 9 joint velocities in rad/s
     */
    std::vector<double> get_joint_velocities();

    /**
     * @brief Get joint torques for all motors
     * @return Vector of 9 joint torques in Nm
     */
    std::vector<double> get_joint_torques();

    /**
     * @brief Set joint positions for all motors
     * @param positions Vector of target positions (radians)
     * @param velocities Optional velocity limits (rad/s)
     * @param torques Optional torque limits (Nm)
     * @return true if successful
     */
    bool set_joint_positions(const std::vector<double>& positions,
                            const std::vector<double>& velocities = {},
                            const std::vector<double>& torques = {});

    /**
     * @brief Set joint velocities for all motors
     * @param velocities Vector of target velocities (rad/s)
     * @param torques Optional torque limits (Nm)
     * @return true if successful
     */
    bool set_joint_velocities(const std::vector<double>& velocities,
                             const std::vector<double>& torques = {});

    /**
     * @brief Set joint torques for all motors
     * @param torques Vector of target torques (Nm)
     * @return true if successful
     */
    bool set_joint_torques(const std::vector<double>& torques);

    /**
     * @brief Send HT motor MIT command for position request only (no movement)
     * @param position Target position in radians (ignored, for interface compatibility)
     * @param velocity Target velocity in rad/s (ignored, for interface compatibility)
     * @param torque Target torque in Nm (ignored, for interface compatibility)
     * @param kp Position gain (ignored, for interface compatibility)
     * @param kd Velocity gain (ignored, for interface compatibility)
     */
    void send_ht_mit_command(double position, double velocity, double torque,
                             double kp, double kd);

    /**
     * @brief Send HT motor MIT command for actual movement
     * @param m7_position Target position for motor 7 in radians
     * @param m7_velocity Target velocity for motor 7 in rad/s
     * @param m7_torque Target torque for motor 7 in Nm
     * @param m8_position Target position for motor 8 in radians
     * @param m8_velocity Target velocity for motor 8 in rad/s
     * @param m8_torque Target torque for motor 8 in Nm
     * @param kp Position gain
     * @param kd Velocity gain
     */
    void send_ht_mit_command_with_movement(double m7_position, double m7_velocity, double m7_torque,
                                          double m8_position, double m8_velocity, double m8_torque,
                                          double kp, double kd);

    /**
     * @brief Send HT motor SET ZERO command
     * Sets the current position as zero position for HT motors
     */
    void send_ht_set_zero_command();

    /**
     * @brief Send HT motor BRAKE command
     * Applies brake to HT motors
     */
    void send_ht_brake_command();


    /**
     * @brief Send HT motor read state command
     * Reads current state of both HT motors (7 and 8)
     */
    void send_ht_read_state();


    /**
     * @brief Start control loop with configurable frequency
     * @param frequency Control frequency in Hz (e.g., 200, 500)
     * @return true if successful
     */
    bool start_control_loop(double frequency);

    /**
     * @brief Start high-frequency control loop (500Hz) - deprecated
     * @return true if successful
     */
    bool start_high_frequency_control();

    /**
     * @brief Stop control loop
     */
    void stop_control_loop();

    /**
     * @brief Stop high-frequency control loop - deprecated
     */
    void stop_high_frequency_control();

    /**
     * @brief Check if control loop is running
     * @return true if running
     */
    bool is_control_running() const;

    /**
     * @brief Check if high-frequency control is running - deprecated
     * @return true if running
     */
    bool is_hf_control_running() const;

    /**
     * @brief Set target positions with velocity-limited interpolation
     * @param target_positions Target positions for all joints
     * @param max_velocity Maximum velocity for interpolation (rad/s)
     */
    void set_target_positions_interpolated(const std::vector<double>& target_positions,
                                           double max_velocity = 0.2);

    /**
     * @brief Interpolate positions with velocity limiting
     * @param current_positions Current positions
     * @param target_positions Target positions
     * @param dt Time step (seconds)
     * @param max_velocity Maximum velocity (rad/s)
     * @return Interpolated positions
     */
    static std::vector<double> interpolate_positions(const std::vector<double>& current_positions,
                                                      const std::vector<double>& target_positions,
                                                      double dt, double max_velocity = 0.2);

    /**
     * @brief Set motor-specific P and D gains
     * @param motor_id Motor ID (1-9)
     * @param kp Proportional gain
     * @param kd Derivative gain
     * @return true if successful
     */
    bool set_motor_gains(int motor_id, double kp, double kd);

    /**
     * @brief Set P and D gains for all motors
     * @param kp_values Vector of proportional gains for each motor (size 9)
     * @param kd_values Vector of derivative gains for each motor (size 9)
     * @return true if successful
     */
    bool set_all_motor_gains(const std::vector<double>& kp_values,
                            const std::vector<double>& kd_values);

    /**
     * @brief Get motor P and D gains
     * @param motor_id Motor ID (1-9)
     * @param kp Output proportional gain
     * @param kd Output derivative gain
     * @return true if successful
     */
    bool get_motor_gains(int motor_id, double& kp, double& kd);

    /**
     * @brief Load default motor gains based on motor type
     */
    void load_default_motor_gains();

    /**
     * @brief Get system status information
     * @return Map of status key-value pairs
     */
    std::map<std::string, std::string> get_system_status();

    /**
     * @brief Print system information
     */
    void print_system_info();

    /**
     * @brief Get arm component
     * @return Reference to arm component
     */
    ArmComponent& get_arm();

    /**
     * @brief Get gripper component
     * @return Reference to gripper component
     */
    GripperComponent& get_gripper();

    /**
     * @brief Get wrist component
     * @return Reference to wrist component
     */
    WristComponent& get_wrist();

    /**
     * @brief Get safety module
     * @return Reference to safety module
     */
    SafetyModule& get_safety();

    /**
     * @brief Start logging system data to CSV files
     * @param log_directory Directory to save log files
     * @return true if logging started successfully
     */
    bool start_logging(const std::string& log_directory = ".");

    /**
     * @brief Start trajectory-specific logging
     * @param log_directory Directory to save log files
     * @return true if logging started successfully
     */
    bool start_trajectory_logging(const std::string& log_directory);

    /**
     * @brief Stop logging system data
     */
    void stop_logging();

    /**
     * @brief Get performance statistics
     * @return Map of performance metrics
     */
    std::map<std::string, double> get_performance_stats();

    /**
     * @brief Print performance statistics
     */
    void print_performance_stats();

    /**
     * @brief Enable frequency monitoring
     */
    void enable_frequency_monitoring();

    /**
     * @brief Disable frequency monitoring
     */
    void disable_frequency_monitoring();

    /**
     * @brief Enable gravity compensation for arm joints
     * @return true if successful
     */
    bool enable_gravity_compensation();

    /**
     * @brief Disable gravity compensation
     * @return true if successful
     */
    bool disable_gravity_compensation();

    /**
     * @brief Check if gravity compensation is enabled
     * @return true if enabled
     */
    bool is_gravity_compensation_enabled() const;

    /**
     * @brief Get gravity compensation torques for all joints
     * @return Vector of 9 gravity torques (first 6 are for arm joints)
     */
    std::vector<double> get_gravity_compensation_torques();

    /**
     * @brief Get all predicted torques (M*ddq + C*dq + G) for current state
     * @return Vector of 9 predicted torques (first 6 are for arm joints)
     */
    std::vector<double> get_all_predicted_torques();

    /**
     * @brief Print detailed torque component breakdown
     */
    void print_torque_breakdown();

    /**
     * @brief Switch torque prediction method
     * @param method_id Method ID (0=Pure C MATLAB, 1=Regressor-Based)
     * @return true if switch successful
     */
    bool switch_torque_prediction_method(int method_id);

    /**
     * @brief Print available torque prediction methods and their status
     */
    void print_torque_method_status();

    // Friction compensation API
    /**
     * @brief Enable friction compensation
     * @return true if successful
     */
    bool enable_friction_compensation();

    /**
     * @brief Disable friction compensation
     * @return true if successful
     */
    bool disable_friction_compensation();

    /**
     * @brief Check if friction compensation is enabled
     * @return true if enabled
     */
    bool is_friction_compensation_enabled() const;

    /**
     * @brief Load friction parameters from JSON file
     * @param filename Path to friction parameters JSON file
     * @return true if successful
     */
    bool load_friction_params_from_file(const std::string& filename);

    /**
     * @brief Print friction compensation status and parameters
     */
    void print_friction_compensation_status();

    // Safety and emergency methods
    /**
     * @brief Emergency stop - immediately stops all motion and puts motors in safe hold mode
     * @return true if emergency stop was successful
     */
    bool emergency_stop();

    /**
     * @brief Check system safety status
     * @return true if system is in safe operating condition
     */
    bool check_system_safety();

    // Wrist position monitoring methods
    /**
     * @brief Start wrist position monitoring for motors 7 and 8 only
     * @param frequency Monitoring frequency in Hz (default: 50Hz)
     * @return true if monitoring started successfully
     */
    bool start_wrist_position_monitoring(double frequency = 50.0);

    /**
     * @brief Stop wrist position monitoring
     */
    void stop_wrist_position_monitoring();

    /**
     * @brief Refresh wrist motors only (motors 7 and 8)
     * @return true if refresh successful
     */
    bool refresh_wrist_motors_only();

    /**
     * @brief Get current wrist positions (motors 7 and 8)
     * @return Vector of positions [motor_7, motor_8] in radians
     */
    std::vector<double> get_wrist_positions();

    /**
     * @brief Get current wrist velocities (motors 7 and 8)
     * @return Vector of velocities [motor_7, motor_8] in rad/s
     */
    std::vector<double> get_wrist_velocities();

    /**
     * @brief Get current wrist torques (motors 7 and 8)
     * @return Vector of torques [motor_7, motor_8] in Nm
     */
    std::vector<double> get_wrist_torques();

    /**
     * @brief Get comprehensive wrist monitoring data
     * @return Map containing positions, velocities, torques, and data age
     */
    std::map<std::string, std::vector<double>> get_wrist_monitoring_data();

    /**
     * @brief Print detailed wrist status information
     */
    void print_wrist_status();

    /**
     * @brief Check if wrist monitoring is currently running
     * @return true if wrist monitoring is active
     */
    bool is_wrist_monitoring_running() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ic_can
