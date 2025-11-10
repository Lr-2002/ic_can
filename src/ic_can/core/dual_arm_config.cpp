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

#include "ic_can/core/dual_arm_config.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cassert>

namespace ic_can {

// ArmMotorConfig implementation
bool ArmMotorConfig::is_valid() const {
    if (motor_indices.size() != 9) {
        std::cout << "❌ Arm must have exactly 9 motors, got " << motor_indices.size() << std::endl;
        return false;
    }

    if (can_send_ids.size() != 9 || can_receive_ids.size() != 9) {
        std::cout << "❌ Arm must have exactly 9 CAN send/receive IDs" << std::endl;
        return false;
    }

    if (channel < 0 || channel > 1) {
        std::cout << "❌ Invalid channel: " << channel << " (must be 0 or 1)" << std::endl;
        return false;
    }

    // Check CAN IDs are in expected range
    for (size_t i = 0; i < can_send_ids.size(); i++) {
        if (can_send_ids[i] < 0x01 || can_send_ids[i] > 0x09) {
            std::cout << "❌ Invalid CAN send ID: 0x" << std::hex << can_send_ids[i] << std::endl;
            return false;
        }
        if (can_receive_ids[i] < 0x11 || can_receive_ids[i] > 0x19) {
            std::cout << "❌ Invalid CAN receive ID: 0x" << std::hex << can_receive_ids[i] << std::endl;
            return false;
        }
    }

    return true;
}

// DualArmConfig implementation
ArmMotorConfig DualArmConfig::get_arm_config(ArmIdentifier arm_id) const {
    switch (arm_id) {
        case ArmIdentifier::LEFT_ARM:
            return left_arm;
        case ArmIdentifier::RIGHT_ARM:
            return right_arm;
        default:
            // Return empty config
            return ArmMotorConfig{};
    }
}

bool DualArmConfig::validate() const {
    // Validate arm configurations
    if (!left_arm.is_valid()) {
        std::cout << "❌ Left arm configuration is invalid" << std::endl;
        return false;
    }

    if (!right_arm.is_valid()) {
        std::cout << "❌ Right arm configuration is invalid" << std::endl;
        return false;
    }

    // Check channels are different
    if (left_arm.channel == right_arm.channel) {
        std::cout << "❌ Both arms cannot use the same channel: " << left_arm.channel << std::endl;
        return false;
    }

    // Check motor indices don't overlap
    for (int left_motor : left_arm.motor_indices) {
        for (int right_motor : right_arm.motor_indices) {
            if (left_motor == right_motor) {
                std::cout << "❌ Overlapping motor index: " << left_motor << std::endl;
                return false;
            }
        }
    }

    // Check system parameters
    if (target_frequency <= 0 || target_frequency > 2000) {
        std::cout << "❌ Invalid target frequency: " << target_frequency << " Hz" << std::endl;
        return false;
    }

    return true;
}

void DualArmConfig::set_defaults() {
    *this = create_default_dual_arm_config();
}

nlohmann::json DualArmConfig::to_json() const {
    nlohmann::json j;

    j["device_serial"] = device_serial;
    j["auto_detect_device"] = auto_detect_device;

    // Left arm configuration
    j["left_arm"]["arm_id"] = "left";
    j["left_arm"]["channel"] = left_arm.channel;
    j["left_arm"]["motor_indices"] = left_arm.motor_indices;
    j["left_arm"]["can_send_ids"] = left_arm.can_send_ids;
    j["left_arm"]["can_receive_ids"] = left_arm.can_receive_ids;
    j["left_arm"]["servo_motor_index"] = left_arm.servo_motor_index;
    j["left_arm"]["servo_can_send_id"] = left_arm.servo_can_send_id;
    j["left_arm"]["servo_can_receive_id"] = left_arm.servo_can_receive_id;

    // Right arm configuration
    j["right_arm"]["arm_id"] = "right";
    j["right_arm"]["channel"] = right_arm.channel;
    j["right_arm"]["motor_indices"] = right_arm.motor_indices;
    j["right_arm"]["can_send_ids"] = right_arm.can_send_ids;
    j["right_arm"]["can_receive_ids"] = right_arm.can_receive_ids;
    j["right_arm"]["servo_motor_index"] = right_arm.servo_motor_index;
    j["right_arm"]["servo_can_send_id"] = right_arm.servo_can_send_id;
    j["right_arm"]["servo_can_receive_id"] = right_arm.servo_can_receive_id;

    // System settings
    j["enable_safety_checks"] = enable_safety_checks;
    j["enable_coordination"] = enable_coordination;
    j["max_inter_arm_distance"] = max_inter_arm_distance;
    j["target_frequency"] = target_frequency;
    j["enable_performance_monitoring"] = enable_performance_monitoring;

    return j;
}

void DualArmConfig::from_json(const nlohmann::json& j) {
    try {
        if (j.contains("device_serial")) {
            device_serial = j["device_serial"].get<std::string>();
        }

        if (j.contains("auto_detect_device")) {
            auto_detect_device = j["auto_detect_device"].get<bool>();
        }

        // Left arm
        if (j.contains("left_arm")) {
            const auto& left = j["left_arm"];
            left_arm.arm_id = ArmIdentifier::LEFT_ARM;
            left_arm.channel = left.value("channel", 0);
            left_arm.motor_indices = left.value("motor_indices", std::vector<int>{1,2,3,4,5,6,7,8,9});
            left_arm.can_send_ids = left.value("can_send_ids", std::vector<uint32_t>{0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09});
            left_arm.can_receive_ids = left.value("can_receive_ids", std::vector<uint32_t>{0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19});
            left_arm.servo_motor_index = left.value("servo_motor_index", 9);
            left_arm.servo_can_send_id = left.value("servo_can_send_id", 0x09);
            left_arm.servo_can_receive_id = left.value("servo_can_receive_id", 0x19);
        }

        // Right arm
        if (j.contains("right_arm")) {
            const auto& right = j["right_arm"];
            right_arm.arm_id = ArmIdentifier::RIGHT_ARM;
            right_arm.channel = right.value("channel", 1);
            right_arm.motor_indices = right.value("motor_indices", std::vector<int>{10,11,12,13,14,15,16,17,18});
            right_arm.can_send_ids = right.value("can_send_ids", std::vector<uint32_t>{0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09});
            right_arm.can_receive_ids = right.value("can_receive_ids", std::vector<uint32_t>{0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19});
            right_arm.servo_motor_index = right.value("servo_motor_index", 18);
            right_arm.servo_can_send_id = right.value("servo_can_send_id", 0x09);
            right_arm.servo_can_receive_id = right.value("servo_can_receive_id", 0x19);
        }

        // System settings
        enable_safety_checks = j.value("enable_safety_checks", true);
        enable_coordination = j.value("enable_coordination", true);
        max_inter_arm_distance = j.value("max_inter_arm_distance", 1.0);
        target_frequency = j.value("target_frequency", 500.0);
        enable_performance_monitoring = j.value("enable_performance_monitoring", true);

    } catch (const nlohmann::json::exception& e) {
        std::cout << "❌ JSON parsing error: " << e.what() << std::endl;
        throw;
    }
}

// DualCameraConfig implementation
CameraConfig* DualCameraConfig::get_camera_config(int camera_id) {
    for (auto& camera : cameras) {
        if (camera.camera_id == camera_id) {
            return &camera;
        }
    }
    return nullptr;
}

const CameraConfig* DualCameraConfig::get_camera_config(int camera_id) const {
    for (const auto& camera : cameras) {
        if (camera.camera_id == camera_id) {
            return &camera;
        }
    }
    return nullptr;
}

bool DualCameraConfig::validate() const {
    if (cameras.size() != 3) {
        std::cout << "❌ Expected exactly 3 cameras, got " << cameras.size() << std::endl;
        return false;
    }

    // Check for duplicate camera IDs
    std::set<int> camera_ids;
    for (const auto& camera : cameras) {
        if (camera_ids.count(camera.camera_id)) {
            std::cout << "❌ Duplicate camera ID: " << camera.camera_id << std::endl;
            return false;
        }
        camera_ids.insert(camera.camera_id);
    }

    // Validate camera parameters
    for (const auto& camera : cameras) {
        if (camera.camera_id < 0 || camera.camera_id > 2) {
            std::cout << "❌ Invalid camera ID: " << camera.camera_id << " (must be 0-2)" << std::endl;
            return false;
        }

        if (camera.resolution_width <= 0 || camera.resolution_height <= 0) {
            std::cout << "❌ Invalid resolution for camera " << camera.camera_id << std::endl;
            return false;
        }

        if (camera.framerate <= 0) {
            std::cout << "❌ Invalid framerate for camera " << camera.camera_id << std::endl;
            return false;
        }
    }

    return true;
}

void DualCameraConfig::set_defaults() {
    *this = create_default_camera_config();
}

nlohmann::json DualCameraConfig::to_json() const {
    nlohmann::json j;

    j["cameras"] = nlohmann::json::array();
    for (const auto& camera : cameras) {
        nlohmann::json cam_json;
        cam_json["camera_id"] = camera.camera_id;
        cam_json["camera_name"] = camera.camera_name;
        cam_json["device_path"] = camera.device_path;
        cam_json["associated_arm"] = arm_id_to_string(camera.associated_arm);
        cam_json["is_hand_eye_camera"] = camera.is_hand_eye_camera;
        cam_json["calibration_file"] = camera.calibration_file;
        cam_json["resolution_width"] = camera.resolution_width;
        cam_json["resolution_height"] = camera.resolution_height;
        cam_json["framerate"] = camera.framerate;
        cam_json["enabled"] = camera.enabled;
        j["cameras"].push_back(cam_json);
    }

    j["enable_synchronized_capture"] = enable_synchronized_capture;
    j["enable_depth_sensing"] = enable_depth_sensing;

    return j;
}

void DualCameraConfig::from_json(const nlohmann::json& j) {
    try {
        cameras.clear();

        if (j.contains("cameras")) {
            for (const auto& cam_json : j["cameras"]) {
                CameraConfig camera;
                camera.camera_id = cam_json.value("camera_id", 0);
                camera.camera_name = cam_json.value("camera_name", "unknown");
                camera.device_path = cam_json.value("device_path", "");
                camera.associated_arm = string_to_arm_id(cam_json.value("associated_arm", "left"));
                camera.is_hand_eye_camera = cam_json.value("is_hand_eye_camera", false);
                camera.calibration_file = cam_json.value("calibration_file", "");
                camera.resolution_width = cam_json.value("resolution_width", 640);
                camera.resolution_height = cam_json.value("resolution_height", 480);
                camera.framerate = cam_json.value("framerate", 30.0);
                camera.enabled = cam_json.value("enabled", true);
                cameras.push_back(camera);
            }
        }

        enable_synchronized_capture = j.value("enable_synchronized_capture", true);
        enable_depth_sensing = j.value("enable_depth_sensing", false);

    } catch (const nlohmann::json::exception& e) {
        std::cout << "❌ JSON parsing error in camera config: " << e.what() << std::endl;
        throw;
    }
}

// DualArmConfigManager implementation
bool DualArmConfigManager::load_dual_arm_config(const std::string& config_file) {
    try {
        std::ifstream file(config_file);
        if (!file.is_open()) {
            std::cout << "❌ Cannot open dual-arm config file: " << config_file << std::endl;
            return false;
        }

        nlohmann::json j;
        file >> j;
        dual_arm_config_.from_json(j);

        if (!dual_arm_config_.validate()) {
            std::cout << "❌ Dual-arm configuration validation failed" << std::endl;
            return false;
        }

        config_loaded_ = true;
        std::cout << "✅ Dual-arm configuration loaded from: " << config_file << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cout << "❌ Error loading dual-arm config: " << e.what() << std::endl;
        return false;
    }
}

bool DualArmConfigManager::load_camera_config(const std::string& config_file) {
    try {
        std::ifstream file(config_file);
        if (!file.is_open()) {
            std::cout << "❌ Cannot open camera config file: " << config_file << std::endl;
            return false;
        }

        nlohmann::json j;
        file >> j;
        camera_config_.from_json(j);

        if (!camera_config_.validate()) {
            std::cout << "❌ Camera configuration validation failed" << std::endl;
            return false;
        }

        std::cout << "✅ Camera configuration loaded from: " << config_file << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cout << "❌ Error loading camera config: " << e.what() << std::endl;
        return false;
    }
}

bool DualArmConfigManager::load_all_configs(const std::string& dual_arm_config_file,
                                          const std::string& camera_config_file) {
    bool success = true;
    success &= load_dual_arm_config(dual_arm_config_file);
    success &= load_camera_config(camera_config_file);

    if (success) {
        std::cout << "✅ All dual-arm configurations loaded successfully" << std::endl;
    }

    return success;
}

bool DualArmConfigManager::save_dual_arm_config(const std::string& config_file) const {
    try {
        std::ofstream file(config_file);
        if (!file.is_open()) {
            std::cout << "❌ Cannot create dual-arm config file: " << config_file << std::endl;
            return false;
        }

        nlohmann::json j = dual_arm_config_.to_json();
        file << j.dump(4); // Pretty print with 4 spaces

        std::cout << "✅ Dual-arm configuration saved to: " << config_file << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cout << "❌ Error saving dual-arm config: " << e.what() << std::endl;
        return false;
    }
}

bool DualArmConfigManager::save_camera_config(const std::string& config_file) const {
    try {
        std::ofstream file(config_file);
        if (!file.is_open()) {
            std::cout << "❌ Cannot create camera config file: " << config_file << std::endl;
            return false;
        }

        nlohmann::json j = camera_config_.to_json();
        file << j.dump(4); // Pretty print with 4 spaces

        std::cout << "✅ Camera configuration saved to: " << config_file << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cout << "❌ Error saving camera config: " << e.what() << std::endl;
        return false;
    }
}

CommunicationConfig DualArmConfigManager::create_communication_config(ArmIdentifier arm_id) const {
    CommunicationConfig config;

    const ArmMotorConfig& arm_config = dual_arm_config_.get_arm_config(arm_id);

    // Use ZLG backend with specific channel
    config.preferred_backend = "zlg";
    config.device_serial = dual_arm_config_.device_serial;
    config.preferred_channel = arm_config.channel;
    config.auto_detect_device = dual_arm_config_.auto_detect_device;
    config.zlg_can_fd_mode = true;
    config.zlg_arbitration_baud = 1000000;  // 1 Mbps
    config.zlg_data_baud = 5000000;         // 5 Mbps
    config.enable_performance_monitoring = dual_arm_config_.enable_performance_monitoring;
    config.verbose_logging = false;

    // Add fallback backends
    config.fallback_backends = {"dm_tools"};

    return config;
}

void DualArmConfigManager::set_default_configs() {
    dual_arm_config_ = create_default_dual_arm_config();
    camera_config_ = create_default_camera_config();
    config_loaded_ = true;
}

bool DualArmConfigManager::validate_configs() const {
    return dual_arm_config_.validate() && camera_config_.validate();
}

void DualArmConfigManager::print_config_summary() const {
    if (!config_loaded_) {
        std::cout << "⚠️ No configuration loaded" << std::endl;
        return;
    }

    std::cout << "\n📋 Dual-Arm Configuration Summary:" << std::endl;
    std::cout << std::string(50, '=') << std::endl;

    std::cout << "Device Serial: " << (dual_arm_config_.device_serial.empty() ? "auto-detect" : dual_arm_config_.device_serial) << std::endl;
    std::cout << "Target Frequency: " << dual_arm_config_.target_frequency << " Hz" << std::endl;
    std::cout << "Safety Checks: " << (dual_arm_config_.enable_safety_checks ? "enabled" : "disabled") << std::endl;

    std::cout << "\n🦾 Left Arm (Channel " << dual_arm_config_.left_arm.channel << "):" << std::endl;
    std::cout << "  Motors: ";
    for (int i = 0; i < dual_arm_config_.left_arm.motor_indices.size(); i++) {
        std::cout << dual_arm_config_.left_arm.motor_indices[i];
        if (i < dual_arm_config_.left_arm.motor_indices.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    std::cout << "  Servo: Motor " << dual_arm_config_.left_arm.servo_motor_index << std::endl;

    std::cout << "\n🦾 Right Arm (Channel " << dual_arm_config_.right_arm.channel << "):" << std::endl;
    std::cout << "  Motors: ";
    for (int i = 0; i < dual_arm_config_.right_arm.motor_indices.size(); i++) {
        std::cout << dual_arm_config_.right_arm.motor_indices[i];
        if (i < dual_arm_config_.right_arm.motor_indices.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    std::cout << "  Servo: Motor " << dual_arm_config_.right_arm.servo_motor_index << std::endl;

    std::cout << "\n📷 Camera Configuration:" << std::endl;
    for (const auto& camera : camera_config_.cameras) {
        std::cout << "  Camera " << camera.camera_id << " (" << camera.camera_name << "): ";
        std::cout << (camera.is_hand_eye_camera ? "Hand-eye" : "Fixed") << " - ";
        std::cout << arm_id_to_string(camera.associated_arm) << " arm" << std::endl;
    }

    std::cout << std::string(50, '=') << std::endl;
}

// Utility functions
std::string arm_id_to_string(ArmIdentifier arm_id) {
    switch (arm_id) {
        case ArmIdentifier::LEFT_ARM: return "left";
        case ArmIdentifier::RIGHT_ARM: return "right";
        default: return "unknown";
    }
}

ArmIdentifier string_to_arm_id(const std::string& arm_str) {
    if (arm_str == "left") return ArmIdentifier::LEFT_ARM;
    if (arm_str == "right") return ArmIdentifier::RIGHT_ARM;
    return ArmIdentifier::LEFT_ARM; // Default to left
}

// Default configuration generators
DualArmConfig create_default_dual_arm_config() {
    DualArmConfig config;

    // Device settings
    config.device_serial = "";  // Auto-detect
    config.auto_detect_device = true;

    // Left arm configuration (Channel 0, Motors 1-9)
    config.left_arm.arm_id = ArmIdentifier::LEFT_ARM;
    config.left_arm.channel = 0;
    config.left_arm.motor_indices = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    config.left_arm.can_send_ids = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
    config.left_arm.can_receive_ids = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19};
    config.left_arm.servo_motor_index = 9;
    config.left_arm.servo_can_send_id = 0x09;
    config.left_arm.servo_can_receive_id = 0x19;

    // Right arm configuration (Channel 1, Motors 10-18)
    config.right_arm.arm_id = ArmIdentifier::RIGHT_ARM;
    config.right_arm.channel = 1;
    config.right_arm.motor_indices = {10, 11, 12, 13, 14, 15, 16, 17, 18};
    config.right_arm.can_send_ids = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
    config.right_arm.can_receive_ids = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19};
    config.right_arm.servo_motor_index = 18;
    config.right_arm.servo_can_send_id = 0x09;
    config.right_arm.servo_can_receive_id = 0x19;

    // System settings
    config.enable_safety_checks = true;
    config.enable_coordination = true;
    config.max_inter_arm_distance = 1.0;
    config.target_frequency = 500.0;
    config.enable_performance_monitoring = true;

    return config;
}

DualCameraConfig create_default_camera_config() {
    DualCameraConfig config;

    config.enable_synchronized_capture = true;
    config.enable_depth_sensing = false;

    // Camera 0: Left hand-eye camera
    CameraConfig cam0;
    cam0.camera_id = 0;
    cam0.camera_name = "left_hand_eye";
    cam0.device_path = "/dev/video0";
    cam0.associated_arm = ArmIdentifier::LEFT_ARM;
    cam0.is_hand_eye_camera = true;
    cam0.calibration_file = "calibration/left_camera.json";
    cam0.resolution_width = 640;
    cam0.resolution_height = 480;
    cam0.framerate = 30.0;
    cam0.enabled = true;
    config.cameras.push_back(cam0);

    // Camera 1: Right hand-eye camera
    CameraConfig cam1;
    cam1.camera_id = 1;
    cam1.camera_name = "right_hand_eye";
    cam1.device_path = "/dev/video1";
    cam1.associated_arm = ArmIdentifier::RIGHT_ARM;
    cam1.is_hand_eye_camera = true;
    cam1.calibration_file = "calibration/right_camera.json";
    cam1.resolution_width = 640;
    cam1.resolution_height = 480;
    cam1.framerate = 30.0;
    cam1.enabled = true;
    config.cameras.push_back(cam1);

    // Camera 2: Fixed overhead camera
    CameraConfig cam2;
    cam2.camera_id = 2;
    cam2.camera_name = "overhead_fixed";
    cam2.device_path = "/dev/video2";
    cam2.associated_arm = ArmIdentifier::LEFT_ARM;  // Can be associated with either or both
    cam2.is_hand_eye_camera = false;
    cam2.calibration_file = "calibration/overhead_camera.json";
    cam2.resolution_width = 1280;
    cam2.resolution_height = 720;
    cam2.framerate = 30.0;
    cam2.enabled = true;
    config.cameras.push_back(cam2);

    return config;
}

} // namespace ic_can