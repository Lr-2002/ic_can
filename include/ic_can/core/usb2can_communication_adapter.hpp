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

#include "ic_can/core/can_protocol_interface.hpp"
#include <functional>
#include <memory>
#include <string>

// Forward declarations for dm-tools types
class usb_class;

namespace ic_can {

/**
 * @brief USB2CAN Communication Adapter
 *
 * Bridges between dm-tools SDK's usb_class and the new CANCommunicationInterface.
 * This adapter allows the central dispatcher to work with the existing USB2CAN hardware
 * while maintaining compatibility with the new communication architecture.
 */
class USB2CANCommunicationAdapter : public CANCommunicationInterface {
public:
    /**
     * @brief Constructor
     * @param device_sn USB2CAN device serial number
     * @param debug Enable debug output
     */
    explicit USB2CANCommunicationAdapter(const std::string& device_sn, bool debug = false);

    /**
     * @brief Destructor
     */
    virtual ~USB2CANCommunicationAdapter();

    // ========== CANCommunicationInterface Implementation ==========

    /**
     * @brief Send CAN frame through USB2CAN hardware
     * @param frame CAN frame to send
     * @return True if frame was sent successfully
     */
    bool send_can_frame(const CANFrame& frame) override;

    /**
     * @brief Set frame received callback
     * @param callback Callback function for received frames
     */
    void set_frame_received_callback(std::function<void(const CANFrame&)> callback) override;

    /**
     * @brief Start CAN communication
     * @return True if communication started successfully
     */
    bool start_communication() override;

    /**
     * @brief Stop CAN communication
     * @return True if communication stopped successfully
     */
    bool stop_communication() override;

    /**
     * @brief Check if communication is active
     * @return True if communication is active
     */
    bool is_communication_active() const;

    /**
     * @brief Get communication statistics
     * @return Statistics about sent/received frames
     */
    CommunicationStats get_statistics() const override;

    // ========== USB2CAN Specific Methods ==========

    /**
     * @brief Initialize USB2CAN connection
     * @return True if initialization successful
     */
    bool initialize();

    /**
     * @brief Check if device is connected
     * @return True if device is connected
     */
    bool is_connected() const;

    /**
     * @brief Get device serial number
     * @return Device serial number
     */
    const std::string& get_device_serial() const;

    /**
     * @brief Enable/disable debug output
     * @param debug Debug flag
     */
    void set_debug_enabled(bool debug);

private:
    // ========== Private Members ==========

    std::string device_sn_;                                    // Device serial number
    bool debug_enabled_;                                       // Debug output flag
    std::unique_ptr<usb_class> usb_device_;                   // USB2CAN device instance
    std::function<void(const CANFrame&)> frame_callback_;    // Frame received callback
    bool is_connected_;                                        // Connection status
    bool is_communicating_;                                     // Communication status

    // Statistics
    mutable std::mutex stats_mutex_;
    uint64_t frames_sent_ = 0;
    uint64_t frames_received_ = 0;
    uint64_t bytes_sent_ = 0;
    uint64_t bytes_received_ = 0;
    std::chrono::steady_clock::time_point start_time_;

    // ========== Private Methods ==========

    /**
     * @brief Convert CANFrame to can_value_type (dm-tools format)
     * @param frame New CAN frame format
     * @return dm-tools frame format
     */
    can_value_type convert_to_usb_frame(const CANFrame& frame) const;

    /**
     * @brief Convert can_value_type (dm-tools format) to CANFrame
     * @param usb_frame dm-tools frame format
     * @return New CAN frame format
     */
    CANFrame convert_from_usb_frame(const can_value_type& usb_frame) const;

    /**
     * @brief Internal frame received handler from dm-tools
     * @param frame Received frame from dm-tools
     */
    void on_frame_received(can_value_type& frame);

    /**
     * @brief Debug print function
     * @param message Debug message
     */
    void debug_print(const std::string& message) const;

    /**
     * @brief Update statistics
     * @param bytes_sent Bytes sent (0 if not applicable)
     * @param bytes_received Bytes received (0 if not applicable)
     */
    void update_statistics(size_t bytes_sent = 0, size_t bytes_received = 0);
};

} // namespace ic_can