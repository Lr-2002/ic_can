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

#include <vector>
#include <chrono>
#include <string>

namespace ic_can::python {

/**
 * @brief Motor state data structure for Python API
 *
 * Provides a clean, unified representation of motor state for all 9 motors
 * in the IC_ARM system. Designed for RL/DNN applications with NumPy compatibility.
 */
struct MotorState {
    // Joint states (9 elements: motors 1-9)
    std::vector<double> positions;      // Joint angles in radians
    std::vector<double> velocities;     // Joint velocities in rad/s
    std::vector<double> torques;        // Joint torques in Nm
    std::vector<double> temperatures;   // Motor temperatures in Celsius
    std::vector<bool> enabled;          // Motor enable status
    std::vector<bool> errors;           // Motor error status

    // Metadata
    double timestamp;                   // Unix timestamp in seconds
    int motor_count;                    // Number of motors (always 9)
    std::string control_mode;           // Current control mode ("TEACH" or "EXECUTION")

    // Constructor
    MotorState() :
        positions(9, 0.0),
        velocities(9, 0.0),
        torques(9, 0.0),
        temperatures(9, 25.0),
        enabled(9, false),
        errors(9, false),
        timestamp(0.0),
        motor_count(9),
        control_mode("TEACH") {}

    // Constructor from raw data
    MotorState(const std::vector<double>& pos,
               const std::vector<double>& vel,
               const std::vector<double>& tq,
               const std::string& mode = "TEACH") :
        positions(pos),
        velocities(vel),
        torques(tq),
        temperatures(9, 25.0),
        enabled(9, true),
        errors(9, false),
        timestamp(std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count() / 1000000.0),
        motor_count(9),
        control_mode(mode) {}

    // Get state as a single flat vector (for RL applications)
    std::vector<double> to_vector() const {
        std::vector<double> state_vec;
        state_vec.insert(state_vec.end(), positions.begin(), positions.end());
        state_vec.insert(state_vec.end(), velocities.begin(), velocities.end());
        state_vec.insert(state_vec.end(), torques.begin(), torques.end());
        return state_vec;
    }

    // Validate state consistency
    bool is_valid() const {
        return positions.size() == 9 &&
               velocities.size() == 9 &&
               torques.size() == 9 &&
               temperatures.size() == 9 &&
               enabled.size() == 9 &&
               errors.size() == 9;
    }

    // Check if any motor has errors
    bool has_errors() const {
        for (bool error : errors) {
            if (error) return true;
        }
        return false;
    }

    // Get positions for specific motor group
    std::vector<double> get_arm_positions() const {
        return std::vector<double>(positions.begin(), positions.begin() + 6); // motors 1-6
    }

    std::vector<double> get_wrist_positions() const {
        return std::vector<double>(positions.begin() + 6, positions.begin() + 8); // motors 7-8
    }

    double get_gripper_position() const {
        return positions[8]; // motor 9
    }

    // Check joint limits (basic safety check)
    bool is_within_limits() const {
        for (double pos : positions) {
            if (pos < -3.14159 || pos > 3.14159) { // +/- π radians
                return false;
            }
        }
        return true;
    }
};

/**
 * @brief Control command structure for Python API
 */
struct MotorCommand {
    std::vector<double> positions;      // Target positions (radians)
    std::vector<double> velocities;     // Target velocities (rad/s), optional
    std::vector<double> torques;        // Target torques (Nm), optional
    bool use_velocity_limits = false;   // Whether to apply velocity limits
    bool use_torque_limits = false;     // Whether to apply torque limits

    // Constructor
    MotorCommand(const std::vector<double>& pos) : positions(pos) {}

    // Validate command
    bool is_valid() const {
        return positions.size() == 9 &&
               (!use_velocity_limits || velocities.size() == 9) &&
               (!use_torque_limits || torques.size() == 9);
    }
};

} // namespace ic_can::python