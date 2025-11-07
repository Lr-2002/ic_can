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

#include "ic_can/core/config.hpp"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <cstdlib>

namespace ic_can {

// ===================================
// UTILITY FUNCTIONS
// ===================================

std::string debug_level_to_string(DebugLevel level) {
    switch (level) {
        case DebugLevel::DEBUG: return "DEBUG";
        case DebugLevel::INFO: return "INFO";
        case DebugLevel::WARN: return "WARN";
        case DebugLevel::ERROR: return "ERROR";
        default: return "INFO";
    }
}

DebugLevel string_to_debug_level(const std::string& level_str) {
    std::string upper_level = level_str;
    std::transform(upper_level.begin(), upper_level.end(), upper_level.begin(), ::toupper);

    if (upper_level == "DEBUG") return DebugLevel::DEBUG;
    if (upper_level == "INFO") return DebugLevel::INFO;
    if (upper_level == "WARN" || upper_level == "WARNING") return DebugLevel::WARN;
    if (upper_level == "ERROR") return DebugLevel::ERROR;

    return DebugLevel::INFO; // Default
}

std::string log_format_to_string(LogFormat format) {
    switch (format) {
        case LogFormat::CSV: return "csv";
        case LogFormat::JSON: return "json";
        case LogFormat::HUMAN_READABLE: return "human_readable";
        default: return "csv";
    }
}

LogFormat string_to_log_format(const std::string& format_str) {
    std::string lower_format = format_str;
    std::transform(lower_format.begin(), lower_format.end(), lower_format.begin(), ::tolower);

    if (lower_format == "csv") return LogFormat::CSV;
    if (lower_format == "json") return LogFormat::JSON;
    if (lower_format == "human_readable" || lower_format == "human") return LogFormat::HUMAN_READABLE;

    return LogFormat::CSV; // Default
}

std::string control_mode_to_string(ControlMode mode) {
    switch (mode) {
        case ControlMode::TEACH_MODE: return "TEACH_MODE";
        case ControlMode::EXECUTION_MODE: return "EXECUTION_MODE";
        default: return "EXECUTION_MODE";
    }
}

ControlMode string_to_control_mode(const std::string& mode_str) {
    std::string upper_mode = mode_str;
    std::transform(upper_mode.begin(), upper_mode.end(), upper_mode.begin(), ::toupper);

    if (upper_mode == "TEACH_MODE") return ControlMode::TEACH_MODE;
    if (upper_mode == "EXECUTION_MODE") return ControlMode::EXECUTION_MODE;

    return ControlMode::EXECUTION_MODE; // Default
}

std::string communication_backend_to_string(CommunicationBackend backend) {
    switch (backend) {
        case CommunicationBackend::ZLG_CAN_FD: return "zlg";
        case CommunicationBackend::DM_TOOLS: return "dm_tools";
        case CommunicationBackend::SIMULATION: return "simulation";
        case CommunicationBackend::AUTO: return "auto";
        default: return "zlg";
    }
}

CommunicationBackend string_to_communication_backend(const std::string& backend_str) {
    std::string lower_backend = backend_str;
    std::transform(lower_backend.begin(), lower_backend.end(), lower_backend.begin(), ::tolower);

    if (lower_backend == "zlg" || lower_backend == "zlg_can_fd") return CommunicationBackend::ZLG_CAN_FD;
    if (lower_backend == "dm_tools" || lower_backend == "dm-tools") return CommunicationBackend::DM_TOOLS;
    if (lower_backend == "simulation" || lower_backend == "sim") return CommunicationBackend::SIMULATION;
    if (lower_backend == "auto") return CommunicationBackend::AUTO;

    return CommunicationBackend::ZLG_CAN_FD; // Default
}

// ===================================
// MOTORS CONFIG IMPLEMENTATION
// ===================================

std::vector<MotorConfig> MotorsConfig::all_motors() const {
    std::vector<MotorConfig> all;
    all.insert(all.end(), arm_motors.begin(), arm_motors.end());
    all.insert(all.end(), wrist_motors.begin(), wrist_motors.end());
    all.insert(all.end(), gripper_motors.begin(), gripper_motors.end());
    return all;
}

MotorConfig MotorsConfig::get_motor(int joint) const {
    for (const auto& motor : arm_motors) {
        if (motor.joint == joint) return motor;
    }
    for (const auto& motor : wrist_motors) {
        if (motor.joint == joint) return motor;
    }
    for (const auto& motor : gripper_motors) {
        if (motor.joint == joint) return motor;
    }

    // Return empty motor config if not found
    MotorConfig empty_motor;
    empty_motor.joint = joint;
    return empty_motor;
}

bool MotorsConfig::validate() const {
    // Check for duplicate joint numbers
    std::vector<int> joint_numbers;
    auto all_motors_vec = all_motors();

    for (const auto& motor : all_motors_vec) {
        if (std::find(joint_numbers.begin(), joint_numbers.end(), motor.joint) != joint_numbers.end()) {
            return false; // Duplicate joint number
        }
        joint_numbers.push_back(motor.joint);
    }

    return true;
}

// ===================================
// COMMUNICATION CONFIG IMPLEMENTATION
// ===================================

bool CommunicationConfig::validate() const {
    // Check that at least one backend is enabled
    if (!zlg_config.enabled && !dm_tools_config.enabled && !simulation_config.enabled) {
        return false;
    }

    return true;
}

// ===================================
// IC_CAN CONFIG IMPLEMENTATION
// ===================================

bool IC_CANConfig::validate() const {
    if (!motors.validate()) return false;
    if (!communication.validate()) return false;

    // Check system frequency
    if (system.frequency <= 0 || system.frequency > 10000) {
        return false;
    }

    return true;
}

void IC_CANConfig::set_defaults() {
    // Set all default values here if needed
    config_version = "1.0.0";
    config_description = "IC_CAN Robot Control Configuration";
    last_modified = std::chrono::system_clock::now();
}

std::map<std::string, std::string> IC_CANConfig::get_summary() const {
    std::map<std::string, std::string> summary;

    summary["version"] = config_version;
    summary["frequency"] = std::to_string(system.frequency);
    summary["control_mode"] = control_mode_to_string(system.control_mode);
    summary["motors_count"] = std::to_string(motors.all_motors().size());
    summary["preferred_backend"] = communication_backend_to_string(communication.preferred_backend);
    summary["debug_level"] = debug_level_to_string(debug.debug_level);

    return summary;
}

// ===================================
// CONFIG MANAGER IMPLEMENTATION
// ===================================

ConfigManager::ConfigManager(const std::string& config_file)
    : config_(std::make_unique<IC_CANConfig>()),
      config_file_path_(config_file),
      config_loaded_(false) {

    // Set default configuration
    config_->set_defaults();

    // Load configuration if file provided
    if (!config_file.empty()) {
        load_config(config_file);
    }
}

ConfigManager::~ConfigManager() = default;

bool ConfigManager::load_config(const std::string& config_file) {
    config_file_path_ = config_file;

    if (!parse_yaml_file(config_file)) {
        std::cerr << "❌ Failed to parse configuration file: " << config_file << std::endl;
        return false;
    }

    if (!validate_config_structure()) {
        std::cerr << "❌ Configuration validation failed for: " << config_file << std::endl;
        return false;
    }

    config_loaded_ = true;
    config_->last_modified = std::chrono::system_clock::now();

    std::cout << "✅ Configuration loaded successfully from: " << config_file << std::endl;
    return true;
}

bool ConfigManager::save_config(const std::string& config_file) const {
    if (!config_loaded_) {
        std::cerr << "❌ No configuration loaded to save" << std::endl;
        return false;
    }

    return write_yaml_file(config_file);
}

bool ConfigManager::reload_config() {
    if (config_file_path_.empty()) {
        std::cerr << "❌ No configuration file path set for reload" << std::endl;
        return false;
    }

    return load_config(config_file_path_);
}

const IC_CANConfig& ConfigManager::get_config() const {
    return *config_;
}

const SystemConfig& ConfigManager::get_system_config() const {
    return config_->system;
}

const MotorsConfig& ConfigManager::get_motors_config() const {
    return config_->motors;
}

const CommunicationConfig& ConfigManager::get_communication_config() const {
    return config_->communication;
}

const LoggingConfig& ConfigManager::get_logging_config() const {
    return config_->logging;
}

const DebugConfig& ConfigManager::get_debug_config() const {
    return config_->debug;
}

const FeaturesConfig& ConfigManager::get_features_config() const {
    return config_->features;
}

bool ConfigManager::set_config_value(const std::string& key, const std::string& value) {
    // Simple dot-notation configuration setting
    // Format: "section.subsection.parameter"
    std::istringstream key_stream(key);
    std::string section;
    std::vector<std::string> parts;

    while (std::getline(key_stream, section, '.')) {
        parts.push_back(section);
    }

    if (parts.size() < 2) {
        return false;
    }

    // This is a simplified implementation - in practice, you'd want more sophisticated handling
    try {
        if (parts[0] == "system" && parts[1] == "frequency") {
            config_->system.frequency = std::stod(value);
            return true;
        } else if (parts[0] == "debug" && parts[1] == "debug_level") {
            config_->debug.debug_level = string_to_debug_level(value);
            return true;
        }
        // Add more configuration options as needed
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to set config value " << key << " = " << value << ": " << e.what() << std::endl;
        return false;
    }

    return false;
}

std::string ConfigManager::get_config_value(const std::string& key) const {
    // Simple dot-notation configuration getting
    std::istringstream key_stream(key);
    std::string section;
    std::vector<std::string> parts;

    while (std::getline(key_stream, section, '.')) {
        parts.push_back(section);
    }

    if (parts.size() < 2) {
        return "";
    }

    if (parts[0] == "system" && parts[1] == "frequency") {
        return std::to_string(config_->system.frequency);
    } else if (parts[0] == "debug" && parts[1] == "debug_level") {
        return debug_level_to_string(config_->debug.debug_level);
    }

    return "";
}

bool ConfigManager::validate_config() const {
    if (!config_loaded_) {
        return false;
    }

    return config_->validate();
}

const std::string& ConfigManager::get_config_file_path() const {
    return config_file_path_;
}

void ConfigManager::print_config_summary() const {
    if (!config_loaded_) {
        std::cout << "❌ No configuration loaded" << std::endl;
        return;
    }

    std::cout << "\n🔧 IC_CAN Configuration Summary:" << std::endl;
    std::cout << "   File: " << config_file_path_ << std::endl;

    auto summary = config_->get_summary();
    for (const auto& [key, value] : summary) {
        std::cout << "   " << key << ": " << value << std::endl;
    }
    std::cout << std::endl;
}

bool ConfigManager::is_config_loaded() const {
    return config_loaded_;
}

bool ConfigManager::create_default_config(const std::string& config_file) const {
    // Create a default configuration and save it
    IC_CANConfig default_config;
    default_config.set_defaults();

    // Create default motor configurations for 9 motors
    for (int i = 1; i <= 6; ++i) {
        MotorConfig arm_motor;
        arm_motor.joint = i;
        arm_motor.type = (i <= 2) ? "DM10010L" : (i <= 4) ? "DM6248" : "DM4340";
        arm_motor.can_send_id = 0x200 + i;
        arm_motor.can_receive_id = 0x200 + i;
        default_config.motors.arm_motors.push_back(arm_motor);
    }

    for (int i = 7; i <= 8; ++i) {
        MotorConfig wrist_motor;
        wrist_motor.joint = i;
        wrist_motor.type = "HT4438";
        wrist_motor.can_send_id = 0x200 + i;
        wrist_motor.can_receive_id = 0x200 + i;
        default_config.motors.wrist_motors.push_back(wrist_motor);
    }

    MotorConfig gripper_motor;
    gripper_motor.joint = 9;
    gripper_motor.type = "SERVO";
    gripper_motor.can_send_id = 0x209;
    gripper_motor.can_receive_id = 0x209;
    default_config.motors.gripper_motors.push_back(gripper_motor);

    // Save the configuration using a temporary ConfigManager
    ConfigManager temp_manager;
    *temp_manager.config_ = default_config;
    temp_manager.config_loaded_ = true;
    return temp_manager.write_yaml_file(config_file);
}

bool ConfigManager::merge_config(const std::string& config_file) {
    if (!config_loaded_) {
        return load_config(config_file);
    }

    // Load the new configuration and merge it
    IC_CANConfig new_config;
    ConfigManager temp_manager;
    if (!temp_manager.load_config(config_file)) {
        return false;
    }

    new_config = temp_manager.get_config();

    // Merge logic - new config overrides current config
    // This is a simplified merge - in practice, you'd want more sophisticated merging
    config_->system = new_config.system;
    config_->debug = new_config.debug;
    config_->logging = new_config.logging;
    config_->communication = new_config.communication;
    config_->features = new_config.features;

    // For motors, we might want to merge more carefully
    if (!new_config.motors.arm_motors.empty()) {
        config_->motors = new_config.motors;
    }

    config_->last_modified = std::chrono::system_clock::now();

    return true;
}

// ===================================
// PRIVATE HELPER METHODS
// ===================================

bool ConfigManager::parse_yaml_file(const std::string& file_path) {
    try {
        YAML::Node root = YAML::LoadFile(file_path);

        // Parse each configuration section
        if (root["system"]) {
            parse_system_config(root["system"]);
        }

        if (root["debug"]) {
            parse_debug_config(root["debug"]);
        }

        if (root["logging"]) {
            parse_logging_config(root["logging"]);
        }

        if (root["motors"]) {
            parse_motors_config(root["motors"]);
        }

        if (root["communication"]) {
            parse_communication_config(root["communication"]);
        }

        if (root["features"]) {
            parse_features_config(root["features"]);
        }

        // Parse metadata
        if (root["config_version"]) {
            config_->config_version = root["config_version"].as<std::string>();
        }

        if (root["config_description"]) {
            config_->config_description = root["config_description"].as<std::string>();
        }

        return true;

    } catch (const YAML::Exception& e) {
        std::cerr << "❌ YAML parsing error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "❌ Configuration parsing error: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::write_yaml_file(const std::string& file_path) const {
    try {
        YAML::Node root;

        // System configuration
        YAML::Node system_node;
        system_node["frequency"] = config_->system.frequency;
        system_node["control_mode"] = control_mode_to_string(config_->system.control_mode);
        system_node["safety_enabled"] = config_->system.safety_enabled;
        system_node["performance_monitoring"] = config_->system.performance_monitoring;
        root["system"] = system_node;

        // Debug configuration
        YAML::Node debug_node;
        debug_node["debug_level"] = debug_level_to_string(config_->debug.debug_level);
        debug_node["can_debug"] = config_->debug.can_debug;
        debug_node["motor_debug"] = config_->debug.motor_debug;
        debug_node["system_debug"] = config_->debug.system_debug;
        root["debug"] = debug_node;

        // Logging configuration
        YAML::Node logging_node;
        // Add logging configuration as needed
        root["logging"] = logging_node;

        // Motor configuration would go here
        // This is a simplified version

        // Metadata
        root["config_version"] = config_->config_version;
        root["config_description"] = config_->config_description;

        // Write to file
        std::ofstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "❌ Failed to open file for writing: " << file_path << std::endl;
            return false;
        }

        file << root;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to write configuration file: " << e.what() << std::endl;
        return false;
    }
}

void ConfigManager::apply_environment_substitution() {
    // This would substitute environment variables in configuration values
    // Implementation would go here
}

bool ConfigManager::validate_config_structure() const {
    return config_->validate();
}

std::vector<std::string> ConfigManager::get_config_errors() const {
    std::vector<std::string> errors;

    // Add validation error messages here
    if (config_->system.frequency <= 0) {
        errors.push_back("System frequency must be positive");
    }

    return errors;
}

// Configuration section parsers
bool ConfigManager::parse_system_config(const YAML::Node& node) {
    try {
        if (node["frequency"]) {
            config_->system.frequency = node["frequency"].as<double>();
        }

        if (node["control_mode"]) {
            config_->system.control_mode = string_to_control_mode(node["control_mode"].as<std::string>());
        }

        if (node["safety_enabled"]) {
            config_->system.safety_enabled = node["safety_enabled"].as<bool>();
        }

        if (node["performance_monitoring"]) {
            config_->system.performance_monitoring = node["performance_monitoring"].as<bool>();
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to parse system configuration: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::parse_debug_config(const YAML::Node& node) {
    try {
        if (node["debug_level"]) {
            config_->debug.debug_level = string_to_debug_level(node["debug_level"].as<std::string>());
        }

        if (node["can_debug"]) {
            config_->debug.can_debug = node["can_debug"].as<bool>();
        }

        if (node["motor_debug"]) {
            config_->debug.motor_debug = node["motor_debug"].as<bool>();
        }

        if (node["system_debug"]) {
            config_->debug.system_debug = node["system_debug"].as<bool>();
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to parse debug configuration: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::parse_logging_config(const YAML::Node& node) {
    try {
        // Implement logging configuration parsing
        return true;
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to parse logging configuration: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::parse_motors_config(const YAML::Node& node) {
    try {
        // Implement motor configuration parsing
        return true;
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to parse motors configuration: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::parse_communication_config(const YAML::Node& node) {
    try {
        // Implement communication configuration parsing
        return true;
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to parse communication configuration: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::parse_features_config(const YAML::Node& node) {
    try {
        // Implement features configuration parsing
        return true;
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to parse features configuration: " << e.what() << std::endl;
        return false;
    }
}

} // namespace ic_can