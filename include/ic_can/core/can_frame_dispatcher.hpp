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
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace ic_can {

/**
 * @brief Implementation of CAN frame dispatcher
 *
 * Central dispatcher that routes incoming CAN frames to the appropriate
 * protocol handlers based on CAN ID ranges. Provides O(1) frame routing
 * performance for 500Hz control loops.
 */
class CANFrameDispatcher : public CANFrameDispatcherInterface {
public:
    /**
     * @brief Constructor
     * @param communication Communication interface for sending frames
     */
    explicit CANFrameDispatcher(std::shared_ptr<CANCommunicationInterface> communication);

    /**
     * @brief Destructor
     */
    virtual ~CANFrameDispatcher() = default;

    // ========== CANFrameDispatcherInterface Implementation ==========

    /**
     * @brief Register a protocol with the dispatcher
     * @param protocol Protocol to register
     * @return True if registration successful
     */
    bool register_protocol(std::shared_ptr<CANProtocolInterface> protocol) override;

    /**
     * @brief Unregister a protocol from the dispatcher
     * @param protocol_name Name of protocol to unregister
     * @return True if unregistration successful
     */
    bool unregister_protocol(const std::string& protocol_name) override;

    /**
     * @brief Dispatch an incoming CAN frame to the appropriate protocol
     * @param frame CAN frame to dispatch
     * @return True if frame was successfully dispatched
     */
    bool dispatch_frame(const CANFrame& frame) override;

    /**
     * @brief Send a CAN frame through the communication interface
     * @param frame CAN frame to send
     * @return True if frame was sent successfully
     */
    bool send_frame(const CANFrame& frame) override;

    /**
     * @brief Get all registered protocols
     * @return Vector of registered protocol names
     */
    std::vector<std::string> get_registered_protocols() const override;

    /**
     * @brief Get dispatcher statistics
     * @return Statistics about frame routing
     */
    DispatcherStats get_statistics() const override;

    // ========== Additional Methods ==========

    /**
     * @brief Get protocol by CAN ID
     * @param can_id CAN ID to look up
     * @return Protocol handling this CAN ID, or nullptr if none
     */
    std::shared_ptr<CANProtocolInterface> get_protocol_by_id(uint32_t can_id) const;

    /**
     * @brief Check if any protocols are registered
     * @return True if at least one protocol is registered
     */
    bool has_protocols() const;

    /**
     * @brief Clear all registered protocols
     */
    void clear_protocols();

private:
    // ========== Private Members ==========

    std::shared_ptr<CANCommunicationInterface> communication_;  // Communication interface
    std::unordered_map<std::string, std::shared_ptr<CANProtocolInterface>> protocols_;  // Registered protocols
    mutable std::mutex protocols_mutex_;                         // Mutex for thread safety

    // Statistics
    mutable std::mutex stats_mutex_;
    uint64_t frames_dispatched_ = 0;
    uint64_t frames_sent_ = 0;
    uint64_t routing_errors_ = 0;

    // ========== Private Methods ==========

    /**
     * @brief Build efficient routing table from protocols
     */
    void build_routing_table();

    /**
     * @brief Debug print function
     * @param message Debug message
     */
    void debug_print(const std::string& message) const;

    /**
     * @brief Update statistics
     * @param frames_dispatched Number of frames dispatched
     * @param frames_sent Number of frames sent
     * @param routing_errors Number of routing errors
     */
    void update_statistics(uint64_t frames_dispatched = 0,
                           uint64_t frames_sent = 0,
                           uint64_t routing_errors = 0) const;
};

} // namespace ic_can