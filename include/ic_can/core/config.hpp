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
#include <map>
#include <chrono>
#include <memory>

// Forward declaration for YAML namespace
namespace YAML {
    class Node;
}

namespace ic_can {

/**
 * @brief Debug level enumeration
 */
enum class DebugLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

/**
 * @brief Log format enumeration
 */
enum class LogFormat {
    CSV = 0,
    JSON = 1,
    HUMAN_READABLE = 2
};

/**
 * @brief Control mode enumeration
 */
enum class ControlMode {
    TEACH_MODE,
    EXECUTION_MODE
};

/**
 * @brief Communication backend enumeration
 */
enum class CommunicationBackend {
    ZLG_CAN_FD,
    DM_TOOLS,
    SIMULATION,
    AUTO
};

/**
 * @brief System-wide configuration
 */
struct SystemConfig {
    double frequency = 500.0;                          ///< Control frequency in Hz
    ControlMode control_mode = ControlMode::EXECUTION_MODE;
    bool safety_enabled = true;
    bool performance_monitoring = true;
    std::chrono::milliseconds command_timeout{100};    ///< Command timeout in milliseconds
    std::chrono::milliseconds receive_timeout{50};     ///< Receive timeout in milliseconds
};

/**
 * @brief Debug configuration
 */
struct DebugConfig {
    DebugLevel debug_level = DebugLevel::INFO;
    bool can_debug = false;
    bool motor_debug = false;
    bool system_debug = true;
    bool communication_debug = false;
    bool performance_debug = false;
};

/**
 * @brief Logger configuration for different log types
 */
struct LoggerConfig {
    bool enabled = false;
    std::string directory = "./logs";
    LogFormat format = LogFormat::CSV;
    double frequency = 50.0;                            ///< Logging frequency in Hz
    size_t rotation_size_mb = 100;                      ///< Log file rotation size in MB
    bool timestamp_enabled = true;
    std::string custom_prefix;                          ///< Optional prefix for log files
};

/**
 * @brief Complete logging configuration
 */
struct LoggingConfig {
    LoggerConfig can_logger;
    LoggerConfig states_logger;
    LoggerConfig cmd_logger;
    bool unified_logging = false;                        ///< Use single log file for all types
    std::string unified_log_filename = "ic_can_unified.log";
};

/**
 * @brief Motor gains configuration
 */
struct MotorGains {
    double kp = 50.0;                                   ///< Proportional gain
    double kd = 5.0;                                    ///< Derivative gain
    double ki = 0.0;                                    ///< Integral gain (if needed)
};

/**
 * @brief Motor limits configuration
 */
struct MotorLimits {
    std::vector<double> position_limits = {-3.14159, 3.14159};  ///< Min, max position in radians
    std::vector<double> velocity_limits = {-10.0, 10.0};         ///< Min, max velocity in rad/s
    std::vector<double> torque_limits = {-50.0, 50.0};           ///< Min, max torque in Nm
    double max_acceleration = 20.0;                               ///< Max acceleration in rad/s^2
};

/**
 * @brief Friction compensation parameters
 */
struct FrictionParams {
    double Fc = 1.0;                                     ///< Coulomb friction coefficient
    double Fv = 0.5;                                     ///< Viscous friction coefficient
    double Fs = 1.2;                                     ///< Static friction coefficient
    double velocity_threshold = 0.1;                     ///< Velocity threshold for friction compensation
    double stribeck_velocity = 0.01;                     ///< Stribeck velocity parameter
    bool smooth_transition = true;                       ///< Enable smooth friction transitions
};

/**
 * @brief Individual motor configuration
 */
struct MotorConfig {
    int joint = 1;                                       ///< Joint number (1-9)
    std::string type = "DM10010L";                       ///< Motor type string
    uint32_t can_send_id = 0x201;                        ///< CAN send ID
    uint32_t can_receive_id = 0x201;                     ///< CAN receive ID
    MotorGains gains;
    MotorLimits limits;
    FrictionParams friction_params;
    double zero_calibration = 0.0;                       ///< Zero position calibration offset
    bool enabled = true;                                 ///< Motor enable/disable flag
    std::string description;                             ///< Optional motor description
};

/**
 * @brief Motor groups configuration
 */
struct MotorsConfig {
    std::vector<MotorConfig> arm_motors;                 ///< Motors 1-6 (arm joints)
    std::vector<MotorConfig> wrist_motors;               ///< Motors 7-8 (wrist joints)
    std::vector<MotorConfig> gripper_motors;             ///< Motor 9 (gripper servo)

    // Convenience accessors
    std::vector<MotorConfig> all_motors() const;
    MotorConfig get_motor(int joint) const;
    bool validate() const;
};

/**
 * @brief ZLG CAN FD communication configuration
 */
struct ZLGCanFDConfig {
    bool enabled = true;
    std::string device_serial = "";                      ///< Device serial number (empty = auto-detect)
    int channel = -1;                                    ///< Channel index (-1 = auto-detect)
    uint32_t arbitration_baud = 1000000;                 ///< Arbitration phase baud rate
    uint32_t data_baud = 5000000;                        ///< Data phase baud rate
    std::string library_path = "";                       ///< Custom library path
    bool auto_detect_device = true;
    bool can_fd_mode = true;
    bool classic_can_fallback = true;
    bool enable_termination = true;
    bool debug_enabled = false;
};

/**
 * @brief DM Tools communication configuration
 */
struct DMToolsConfig {
    bool enabled = true;
    std::string device_serial = "";                      ///< Device serial number
    int channel = -1;                                    ///< Channel index (-1 = auto-detect)
    bool auto_detect_device = true;
    bool debug_enabled = false;
    std::string sdk_path = "";                           ///< Custom SDK path
};

/**
 * @brief Simulation backend configuration
 */
struct SimulationConfig {
    bool enabled = false;
    double update_frequency = 500.0;                     ///< Simulation update frequency
    std::string simulation_model = "default";            ///< Simulation model type
    bool noise_simulation = false;                       ///< Enable sensor noise simulation
    double noise_std_dev = 0.001;                        ///< Noise standard deviation
};

/**
 * @brief Complete communication configuration
 */
struct CommunicationConfig {
    CommunicationBackend preferred_backend = CommunicationBackend::ZLG_CAN_FD;
    std::vector<CommunicationBackend> fallback_backends = {
        CommunicationBackend::DM_TOOLS,
        CommunicationBackend::SIMULATION,
        CommunicationBackend::AUTO
    };

    ZLGCanFDConfig zlg_config;
    DMToolsConfig dm_tools_config;
    SimulationConfig simulation_config;

    bool validate() const;
};

/**
 * @brief Gravity compensation configuration
 */
struct GravityCompensationConfig {
    bool enabled = true;
    std::string method = "pinocchio";                     ///< "pinocchio", "regressor", "pure_c"
    std::vector<double> gravity_vector = {0.0, 0.0, -9.81}; ///< Gravity vector in world frame
    std::string robot_model_path = "";                   ///< Path to robot model file
    bool include_motor_inertia = true;
};

/**
 * @brief Friction compensation configuration
 */
struct FrictionCompensationConfig {
    bool enabled = true;
    std::string method = "stribeck";                     ///< "stribeck", "coulomb", "viscous"
    bool smooth_transition = true;
    double sgn_threshold = 0.01;                         ///< Threshold for sign function smoothing
    bool adaptive_friction = false;                      ///< Enable adaptive friction estimation
    double adaptation_rate = 0.01;                       ///< Friction parameter adaptation rate
};

/**
 * @brief Trajectory execution configuration
 */
struct TrajectoryExecutionConfig {
    bool enabled = true;
    std::string interpolation_method = "linear";        ///< "linear", "cubic", "quintic"
    double max_velocity = 1.0;                           ///< Maximum trajectory velocity in rad/s
    double max_acceleration = 2.0;                       ///< Maximum trajectory acceleration in rad/s^2
    bool servo_100x_control = false;                     ///< Enable 1:100 servo control ratio
    double smoothing_factor = 0.0;                       ///< Trajectory preprocessing smoothing (0-1)
    bool continuity_check = true;                        ///< Check trajectory continuity
    double max_position_jump = 0.1;                      ///< Maximum allowed position jump
};

/**
 * @brief Safety monitoring configuration
 */
struct SafetyMonitoringConfig {
    bool enabled = true;
    bool collision_detection = false;
    bool torque_limiting = true;
    bool velocity_limiting = true;
    bool position_limiting = true;
    std::vector<double> torque_limits = {80.0, 80.0, 80.0, 80.0, 80.0, 80.0, 30.0, 30.0, 15.0};
    std::vector<double> velocity_limits = {3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0};
    std::vector<double> position_limits = {-3.14, -3.14, -3.14, -3.14, -3.14, -3.14, -3.14, -3.14, -3.14};
    double emergency_stop_threshold = 0.5;               ///< Emergency stop trigger threshold
    std::chrono::milliseconds safety_check_interval{10}; ///< Safety monitoring interval
};

/**
 * @brief Advanced features configuration
 */
struct FeaturesConfig {
    GravityCompensationConfig gravity_compensation;
    FrictionCompensationConfig friction_compensation;
    TrajectoryExecutionConfig trajectory_execution;
    SafetyMonitoringConfig safety_monitoring;

    // Feature flags for experimental or optional features
    bool adaptive_control = false;
    bool learning_enabled = false;
    bool predictive_control = false;
    bool remote_monitoring = false;
};

/**
 * @brief Complete IC_CAN configuration structure
 */
struct IC_CANConfig {
    SystemConfig system;
    DebugConfig debug;
    LoggingConfig logging;
    MotorsConfig motors;
    CommunicationConfig communication;
    FeaturesConfig features;

    // Metadata
    std::string config_version = "1.0.0";
    std::string config_description = "IC_CAN Robot Control Configuration";
    std::chrono::system_clock::time_point last_modified;

    // Validation and utilities
    bool validate() const;
    void set_defaults();
    std::map<std::string, std::string> get_summary() const;
};

/**
 * @brief Configuration Manager Class
 *
 * Provides centralized configuration management for the IC_CAN system.
 * Handles loading, validation, and access to configuration parameters.
 */
class ConfigManager {
public:
    /**
     * @brief Constructor
     * @param config_file Path to configuration YAML file (optional)
     */
    explicit ConfigManager(const std::string& config_file = "");

    /**
     * @brief Destructor
     */
    ~ConfigManager();

    /**
     * @brief Load configuration from YAML file
     * @param config_file Path to configuration file
     * @return true if loaded successfully
     */
    bool load_config(const std::string& config_file);

    /**
     * @brief Save current configuration to YAML file
     * @param config_file Path to output file
     * @return true if saved successfully
     */
    bool save_config(const std::string& config_file) const;

    /**
     * @brief Reload configuration from file
     * @return true if reloaded successfully
     */
    bool reload_config();

    /**
     * @brief Get complete configuration
     * @return Reference to configuration structure
     */
    const IC_CANConfig& get_config() const;

    /**
     * @brief Get system configuration
     * @return Reference to system configuration
     */
    const SystemConfig& get_system_config() const;

    /**
     * @brief Get motor configuration
     * @return Reference to motor configuration
     */
    const MotorsConfig& get_motors_config() const;

    /**
     * @brief Get communication configuration
     * @return Reference to communication configuration
     */
    const CommunicationConfig& get_communication_config() const;

    /**
     * @brief Get logging configuration
     * @return Reference to logging configuration
     */
    const LoggingConfig& get_logging_config() const;

    /**
     * @brief Get debug configuration
     * @return Reference to debug configuration
     */
    const DebugConfig& get_debug_config() const;

    /**
     * @brief Get features configuration
     * @return Reference to features configuration
     */
    const FeaturesConfig& get_features_config() const;

    /**
     * @brief Set configuration value (for runtime updates)
     * @param key Configuration key (dot notation, e.g., "system.frequency")
     * @param value Configuration value
     * @return true if set successfully
     */
    bool set_config_value(const std::string& key, const std::string& value);

    /**
     * @brief Get configuration value as string
     * @param key Configuration key (dot notation)
     * @return Configuration value or empty string if not found
     */
    std::string get_config_value(const std::string& key) const;

    /**
     * @brief Validate current configuration
     * @return true if configuration is valid
     */
    bool validate_config() const;

    /**
     * @brief Get configuration file path
     * @return Current configuration file path
     */
    const std::string& get_config_file_path() const;

    /**
     * @brief Print configuration summary
     */
    void print_config_summary() const;

    /**
     * @brief Check if configuration has been loaded
     * @return true if configuration is loaded
     */
    bool is_config_loaded() const;

    /**
     * @brief Create default configuration file
     * @param config_file Path where to create the file
     * @return true if created successfully
     */
    bool create_default_config(const std::string& config_file) const;

    /**
     * @brief Merge configuration from another file
     * @param config_file Path to configuration file to merge
     * @return true if merged successfully
     */
    bool merge_config(const std::string& config_file);

private:
    std::unique_ptr<IC_CANConfig> config_;
    std::string config_file_path_;
    bool config_loaded_;

    // Internal helper methods
    bool parse_yaml_file(const std::string& file_path);
    bool write_yaml_file(const std::string& file_path) const;
    void apply_environment_substitution();
    bool validate_config_structure() const;
    std::vector<std::string> get_config_errors() const;

    // Configuration section parsers
    bool parse_system_config(const YAML::Node& node);
    bool parse_debug_config(const YAML::Node& node);
    bool parse_logging_config(const YAML::Node& node);
    bool parse_motors_config(const YAML::Node& node);
    bool parse_communication_config(const YAML::Node& node);
    bool parse_features_config(const YAML::Node& node);
};

/**
 * @brief Convert debug level to string
 * @param level Debug level enum
 * @return String representation
 */
std::string debug_level_to_string(DebugLevel level);

/**
 * @brief Convert string to debug level
 * @param level_str String representation
 * @return Debug level enum
 */
DebugLevel string_to_debug_level(const std::string& level_str);

/**
 * @brief Convert log format to string
 * @param format Log format enum
 * @return String representation
 */
std::string log_format_to_string(LogFormat format);

/**
 * @brief Convert string to log format
 * @param format_str String representation
 * @return Log format enum
 */
LogFormat string_to_log_format(const std::string& format_str);

/**
 * @brief Convert control mode to string
 * @param mode Control mode enum
 * @return String representation
 */
std::string control_mode_to_string(ControlMode mode);

/**
 * @brief Convert string to control mode
 * @param mode_str String representation
 * @return Control mode enum
 */
ControlMode string_to_control_mode(const std::string& mode_str);

/**
 * @brief Convert communication backend to string
 * @param backend Communication backend enum
 * @return String representation
 */
std::string communication_backend_to_string(CommunicationBackend backend);

/**
 * @brief Convert string to communication backend
 * @param backend_str String representation
 * @return Communication backend enum
 */
CommunicationBackend string_to_communication_backend(const std::string& backend_str);

} // namespace ic_can