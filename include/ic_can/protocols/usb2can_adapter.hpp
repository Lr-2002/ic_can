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
#include <functional>
#include <cstdint>

namespace ic_can {

/**
 * @brief CAN frame structure
 */
struct CANFrame {
    uint32_t id;
    std::vector<uint8_t> data;
    bool extended_id;
    uint8_t dlc;
};

/**
 * @brief USB2CAN adapter interface
 *
 * Handles communication with USB2CAN devices using dm-tools SDK
 */
class USB2CANAdapter {
public:
    /**
     * @brief Constructor
     * @param device_sn USB device serial number
     * @param debug Enable debug output
     */
    explicit USB2CANAdapter(const std::string& device_sn, bool debug = false);

    /**
     * @brief Destructor
     */
    ~USB2CANAdapter();

    /**
     * @brief Initialize the adapter
     * @return true if successful
     */
    bool initialize();

    /**
     * @brief Shutdown the adapter
     */
    void shutdown();

    /**
     * @brief Check if connected
     * @return true if connected
     */
    bool is_connected() const;

    /**
     * @brief Send CAN frame
     * @param data Frame data
     * @param can_id CAN ID
     * @return true if successful
     */
    bool send_frame(const std::vector<uint8_t>& data, uint32_t can_id);

    /**
     * @brief Set frame receive callback
     * @param callback Callback function
     */
    void set_frame_callback(std::function<void(const CANFrame&)> callback);

    /**
     * @brief Start data capture
     * @return true if successful
     */
    bool start_capture();

    /**
     * @brief Stop data capture
     */
    void stop_capture();

    /**
     * @brief Get device serial number
     * @return Device serial number
     */
    const std::string& get_device_sn() const;

    /**
     * @brief Enable/disable debug output
     * @param enabled Debug enabled
     */
    void set_debug_enabled(bool enabled);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ic_can