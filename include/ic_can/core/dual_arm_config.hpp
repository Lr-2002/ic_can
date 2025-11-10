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

#include <string>
#include <vector>
#include <array>
#include <map>
#include <nlohmann/json.hpp>
#include "ic_can/core/config.hpp"

namespace ic_can {

/**
 * @brief Arm identifier enumeration
 */
enum class ArmIdentifier {
    LEFT_ARM,
    RIGHT_ARM
};

/**
 * @brief Arm-specific motor configuration
 * Maps internal motor IDs (1-18) to channel-specific CAN IDs (0x01-0x09)
 */
struct ArmMotorConfig {
    ArmIdentifier arm_id;
    int channel;                                    ///< ZLG CAN channel (0 or 1)
    std::vector<int> motor_indices;                ///< Internal motor indices (e.g., 1-9 for left, 10-18 for right)
    std::vector<uint32_t> can_send_ids;            ///< CAN send IDs (0x01-0x09 for both arms)
    std::vector<uint32_t> can_receive_ids;         ///< CAN receive IDs (0x11-0x19 for both arms)
    int servo_motor_index;                         ///< Servo motor index (9 for left, 18 for right)
    uint32_t servo_can_send_id;                    ///< Servo CAN send ID (0x09 for both)
    uint32_t servo_can_receive_id;                 ///< Servo CAN receive ID (0x19 for both)

    // Convenience methods
    int get_motor_count() const { return motor_indices.size(); }
    bool is_valid() const;
};

/**
 * @brief Dual-arm system configuration
 */
struct DualArmConfig {
    // Device configuration
    std::string device_serial = "";                ///< ZLG device serial (empty = auto-detect)
    bool auto_detect_device = true;

    // Arm configurations
    ArmMotorConfig left_arm;
    ArmMotorConfig right_arm;

    // System-wide settings
    bool enable_safety_checks = true;
    bool enable_coordination = true;
    double max_inter_arm_distance = 1.0;          ///< Maximum distance between end effectors

    // Performance settings
    double target_frequency = 500.0;              ///< Target control frequency (Hz)
    bool enable_performance_monitoring = true;

    // Convenience methods
    ArmMotorConfig get_arm_config(ArmIdentifier arm_id) const;
    bool validate() const;
    void set_defaults();

    // Serialization
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

/**
 * @brief Camera configuration for dual-arm system
 */
struct CameraConfig {
    int camera_id;                                 ///< Camera identifier (0, 1, 2)
    std::string camera_name;                       ///< Descriptive name
    std::string device_path;                       ///< Camera device path
    ArmIdentifier associated_arm;                 ///< Arm this camera is associated with
    bool is_hand_eye_camera;                       ///< True for hand-eye cameras, false for fixed
    std::string calibration_file;                 ///< Camera calibration file path

    // Camera parameters
    int resolution_width = 640;
    int resolution_height = 480;
    double framerate = 30.0;
    bool enabled = true;
};

/**
 * @brief Dual-camera system configuration
 */
struct DualCameraConfig {
    std::vector<CameraConfig> cameras;
    bool enable_synchronized_capture = true;
    bool enable_depth_sensing = false;

    // Convenience methods
    CameraConfig* get_camera_config(int camera_id);
    const CameraConfig* get_camera_config(int camera_id) const;
    bool validate() const;
    void set_defaults();

    // Serialization
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

/**
 * @brief Configuration manager for dual-arm system
 */
class DualArmConfigManager {
private:
    DualArmConfig dual_arm_config_;
    DualCameraConfig camera_config_;
    bool config_loaded_ = false;

public:
    /**
     * @brief Load configuration from JSON files
     */
    bool load_dual_arm_config(const std::string& config_file);
    bool load_camera_config(const std::string& config_file);
    bool load_all_configs(const std::string& dual_arm_config_file,
                         const std::string& camera_config_file);

    /**
     * @brief Save configuration to JSON files
     */
    bool save_dual_arm_config(const std::string& config_file) const;
    bool save_camera_config(const std::string& config_file) const;

    /**
     * @brief Get configuration
     */
    const DualArmConfig& get_dual_arm_config() const { return dual_arm_config_; }
    const DualCameraConfig& get_camera_config() const { return camera_config_; }

    /**
     * @brief Create communication config for specific arm
     */
    CommunicationConfig create_communication_config(ArmIdentifier arm_id) const;

    /**
     * @brief Utility methods
     */
    bool is_config_loaded() const { return config_loaded_; }
    void set_default_configs();

    /**
     * @brief Configuration validation
     */
    bool validate_configs() const;

    /**
     * @brief Print configuration summary
     */
    void print_config_summary() const;
};

// Utility functions
std::string arm_id_to_string(ArmIdentifier arm_id);
ArmIdentifier string_to_arm_id(const std::string& arm_str);

// Default configuration generators
DualArmConfig create_default_dual_arm_config();
DualCameraConfig create_default_camera_config();

} // namespace ic_can