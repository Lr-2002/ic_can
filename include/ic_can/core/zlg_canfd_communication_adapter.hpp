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

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <mutex>
#include <thread>
#include <array>

#include "ic_can/core/can_protocol_interface.hpp"

// Include ZLG CAN FD API definitions
#include "/home/lr-2002/project/InstantCreation/ic_can/lib/zlg_canfd/controlcanfd.h"

namespace ic_can {

/**
 * @brief ZLG CAN FD Communication Adapter Configuration
 *
 * Configuration structure for ZLG CAN FD devices with flexible channel selection
 * and CAN FD mode support.
 */
struct ZLGCanFDConfig {
    // Device Configuration
    uint32_t device_type = 41;              ///< Device type (will be set from controlcanfd.h)
    uint32_t device_index = 0;              ///< Device index (0 for first device)

    // Channel Configuration
    uint32_t channel_index = 0;              ///< CAN channel index (0-15, configurable at runtime)
    bool auto_channel_selection = true;      ///< Automatically find available channel

    // Baud Rate Configuration
    uint32_t arbitration_baud = 1000000;     ///< Arbitration phase baud rate (1 Mbps)
    uint32_t data_baud = 5000000;            ///< Data phase baud rate (5 Mbps)

    // CAN FD Configuration
    bool can_fd_mode = true;                 ///< Enable CAN FD mode (64-byte frames)
    bool classic_can_fallback = true;        ///< Fallback to classic CAN for 8-byte frames

    // Timing Configuration
    uint32_t receive_timeout_ms = 100;       ///< Receive timeout in milliseconds
    uint32_t send_timeout_ms = 50;           ///< Send timeout in milliseconds

    // Library Configuration
    std::string library_path = "";  ///< Path to ZLG library (empty to use config)

    // Debug Configuration
    bool debug = false;                                                     ///< Enable debug output
    bool use_direct_library_calls = false;                                 ///< Use direct library calls (better debugging)

    // CAN FD Options
    bool force_can_fd = false;                                           ///< Force CAN FD mode for all frames

    // Advanced Options
    bool enable_termination = true;          ///< Enable 120Ω termination
    bool enable_filtering = false;           ///< Enable hardware filtering
    uint32_t filter_start_id = 0;            ///< Filter start ID (if enabled)
    uint32_t filter_end_id = 0x7FF;          ///< Filter end ID (if enabled)
};

/**
 * @brief ZLG CAN FD Communication Performance Statistics
 */
struct ZLGCanFDPerformanceStats {
    // Basic Statistics
    uint64_t frames_sent = 0;
    uint64_t frames_received = 0;
    uint64_t send_errors = 0;
    uint64_t receive_errors = 0;
    uint64_t timeout_errors = 0;

    // Timing Statistics
    double average_send_latency_us = 0.0;
    double average_receive_interval_us = 0.0;
    double max_send_latency_us = 0.0;
    double max_receive_interval_us = 0.0;

    // Channel Statistics
    uint32_t device_errors = 0;
    uint32_t channel_errors = 0;
    uint32_t buffer_overruns = 0;

    // Performance Metrics
    double actual_send_frequency = 0.0;
    double actual_receive_frequency = 0.0;
    double bus_load_percentage = 0.0;

    // Timestamps
    std::chrono::high_resolution_clock::time_point last_send_time;
    std::chrono::high_resolution_clock::time_point last_receive_time;
    std::chrono::high_resolution_clock::time_point statistics_start_time;
};

/**
 * @brief ZLG CAN FD Communication Adapter
 *
 * This adapter provides high-performance CAN FD communication using ZLG hardware
 * while maintaining full compatibility with the existing IC_CAN system interface.
 *
 * Key Features:
 * - Runtime channel configuration and selection
 * - CAN FD (64-byte) and classic CAN (8-byte) frame support
 * - Multi-channel device support
 * - High-precision timing and performance monitoring
 * - Automatic frame type detection and conversion
 * - Comprehensive error handling and recovery
 */
class ZLGCanFDCommunicationAdapter : public CANCommunicationInterface {
public:
    /**
     * @brief Constructor
     * @param config Configuration for ZLG CAN FD adapter
     */
    explicit ZLGCanFDCommunicationAdapter(const ZLGCanFDConfig& config = ZLGCanFDConfig{});

    /**
     * @brief Destructor - automatically cleans up resources
     */
    ~ZLGCanFDCommunicationAdapter();

    // ===================================
    // CORE COMMUNICATION INTERFACE
    // ===================================

    /**
     * @brief Initialize the ZLG CAN FD adapter
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * @brief Initialize ZLG hardware (internal method)
     * @return true if initialization successful
     */
    bool initialize_zlg_hardware();

    /**
     * @brief Shutdown the adapter and release resources
     */
    void shutdown();

    /**
     * @brief Check if adapter is initialized and ready
     * @return true if ready for communication
     */
    bool is_ready() const;

    // ===================================
    // CANCommunicationInterface IMPLEMENTATION
    // ===================================

    /**
     * @brief Initialize CAN communication
     * @param device Serial number or device identifier
     * @return true if initialization successful
     */
    bool initialize(const std::string& device = "") override;

    /**
     * @brief Check if communication is connected
     */
    bool is_connected() const override;

    /**
     * @brief Register callback for received frames
     * @param callback Function to call when frame received
     */
    void register_receive_callback(CANFrameCallback callback) override;

    /**
     * @brief Start receiving frames (non-blocking)
     * @return true if started successfully
     */
    bool start_receiving() override;

    /**
     * @brief Stop receiving frames
     */
    void stop_receiving() override;

    /**
     * @brief Get communication statistics
     */
    CommStats get_statistics() const override;

    /**
     * @brief Send a single CAN frame
     * @param frame CAN frame to send (supports both CAN FD and classic CAN)
     * @return true if sent successfully
     */
    bool send_frame(const CANFrame& frame);

    /**
     * @brief Send multiple CAN frames (batch operation)
     * @param frames Vector of CAN frames to send
     * @return Number of frames actually sent
     */
    size_t send_frames(const std::vector<CANFrame>& frames);

    /**
     * @brief Receive a single CAN frame (blocking with timeout)
     * @param frame Output CAN frame structure
     * @param timeout_ms Timeout in milliseconds
     * @return true if frame received
     */
    bool receive_frame(CANFrame& frame, int timeout_ms = 100);

    /**
     * @brief Receive multiple CAN frames (batch operation)
     * @param frames Output vector for received frames
     * @param max_frames Maximum frames to receive
     * @param timeout_ms Timeout in milliseconds
     * @return Number of frames actually received
     */
    size_t receive_frames(std::vector<CANFrame>& frames, size_t max_frames, int timeout_ms = 100);

    /**
     * @brief Set frame receive callback
     * @param callback Function to call for each received frame
     * @return true if callback set successfully
     */
    bool set_receive_callback(std::function<void(const CANFrame&)> callback);

    // ===================================
    // CHANNEL CONFIGURATION
    // ===================================

    /**
     * @brief Configure CAN channel at runtime
     * @param channel_index Channel index (0-15)
     * @param arbitration_baud Arbitration phase baud rate
     * @param data_baud Data phase baud rate
     * @return true if configuration successful
     */
    bool configure_channel(uint32_t channel_index, uint32_t arbitration_baud, uint32_t data_baud);

    /**
     * @brief Auto-detect and configure available channel
     * @return true if channel found and configured
     */
    bool auto_configure_channel();

    /**
     * @brief Get current channel configuration
     * @return Current channel index
     */
    uint32_t get_current_channel() const;

    /**
     * @brief Get available channels on device
     * @return Vector of available channel indices
     */
    std::vector<uint32_t> get_available_channels();

    /**
     * @brief Switch to different channel (runtime reconfiguration)
     * @param new_channel_index New channel index
     * @return true if switch successful
     */
    bool switch_channel(uint32_t new_channel_index);

    // ===================================
    // PERFORMANCE MONITORING
    // ===================================

    /**
     * @brief Get performance statistics
     * @return Performance statistics structure
     */
    ZLGCanFDPerformanceStats get_performance_stats() const;

    /**
     * @brief Reset performance counters
     */
    void reset_performance_stats();

    /**
     * @brief Print detailed performance statistics
     */
    void print_performance_stats() const;

    /**
     * @brief Enable/disable performance monitoring
     * @param enabled Whether to enable monitoring
     */
    void set_performance_monitoring_enabled(bool enabled);

    // ===================================
    // DEVICE INFORMATION
    // ===================================

    /**
     * @brief Get device information
     * @return Map of device information
     */
    std::map<std::string, std::string> get_device_info();

    /**
     * @brief Check if device is online and responsive
     * @return true if device is online
     */
    bool is_device_online();

    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    ZLGCanFDConfig get_current_config() const;

    /**
     * @brief Update configuration (some changes require reinitialization)
     * @param new_config New configuration
     * @return true if update successful
     */
    bool update_config(const ZLGCanFDConfig& new_config);

    // ===================================
    // ADVANCED FEATURES
    // ===================================

    /**
     * @brief Clear device buffers
     * @return true if successful
     */
    bool clear_buffers();

    /**
     * @brief Reset CAN channel
     * @return true if reset successful
     */
    bool reset_channel();

    /**
     * @brief Set hardware filtering
     * @param start_id Start of ID range
     * @param end_id End of ID range
     * @return true if filtering set successfully
     */
    bool set_hardware_filter(uint32_t start_id, uint32_t end_id);

    /**
     * @brief Disable hardware filtering
     * @return true if filtering disabled
     */
    bool disable_hardware_filter();

    /**
     * @brief Enable/disable CAN termination
     * @param enabled Whether to enable termination
     * @return true if operation successful
     */
    bool set_termination_enabled(bool enabled);

private:
    // ===================================
    // PRIVATE IMPLEMENTATION
    // ===================================

    // Dynamic library loading
    bool load_zlg_library();
    void unload_zlg_library();
    bool load_function_pointers();

    // Device and channel management
    bool open_device();
    bool close_device();
    bool init_channel();
    bool start_channel();
    bool stop_channel();
    bool reset_device();

    // Frame conversion and transmission
    ZCAN_TransmitFD_Data convert_frame_to_zlg_fd(const CANFrame& frame);
    ZCAN_ReceiveFD_Data convert_frame_from_zlg_fd(const ZCAN_ReceiveFD_Data& zlg_frame);
    bool should_use_can_fd(const CANFrame& frame);

    // Performance monitoring
    void update_send_performance(std::chrono::microseconds latency);
    void update_receive_performance();
    void calculate_bus_load();

    // Error handling
    bool handle_device_error(uint32_t error_code);
    bool handle_channel_error(uint32_t error_code);
    std::string get_error_description(uint32_t error_code);

    // Thread management
    void start_receive_thread();
    void stop_receive_thread();
    void receive_thread_function();

    // Helper methods are now in public section

    // Configuration and state
    ZLGCanFDConfig config_;
    ZLGCanFDPerformanceStats stats_;

    // Device handles
    DEVICE_HANDLE device_handle_;
    CHANNEL_HANDLE channel_handle_;

    // Library handles
    void* library_handle_;

    // Function pointers (loaded dynamically)
    void* open_device_func_;
    void* close_device_func_;
    void* init_can_func_;
    void* start_can_func_;
    void* stop_can_func_;
    void* reset_can_func_;
    void* transmit_fd_func_;
    void* receive_fd_func_;
    void* get_device_info_func_;
    void* clear_buffer_func_;
    void* set_filter_func_;
    // ... additional function pointers

    // Thread management
    std::unique_ptr<std::thread> receive_thread_;
    std::atomic<bool> receive_thread_running_;
    std::atomic<bool> performance_monitoring_enabled_;

    // Callback management
    std::function<void(const CANFrame&)> receive_callback_;

    // Synchronization
    mutable std::mutex state_mutex_;
    mutable std::mutex stats_mutex_;
    mutable std::mutex callback_mutex_;

    // State flags
    std::atomic<bool> initialized_;
    std::atomic<bool> device_open_;
    std::atomic<bool> channel_started_;

    // Performance timing
    std::chrono::high_resolution_clock::time_point last_bus_load_calculation_;
    std::array<std::chrono::microseconds, 100> send_latency_history_;
    std::array<std::chrono::microseconds, 100> receive_interval_history_;
    size_t latency_history_index_;
};

/**
 * @brief Create ZLG CAN FD adapter with default configuration
 * @param device_serial Device serial number (optional)
 * @param channel_index Channel index (optional, auto-detected if -1)
 * @return Unique pointer to adapter
 */
std::unique_ptr<ZLGCanFDCommunicationAdapter> create_zlg_canfd_adapter(
    const std::string& device_serial = "",
    int channel_index = -1);

/**
 * @brief Detect available ZLG CAN FD devices
 * @return Vector of device information
 */
std::vector<std::map<std::string, std::string>> detect_zlg_canfd_devices();

/**
 * @brief Test ZLG CAN FD adapter functionality
 * @param adapter Adapter to test
 * @param test_duration_ms Test duration in milliseconds
 * @return true if test passed
 */
bool test_zlg_canfd_adapter(ZLGCanFDCommunicationAdapter* adapter, uint32_t test_duration_ms = 5000);

} // namespace ic_can