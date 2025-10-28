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
#include <memory>
#include <functional>
#include <cstdint>
#include <string>

namespace ic_can {

/**
 * @brief CAN frame structure
 */
struct CANFrame {
    uint32_t id;        // CAN ID
    std::vector<uint8_t> data;  // Data payload
    bool is_extended_id;        // Extended ID flag
    bool is_remote_frame;       // Remote frame flag

    CANFrame() : id(0), is_extended_id(false), is_remote_frame(false) {}
    CANFrame(uint32_t can_id, const std::vector<uint8_t>& frame_data,
             bool extended = false, bool remote = false)
        : id(can_id), data(frame_data), is_extended_id(extended), is_remote_frame(remote) {}
};

/**
 * @brief CAN protocol interface for dependency inversion
 *
 * This interface defines the contract that motor protocols must implement
 * to receive CAN communication services from the upper layer.
 */
class CANProtocolInterface {
public:
    virtual ~CANProtocolInterface() = default;

    /**
     * @brief Get protocol name
     */
    virtual std::string get_protocol_name() const = 0;

    /**
     * @brief Get supported CAN ID ranges for this protocol
     * @return Pair of [min_id, max_id] for this protocol
     */
    virtual std::pair<uint32_t, uint32_t> get_can_id_range() const = 0;

    /**
     * @brief Process incoming CAN frame
     * @param frame Received CAN frame
     * @return True if frame was successfully processed
     */
    virtual bool process_can_frame(const CANFrame& frame) = 0;

    /**
     * @brief Send CAN frame through the communication layer
     * @param frame Frame to send
     * @return True if frame was successfully queued for sending
     */
    virtual bool send_can_frame(const CANFrame& frame) = 0;

    /**
     * @brief Check if protocol is ready to send commands
     */
    virtual bool is_ready() const = 0;

    /**
     * @brief Enable protocol (prepare for communication)
     */
    virtual bool enable() = 0;

    /**
     * @brief Disable protocol (stop communication)
     */
    virtual bool disable() = 0;
};

/**
 * @brief CAN frame callback function type
 */
using CANFrameCallback = std::function<void(const CANFrame&)>;

/**
 * @brief CAN frame dispatcher interface
 *
 * Routes CAN frames to appropriate protocols based on ID ranges
 */
class CANFrameDispatcherInterface {
public:
    virtual ~CANFrameDispatcherInterface() = default;

    /**
     * @brief Register a protocol for CAN frame routing
     * @param protocol Protocol to register
     * @return True if registration successful
     */
    virtual bool register_protocol(std::shared_ptr<CANProtocolInterface> protocol) = 0;

    /**
     * @brief Unregister a protocol
     * @param protocol_name Name of protocol to unregister
     */
    virtual void unregister_protocol(const std::string& protocol_name) = 0;

    /**
     * @brief Process incoming CAN frame and route to appropriate protocol
     * @param frame Incoming CAN frame
     */
    virtual void dispatch_frame(const CANFrame& frame) = 0;

    /**
     * @brief Send CAN frame through the communication layer
     * @param frame Frame to send
     * @return True if frame was successfully sent
     */
    virtual bool send_frame(const CANFrame& frame) = 0;

    /**
     * @brief Get list of registered protocols
     */
    virtual std::vector<std::string> get_registered_protocols() const = 0;
};

/**
 * @brief CAN communication interface (abstraction over USB/CAN hardware)
 */
class CANCommunicationInterface {
public:
    virtual ~CANCommunicationInterface() = default;

    /**
     * @brief Initialize CAN communication
     * @param device Serial number or device identifier
     * @return True if initialization successful
     */
    virtual bool initialize(const std::string& device = "") = 0;

    /**
     * @brief Shutdown CAN communication
     */
    virtual void shutdown() = 0;

    /**
     * @brief Check if communication is connected
     */
    virtual bool is_connected() const = 0;

    /**
     * @brief Send CAN frame
     * @param frame Frame to send
     * @return True if frame sent successfully
     */
    virtual bool send_frame(const CANFrame& frame) = 0;

    /**
     * @brief Register callback for received frames
     * @param callback Function to call when frame received
     */
    virtual void register_receive_callback(CANFrameCallback callback) = 0;

    /**
     * @brief Start receiving frames (non-blocking)
     * @return True if started successfully
     */
    virtual bool start_receiving() = 0;

    /**
     * @brief Stop receiving frames
     */
    virtual void stop_receiving() = 0;

    /**
     * @brief Communication statistics structure
     */
    struct CommStats {
        size_t frames_sent;
        size_t frames_received;
        size_t send_errors;
        size_t receive_errors;
    };

    /**
     * @brief Get communication statistics
     */
    virtual CommStats get_statistics() const = 0;
};

} // namespace ic_can