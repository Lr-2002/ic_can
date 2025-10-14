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
class USB2CANAdapter;
class BaseMotor;
class ArmComponent;
class GripperComponent;
class SafetyModule;

/**
 * @brief Main IC_CAN controller class
 *
 * Provides high-level interface for controlling the IC_ARM system
 * with 9 motors (6 arm joints + 2 gripper + 1 servo) at 500Hz.
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
     * @brief Start high-frequency control loop (500Hz)
     * @return true if successful
     */
    bool start_high_frequency_control();

    /**
     * @brief Stop high-frequency control loop
     */
    void stop_high_frequency_control();

    /**
     * @brief Check if high-frequency control is running
     * @return true if running
     */
    bool is_hf_control_running() const;

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
     * @brief Get safety module
     * @return Reference to safety module
     */
    SafetyModule& get_safety();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ic_can