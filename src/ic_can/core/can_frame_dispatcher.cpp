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

#include "ic_can/core/can_protocol_interface.hpp"
#include <algorithm>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <memory>

namespace ic_can {

/**
 * @brief Implementation of CAN frame dispatcher
 */
class CANFrameDispatcher : public CANFrameDispatcherInterface {
public:
    CANFrameDispatcher(std::shared_ptr<CANCommunicationInterface> communication)
        : communication_(communication) {}

    bool register_protocol(std::shared_ptr<CANProtocolInterface> protocol) override {
        if (!protocol) {
            std::cerr << "❌ Cannot register null protocol" << std::endl;
            return false;
        }

        std::lock_guard<std::mutex> lock(protocols_mutex_);

        std::string protocol_name = protocol->get_protocol_name();
        if (protocols_.find(protocol_name) != protocols_.end()) {
            std::cerr << "⚠️  Protocol '" << protocol_name << "' already registered" << std::endl;
            return false;
        }

        auto id_range = protocol->get_can_id_range();
        protocols_[protocol_name] = protocol;

        std::cout << "✅ Registered protocol '" << protocol_name
                  << "' for CAN ID range [0x" << std::hex << id_range.first
                  << ", 0x" << id_range.second << std::dec << "]" << std::endl;

        // Sort protocols by priority based on ID ranges (smaller ranges = higher priority)
        // This ensures proper routing when ID ranges overlap
        update_protocol_priorities();

        return true;
    }

    void unregister_protocol(const std::string& protocol_name) override {
        std::lock_guard<std::mutex> lock(protocols_mutex_);

        auto it = protocols_.find(protocol_name);
        if (it != protocols_.end()) {
            protocols_.erase(it);
            std::cout << "✅ Unregistered protocol '" << protocol_name << "'" << std::endl;
            update_protocol_priorities();
        } else {
            std::cerr << "⚠️  Protocol '" << protocol_name << "' not found for unregistration" << std::endl;
        }
    }

    void dispatch_frame(const CANFrame& frame) override {
        std::lock_guard<std::mutex> lock(protocols_mutex_);

        bool dispatched = false;

        // Dispatch to protocols based on ID range priority
        for (const auto& protocol_name : protocol_priority_) {
            auto it = protocols_.find(protocol_name);
            if (it != protocols_.end()) {
                auto protocol = it->second;
                auto range = protocol->get_can_id_range();

                // Check if frame ID falls within protocol's range
                if (frame.id >= range.first && frame.id <= range.second) {
                    if (protocol->process_can_frame(frame)) {
                        dispatched = true;
                        // Don't break here - allow multiple protocols to process the same frame
                        // if they have overlapping ranges (useful for debugging/monitoring)
                    }
                }
            }
        }

        if (!dispatched) {
            std::cout << "⚠️  No protocol registered for CAN ID 0x"
                      << std::hex << frame.id << std::dec << std::endl;
        }
    }

    bool send_frame(const CANFrame& frame) override {
        if (!communication_) {
            std::cerr << "❌ No communication interface available for sending frame" << std::endl;
            return false;
        }

        return communication_->send_frame(frame);
    }

    std::vector<std::string> get_registered_protocols() const override {
        std::lock_guard<std::mutex> lock(protocols_mutex_);

        std::vector<std::string> protocols;
        for (const auto& pair : protocols_) {
            protocols.push_back(pair.first);
        }
        return protocols;
    }

    /**
     * @brief Set debug mode for frame routing
     */
    void set_debug_mode(bool enabled) {
        debug_enabled_ = enabled;
    }

private:
    std::shared_ptr<CANCommunicationInterface> communication_;
    std::unordered_map<std::string, std::shared_ptr<CANProtocolInterface>> protocols_;
    std::vector<std::string> protocol_priority_;  // Sorted by priority
    mutable std::mutex protocols_mutex_;
    bool debug_enabled_ = false;

    void update_protocol_priorities() {
        protocol_priority_.clear();

        // Sort protocols by ID range size (smaller ranges get higher priority)
        std::vector<std::pair<std::string, uint32_t>> protocol_sizes;
        for (const auto& pair : protocols_) {
            auto range = pair.second->get_can_id_range();
            uint32_t range_size = range.second - range.first + 1;
            protocol_sizes.emplace_back(pair.first, range_size);
        }

        std::sort(protocol_sizes.begin(), protocol_sizes.end(),
                 [](const auto& a, const auto& b) { return a.second < b.second; });

        for (const auto& pair : protocol_sizes) {
            protocol_priority_.push_back(pair.first);
        }

        if (debug_enabled_) {
            std::cout << "📋 Protocol priority order updated:" << std::endl;
            for (size_t i = 0; i < protocol_priority_.size(); ++i) {
                std::cout << "  " << (i + 1) << ". " << protocol_priority_[i] << std::endl;
            }
        }
    }
};

/**
 * @brief Factory function to create CAN frame dispatcher
 */
std::unique_ptr<CANFrameDispatcherInterface> create_can_frame_dispatcher(
    std::shared_ptr<CANCommunicationInterface> communication) {
    return std::make_unique<CANFrameDispatcher>(communication);
}

} // namespace ic_can