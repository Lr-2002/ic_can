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
#include <functional>

namespace ic_can {

// Forward declarations
class CANCommunicationInterface;
class ZLGCanFDCommunicationAdapter;
class USB2CANCommunicationAdapter;

/**
 * @brief Communication Backend Types
 *
 * Enumeration of available CAN communication backends that can be selected
 * at runtime or compile time.
 */
enum class CommunicationBackend {
    AUTO,           ///< Auto-detect best available backend
    DM_TOOLS,       ///< DM Tools USB2CAN adapter (legacy)
    ZLG_CANFD,      ///< ZLG CAN FD adapter (preferred)
    SIMULATION      ///< Simulation mode (for testing)
};

/**
 * @brief Communication Backend Information
 *
 * Structure containing information about available communication backends
 * and their capabilities.
 */
struct CommunicationBackendInfo {
    CommunicationBackend type;
    std::string name;
    std::string description;
    bool available;
    std::map<std::string, std::string> capabilities;
    std::string device_info;
};

/**
 * @brief Communication Factory Configuration
 *
 * Configuration structure for creating communication adapters with specific
 * backend preferences and settings.
 */
struct CommunicationFactoryConfig {
    // Backend Selection
    CommunicationBackend preferred_backend = CommunicationBackend::ZLG_CANFD;
    std::vector<CommunicationBackend> fallback_backends = {
        CommunicationBackend::ZLG_CANFD  // ONLY ZLG - no fallback
    };

    // Device Configuration
    std::string device_serial = "";           ///< Device serial number (optional)
    int preferred_channel = 0;                 ///< Preferred channel index (0 = real channel 1)
    bool auto_detect_device = true;            ///< Auto-detect available devices

    // ZLG CAN FD Specific Configuration
    bool zlg_can_fd_mode = true;              ///< Enable CAN FD mode for ZLG
    uint32_t zlg_arbitration_baud = 1000000; ///< ZLG arbitration baud rate
    uint32_t zlg_data_baud = 5000000;        ///< ZLG data baud rate

    // Performance Configuration
    bool enable_performance_monitoring = true; ///< Enable performance monitoring
    uint32_t receive_timeout_ms = 100;        ///< Receive timeout
    uint32_t send_timeout_ms = 50;            ///< Send timeout

    // Library Paths
    std::string zlg_library_path = "/home/lr-2002/project/InstantCreation/ic_can/lib/zlg_canfd/libcontrolcanfd.so";
    std::string dm_tools_path = "";           ///< dm-tools path (optional)

    // Debug and Logging
    bool debug_enabled = false;              ///< Enable debug output
    bool verbose_logging = false;            ///< Enable verbose logging
};

/**
 * @brief CAN Communication Factory
 *
 * Factory class for creating CAN communication adapters with support for
 * multiple backend types and runtime selection. This factory provides:
 *
 * - Auto-detection of available hardware backends
 * - Runtime switching between different communication adapters
 * - Graceful fallback when preferred backend is unavailable
 * - Centralized configuration management
 * - Performance comparison between backends
 */
class CANCommunicationFactory {
public:
    /**
     * @brief Create a communication adapter with auto-detection
     * @param config Factory configuration
     * @return Unique pointer to communication adapter
     *
     * This method will automatically detect available backends and create
     * the best available adapter based on the configuration preferences.
     */
    static std::unique_ptr<CANCommunicationInterface> create_adapter(
        const CommunicationFactoryConfig& config = CommunicationFactoryConfig{});

    /**
     * @brief Create a communication adapter with specific backend
     * @param backend Specific backend type to use
     * @param config Factory configuration
     * @return Unique pointer to communication adapter
     *
     * This method forces the use of a specific backend type and will fail
     * if that backend is not available.
     */
    static std::unique_ptr<CANCommunicationInterface> create_adapter(
        CommunicationBackend backend,
        const CommunicationFactoryConfig& config = CommunicationFactoryConfig{});

    /**
     * @brief Get information about available backends
     * @param config Factory configuration
     * @return Vector of backend information
     */
    static std::vector<CommunicationBackendInfo> get_available_backends(
        const CommunicationFactoryConfig& config = CommunicationFactoryConfig{});

    /**
     * @brief Check if a specific backend is available
     * @param backend Backend type to check
     * @param config Factory configuration
     * @return true if backend is available
     */
    static bool is_backend_available(
        CommunicationBackend backend,
        const CommunicationFactoryConfig& config = CommunicationFactoryConfig{});

    /**
     * @brief Test all available backends
     * @param config Factory configuration
     * @param test_duration_ms Duration for each test in milliseconds
     * @return Map of backend names to test results
     */
    static std::map<std::string, bool> test_backends(
        const CommunicationFactoryConfig& config = CommunicationFactoryConfig{},
        uint32_t test_duration_ms = 5000);

    /**
     * @brief Compare performance of available backends
     * @param config Factory configuration
     * @param test_duration_ms Duration for performance test
     * @return Performance comparison results
     */
    static std::map<std::string, std::map<std::string, double>> compare_backend_performance(
        const CommunicationFactoryConfig& config = CommunicationFactoryConfig{},
        uint32_t test_duration_ms = 10000);

    /**
     * @brief Get recommended backend for current system
     * @param config Factory configuration
     * @return Recommended backend type
     */
    static CommunicationBackend get_recommended_backend(
        const CommunicationFactoryConfig& config = CommunicationFactoryConfig{});

    /**
     * @brief Set global factory configuration
     * @param config Default configuration for factory
     *
     * This configuration will be used for subsequent factory calls
     * when no specific configuration is provided.
     */
    static void set_default_config(const CommunicationFactoryConfig& config);

    /**
     * @brief Get global factory configuration
     * @return Current default configuration
     */
    static CommunicationFactoryConfig get_default_config();

    // ===================================
    // CONVENIENCE FACTORY METHODS
    // ===================================

    /**
     * @brief Create ZLG CAN FD adapter
     * @param device_serial Device serial number (optional)
     * @param channel_index Channel index (-1 = auto)
     * @param can_fd_mode Enable CAN FD mode
     * @return Unique pointer to ZLG CAN FD adapter
     */
    static std::unique_ptr<CANCommunicationInterface> create_zlg_canfd_adapter(
        const std::string& device_serial = "",
        int channel_index = -1,
        bool can_fd_mode = true);

    /**
     * @brief Create DM Tools adapter
     * @param device_serial Device serial number
     * @return Unique pointer to DM Tools adapter
     */
    static std::unique_ptr<CANCommunicationInterface> create_dm_tools_adapter(
        const std::string& device_serial = "693D3DE86DF5940C8BC74A5B46A3CE2E");

    /**
     * @brief Create simulation adapter
     * @param loopback Enable message loopback
     * @return Unique pointer to simulation adapter
     */
    static std::unique_ptr<CANCommunicationInterface> create_simulation_adapter(
        bool loopback = true);

    /**
     * @brief Create adapter based on string name
     * @param backend_name Backend name ("zlg_canfd", "dm_tools", "simulation", "auto")
     * @param config Factory configuration
     * @return Unique pointer to communication adapter
     */
    static std::unique_ptr<CANCommunicationInterface> create_adapter_by_name(
        const std::string& backend_name,
        const CommunicationFactoryConfig& config = CommunicationFactoryConfig{});

private:
    // Internal factory methods
    static std::unique_ptr<CANCommunicationInterface> create_zlg_canfd_adapter_internal(
        const CommunicationFactoryConfig& config);
    static std::unique_ptr<CANCommunicationInterface> create_dm_tools_adapter_internal(
        const CommunicationFactoryConfig& config);
    static std::unique_ptr<CANCommunicationInterface> create_simulation_adapter_internal(
        const CommunicationFactoryConfig& config);

    // Backend detection and testing
    static bool detect_zlg_canfd(const CommunicationFactoryConfig& config);
    static bool detect_dm_tools(const CommunicationFactoryConfig& config);
    static bool test_backend_performance(
        CommunicationBackend backend,
        const CommunicationFactoryConfig& config,
        uint32_t test_duration_ms,
        std::map<std::string, double>& results);

    // Utility methods
    static std::string backend_to_string(CommunicationBackend backend);
    static CommunicationBackend string_to_backend(const std::string& backend_name);
    static CommunicationBackendInfo create_backend_info(
        CommunicationBackend backend,
        bool available,
        const std::string& device_info = "");

    // Static configuration
    static CommunicationFactoryConfig default_config_;
    static bool default_config_set_;
};

/**
 * @brief Communication Backend Registry
 *
 * Registry for registering and managing communication backends.
 * This allows for extensibility and plugin-like architecture.
 */
class CommunicationBackendRegistry {
public:
    /**
     * @brief Register a new backend type
     * @param backend Backend type identifier
     * @param name Human-readable name
     * @param description Backend description
     * @param factory_func Factory function for creating backend
     * @return true if registration successful
     */
    static bool register_backend(
        CommunicationBackend backend,
        const std::string& name,
        const std::string& description,
        std::function<std::unique_ptr<CANCommunicationInterface>(const CommunicationFactoryConfig&)> factory_func);

    /**
     * @brief Unregister a backend
     * @param backend Backend type to unregister
     * @return true if unregistration successful
     */
    static bool unregister_backend(CommunicationBackend backend);

    /**
     * @brief Get all registered backends
     * @return Map of registered backends
     */
    static std::map<CommunicationBackend, std::string> get_registered_backends();

private:
    static std::map<CommunicationBackend, std::function<std::unique_ptr<CANCommunicationInterface>(const CommunicationFactoryConfig&)>>
        registered_backends_;
    static std::map<CommunicationBackend, std::string> backend_names_;
};

/**
 * @brief Convenience function for quick adapter creation
 *
 * This function provides a simple interface for creating communication
 * adapters with sensible defaults, making it easy to use in most cases.
 *
 * @param backend_type Backend type ("auto", "zlg", "dm_tools", "simulation")
 * @param device_serial Device serial number (for hardware backends)
 * @return Unique pointer to communication adapter
 */
std::unique_ptr<CANCommunicationInterface> create_can_adapter(
    const std::string& backend_type = "auto",
    const std::string& device_serial = "");

} // namespace ic_can