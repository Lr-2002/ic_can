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

/**
 * @file can_bus_logger.hpp
 * @brief CAN Bus Communication Logger
 *
 * Standalone logger for monitoring CAN bus traffic:
 * - Logs all sent CAN frames with timestamps
 * - Logs all received CAN frames with timestamps
 * - Provides real-time statistics
 * - CSV export capabilities
 * - Per-motor frame counting
 */

#pragma once

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <atomic>

namespace ic_can {

/**
 * @brief CAN Frame Structure
 */
struct CANFrameLog {
    std::chrono::high_resolution_clock::time_point timestamp;
    std::string direction;  // "TX" or "RX"
    uint32_t can_id;
    bool extended_id;
    uint8_t dlc;
    std::vector<uint8_t> data;

    CANFrameLog() = default;

    CANFrameLog(const std::string& dir, uint32_t id, bool ext, uint8_t len,
                const std::vector<uint8_t>& payload)
        : timestamp(std::chrono::high_resolution_clock::now()),
          direction(dir), can_id(id), extended_id(ext), dlc(len), data(payload) {}
};

/**
 * @brief CAN Bus Logger
 *
 * Standalone logger for monitoring CAN bus communication.
 * Tracks all sent and received CAN frames with timing information.
 */
class CANBusLogger {
public:
    /**
     * @brief Constructor
     * @param log_directory Directory to save log files
     */
    explicit CANBusLogger(const std::string& log_directory = "");

    /**
     * @brief Destructor - automatically saves logs
     */
    ~CANBusLogger();

    /**
     * @brief Start logging
     * @return true if logging started successfully
     */
    bool start_logging();

    /**
     * @brief Stop logging
     */
    void stop_logging();

    /**
     * @brief Log a sent CAN frame
     * @param can_id CAN ID
     * @param extended_id Whether this is an extended ID frame
     * @param dlc Data length code
     * @param data Frame data
     */
    void log_sent_frame(uint32_t can_id, bool extended_id, uint8_t dlc,
                        const std::vector<uint8_t>& data);

    /**
     * @brief Log a received CAN frame
     * @param can_id CAN ID
     * @param extended_id Whether this is an extended ID frame
     * @param dlc Data length code
     * @param data Frame data
     */
    void log_received_frame(uint32_t can_id, bool extended_id, uint8_t dlc,
                           const std::vector<uint8_t>& data);

    /**
     * @brief Get statistics
     */
    struct Statistics {
        uint64_t total_sent = 0;
        uint64_t total_received = 0;
        std::map<uint32_t, uint64_t> sent_per_id;
        std::map<uint32_t, uint64_t> received_per_id;
        double sent_frequency = 0.0;
        double received_frequency = 0.0;
        std::chrono::duration<double> uptime{0};
    };

    /**
     * @brief Get current statistics
     * @return Current statistics
     */
    Statistics get_statistics() const;

    /**
     * @brief Print current statistics
     */
    void print_statistics() const;

    /**
     * @brief Get motor ID from CAN ID
     * @param can_id CAN ID
     * @return Motor ID (1-9) or 0 if unknown
     */
    int can_id_to_motor_id(uint32_t can_id) const;

    /**
     * @brief Check if logging is active
     * @return true if logging is active
     */
    bool is_logging() const { return logging_active_.load(); }

private:
    // Logging state
    std::atomic<bool> logging_active_;
    std::string log_directory_;
    std::chrono::high_resolution_clock::time_point start_time_;

    // Frame storage
    std::vector<CANFrameLog> frame_buffer_;
    mutable std::mutex buffer_mutex_;

    // File handles
    std::ofstream can_log_file_;
    std::ofstream csv_log_file_;

    // Initialize logging system
    bool initialize_logging();

    // Create log directory
    bool create_log_directory();

    // Open log files
    bool open_log_files();

    // Close log files
    void close_log_files();

    // Write frame to files
    void write_frame_to_files(const CANFrameLog& frame);

    // Format frame for logging
    std::string format_frame(const CANFrameLog& frame) const;

    // Format frame for CSV
    std::string format_frame_csv(const CANFrameLog& frame) const;

    // Update statistics
    void update_statistics(const CANFrameLog& frame);
};

} // namespace ic_can