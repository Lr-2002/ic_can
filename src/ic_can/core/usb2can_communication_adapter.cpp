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

#include "ic_can/core/usb2can_communication_adapter.hpp"
#include <iomanip>
#include <sstream>
#include <chrono>

// Include dm-tools SDK
#include "protocol/usb_class.h"

// Ensure can_value_type is available
using can_value_type = ::can_value_type;

namespace ic_can {

USB2CANCommunicationAdapter::USB2CANCommunicationAdapter(const std::string& device_sn, bool debug)
    : device_sn_(device_sn)
    , debug_enabled_(debug)
    , is_connected_(false)
    , is_communicating_(false)
    , start_time_(std::chrono::steady_clock::now()) {

    debug_print("USB2CANCommunicationAdapter created for device: " + device_sn_);
}

USB2CANCommunicationAdapter::~USB2CANCommunicationAdapter() {
    if (is_communicating_) {
        stop_communication();
    }

    if (usb_device_) {
        debug_print("Cleaning up USB2CAN device connection");
        // usb_device_ will be automatically cleaned up by unique_ptr
    }
}

bool USB2CANCommunicationAdapter::initialize() {
    if (is_connected_) {
        debug_print("Already initialized");
        return true;
    }

    try {
        debug_print("Initializing USB2CAN device...");

        // Create USB device instance
        usb_device_ = std::make_unique<usb_class>();

        // Initialize the device
        if (!usb_device_->usb_init(device_sn_.c_str())) {
            debug_print("Failed to initialize USB2CAN device");
            return false;
        }

        is_connected_ = true;
        debug_print("USB2CAN device initialized successfully");
        return true;

    } catch (const std::exception& e) {
        debug_print("Exception during initialization: " + std::string(e.what()));
        return false;
    }
}

bool USB2CANCommunicationAdapter::send_can_frame(const CANFrame& frame) {
    if (!is_connected_ || !usb_device_) {
        debug_print("Cannot send frame - device not connected");
        return false;
    }

    try {
        // Convert to dm-tools format
        can_value_type usb_frame = convert_to_usb_frame(frame);

        // Send frame through USB2CAN
        bool success = usb_device_->send_data(usb_frame);

        if (success) {
            update_statistics(usb_frame.head.dlc, 0);
            if (debug_enabled_) {
                std::ostringstream oss;
                oss << "Sent frame: ID=0x" << std::hex << frame.id
                    << ", DLC=" << std::dec << frame.data.size();
                debug_print(oss.str());
            }
        } else {
            debug_print("Failed to send frame: ID=0x" + std::to_string(frame.id));
        }

        return success;

    } catch (const std::exception& e) {
        debug_print("Exception during frame sending: " + std::string(e.what()));
        return false;
    }
}

void USB2CANCommunicationAdapter::set_frame_received_callback(std::function<void(const CANFrame&)> callback) {
    frame_callback_ = callback;

    if (usb_device_ && is_connected_) {
        // Set up the callback to receive frames from dm-tools
        usb_device_->set_read_callback([this](can_value_type& frame) {
            this->on_frame_received(frame);
        });

        debug_print("Frame received callback registered");
    }
}

bool USB2CANCommunicationAdapter::start_communication() {
    if (!is_connected_) {
        if (!initialize()) {
            return false;
        }
    }

    if (is_communicating_) {
        debug_print("Communication already started");
        return true;
    }

    try {
        debug_print("Starting CAN communication...");

        // Start receiving data
        bool success = usb_device_->start_read();

        if (success) {
            is_communicating_ = true;
            debug_print("CAN communication started successfully");
        } else {
            debug_print("Failed to start CAN communication");
        }

        return success;

    } catch (const std::exception& e) {
        debug_print("Exception during communication start: " + std::string(e.what()));
        return false;
    }
}

bool USB2CANCommunicationAdapter::stop_communication() {
    if (!is_communicating_) {
        return true;
    }

    try {
        debug_print("Stopping CAN communication...");

        // Stop receiving data
        bool success = false;
        if (usb_device_) {
            success = usb_device_->stop_read();
        }

        is_communicating_ = false;

        if (success) {
            debug_print("CAN communication stopped successfully");
        } else {
            debug_print("Failed to stop CAN communication");
        }

        return success;

    } catch (const std::exception& e) {
        debug_print("Exception during communication stop: " + std::string(e.what()));
        return false;
    }
}

bool USB2CANCommunicationAdapter::is_communication_active() const {
    return is_connected_ && is_communicating_;
}

CommunicationStats USB2CANCommunicationAdapter::get_statistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);

    CommunicationStats stats;
    stats.frames_sent = frames_sent_;
    stats.frames_received = frames_received_;
    stats.bytes_sent = bytes_sent_;
    stats.bytes_received = bytes_received_;

    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    stats.uptime_seconds = duration.count();

    return stats;
}

bool USB2CANCommunicationAdapter::is_connected() const {
    return is_connected_;
}

const std::string& USB2CANCommunicationAdapter::get_device_serial() const {
    return device_sn_;
}

void USB2CANCommunicationAdapter::set_debug_enabled(bool debug) {
    debug_enabled_ = debug;
    debug_print("Debug output " + std::string(debug ? "enabled" : "disabled"));
}

// ========== Private Methods ==========

can_value_type USB2CANCommunicationAdapter::convert_to_usb_frame(const CANFrame& frame) const {
    can_value_type usb_frame;

    // Clear the structure
    memset(&usb_frame, 0, sizeof(usb_frame));

    // Set header
    usb_frame.head.id = frame.id;
    usb_frame.head.dlc = std::min(static_cast<uint8_t>(frame.data.size()), static_cast<uint8_t>(64));
    usb_frame.head.fram_type = 0;  // Data frame
    usb_frame.head.can_type = 0;   // CAN 2.0
    usb_frame.head.id_type = frame.is_extended_id ? 1 : 0;
    usb_frame.head.dir = 1;         // TX
    usb_frame.head.time_stamp = 0;  // Not used for sending

    // Copy data
    for (size_t i = 0; i < usb_frame.head.dlc && i < frame.data.size(); ++i) {
        usb_frame.data[i] = frame.data[i];
    }

    return usb_frame;
}

CANFrame USB2CANCommunicationAdapter::convert_from_usb_frame(const can_value_type& usb_frame) const {
    CANFrame frame;

    // Set basic fields
    frame.id = usb_frame.head.id;
    frame.is_extended_id = (usb_frame.head.id_type == 1);
    frame.is_remote_frame = (usb_frame.head.fram_type == 1);

    // Copy data
    frame.data.clear();
    for (uint8_t i = 0; i < usb_frame.head.dlc && i < 64; ++i) {
        frame.data.push_back(usb_frame.data[i]);
    }

    return frame;
}

void USB2CANCommunicationAdapter::on_frame_received(can_value_type& frame) {
    if (!frame_callback_) {
        return;
    }

    try {
        // Convert to new format
        CANFrame new_frame = convert_from_usb_frame(frame);

        // Update statistics
        update_statistics(0, frame.head.dlc);

        // Call the callback
        frame_callback_(new_frame);

        if (debug_enabled_) {
            std::ostringstream oss;
            oss << "Received frame: ID=0x" << std::hex << frame.head.id
                << ", DLC=" << std::dec << frame.head.dlc;
            debug_print(oss.str());
        }

    } catch (const std::exception& e) {
        debug_print("Exception in frame received callback: " + std::string(e.what()));
    }
}

void USB2CANCommunicationAdapter::debug_print(const std::string& message) const {
    if (debug_enabled_) {
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        std::cout << "[" << timestamp << "] [USB2CAN-ADAPTER] " << message << std::endl;
    }
}

void USB2CANCommunicationAdapter::update_statistics(size_t bytes_sent, size_t bytes_received) {
    std::lock_guard<std::mutex> lock(stats_mutex_);

    if (bytes_sent > 0) {
        frames_sent_++;
        bytes_sent_ += bytes_sent;
    }

    if (bytes_received > 0) {
        frames_received_++;
        bytes_received_ += bytes_received;
    }
}

} // namespace ic_can