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

#include "ic_can/core/simulation_communication_adapter.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <random>
#include <map>

namespace ic_can {

SimulationCommunicationAdapter::SimulationCommunicationAdapter(bool debug)
    : debug_enabled_(debug)
    , initialized_(false)
    , running_(false)
    , simulation_thread_running_(false)
    , frames_sent_(0)
    , frames_received_(0)
    , start_time_(std::chrono::steady_clock::now()) {

    // Initialize random number generator
    if (debug_enabled_) {
        std::cout << "🔧 SimulationCommunicationAdapter created" << std::endl;
    }
}

SimulationCommunicationAdapter::~SimulationCommunicationAdapter() {
    shutdown();
    if (debug_enabled_) {
        std::cout << "🔧 SimulationCommunicationAdapter destroyed" << std::endl;
    }
}

bool SimulationCommunicationAdapter::initialize_simulation_hardware() {
    if (initialized_) {
        return true;
    }

    if (debug_enabled_) {
        std::cout << "🚀 Initializing simulation adapter..." << std::endl;
    }

    // Reset state
    frames_sent_ = 0;
    frames_received_ = 0;
    start_time_ = std::chrono::steady_clock::now();

    // Clear frames
    {
        std::lock_guard<std::mutex> lock(frames_mutex_);
        sent_frames_.clear();
        received_frames_.clear();
    }

    {
        std::lock_guard<std::mutex> lock(devices_mutex_);
        simulated_devices_.clear();
    }

    // Add a default simulated device for testing
    add_simulated_device(0x123, DEFAULT_RESPONSE_DELAY_MS);

    // Start simulation thread
    start_simulation_thread();

    initialized_ = true;
    running_ = true;

    if (debug_enabled_) {
        std::cout << "✅ Simulation adapter initialized successfully" << std::endl;
    }

    return true;
}

void SimulationCommunicationAdapter::shutdown() {
    if (!initialized_) {
        return;
    }

    if (debug_enabled_) {
        std::cout << "🛑 Shutting down simulation adapter..." << std::endl;
    }

    running_ = false;
    stop_simulation_thread();

    initialized_ = false;

    if (debug_enabled_) {
        std::cout << "✅ Simulation adapter shutdown complete" << std::endl;
    }
}

bool SimulationCommunicationAdapter::is_ready() const {
    return initialized_ && running_;
}

bool SimulationCommunicationAdapter::send_frame(const CANFrame& frame) {
    if (!is_ready()) {
        return false;
    }

    // Store the sent frame
    {
        std::lock_guard<std::mutex> lock(frames_mutex_);
        sent_frames_.push_back(frame);
        if (sent_frames_.size() > MAX_PENDING_FRAMES) {
            sent_frames_.erase(sent_frames_.begin());
        }
        frames_sent_++;
    }

    // Add frame to simulated devices' pending frames
    {
        std::lock_guard<std::mutex> lock(devices_mutex_);
        for (auto& [device_id, device] : simulated_devices_) {
            device.pending_frames.push_back(frame);
            device.last_activity = std::chrono::steady_clock::now();
        }
    }

    if (debug_enabled_) {
        std::cout << "📤 Sent frame: ID=0x" << std::hex << frame.id
                  << ", Data size=" << std::dec << frame.data.size() << std::endl;
    }

    return true;
}

size_t SimulationCommunicationAdapter::send_frames(const std::vector<CANFrame>& frames) {
    if (!is_ready()) {
        return 0;
    }

    size_t sent_count = 0;
    for (const auto& frame : frames) {
        if (send_frame(frame)) {
            sent_count++;
        }
    }

    return sent_count;
}

bool SimulationCommunicationAdapter::receive_frame(CANFrame& frame, int timeout_ms) {
    if (!is_ready()) {
        return false;
    }

    auto start_time = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(timeout_ms);

    while (std::chrono::steady_clock::now() - start_time < timeout) {
        {
            std::lock_guard<std::mutex> lock(frames_mutex_);
            if (!received_frames_.empty()) {
                frame = received_frames_.front();
                received_frames_.erase(received_frames_.begin());
                frames_received_++;
                return true;
            }
        }

        // Sleep for a short time to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return false;
}

size_t SimulationCommunicationAdapter::receive_frames(std::vector<CANFrame>& frames, size_t max_frames, int timeout_ms) {
    if (!is_ready()) {
        return 0;
    }

    size_t received_count = 0;
    auto start_time = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(timeout_ms);

    while (received_count < max_frames && std::chrono::steady_clock::now() - start_time < timeout) {
        CANFrame frame;
        if (receive_frame(frame, 1)) {
            frames.push_back(frame);
            received_count++;
        }
    }

    return received_count;
}

bool SimulationCommunicationAdapter::set_receive_callback(std::function<void(const CANFrame&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    receive_callback_ = callback;
    return true;
}

void SimulationCommunicationAdapter::add_simulated_device(uint32_t device_id, uint32_t response_delay_ms) {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    simulated_devices_[device_id] = {
        device_id,
        response_delay_ms,
        {},
        std::chrono::steady_clock::now()
    };

    if (debug_enabled_) {
        std::cout << "➕ Added simulated device: ID=0x" << std::hex << device_id
                  << ", Response delay=" << std::dec << response_delay_ms << "ms" << std::endl;
    }
}

bool SimulationCommunicationAdapter::remove_simulated_device(uint32_t device_id) {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    auto it = simulated_devices_.find(device_id);
    if (it != simulated_devices_.end()) {
        simulated_devices_.erase(it);
        if (debug_enabled_) {
            std::cout << "➖ Removed simulated device: ID=0x" << std::hex << device_id << std::endl;
        }
        return true;
    }
    return false;
}

std::map<std::string, uint64_t> SimulationCommunicationAdapter::get_simulation_stats() const {
    std::map<std::string, uint64_t> stats;
    stats["frames_sent"] = frames_sent_.load();
    stats["frames_received"] = frames_received_.load();
    stats["simulation_time_ms"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time_).count();
    return stats;
}

void SimulationCommunicationAdapter::reset_simulation_stats() {
    frames_sent_ = 0;
    frames_received_ = 0;
    start_time_ = std::chrono::steady_clock::now();
}

void SimulationCommunicationAdapter::simulation_thread_function() {
    if (debug_enabled_) {
        std::cout << "🧵 Simulation thread started" << std::endl;
    }

    while (simulation_thread_running_) {
        // Process simulated devices
        {
            std::lock_guard<std::mutex> lock(devices_mutex_);
            auto now = std::chrono::steady_clock::now();

            for (auto& [device_id, device] : simulated_devices_) {
                // Check if it's time to respond
                if (!device.pending_frames.empty() &&
                    (now - device.last_activity).count() >= device.response_delay_ms) {

                    // Generate a response frame
                    CANFrame response_frame;
                    response_frame.id = device_id | 0x80000000; // Response frame
                    response_frame.data.resize(8);
                    response_frame.data.resize(8, 0);

                    // Simple echo simulation
                    if (!device.pending_frames.empty()) {
                        const auto& original_frame = device.pending_frames.back();
                        response_frame.id = original_frame.id | 0x80000000; // Response ID
                        response_frame.data.resize(std::min(original_frame.data.size(), static_cast<size_t>(8)));
                        response_frame.data = original_frame.data;
                        if (response_frame.data.size() > 8) {
                            response_frame.data.resize(8);
                        }
                        device.pending_frames.pop_back();
                    }

                    // Add to received frames
                    {
                        std::lock_guard<std::mutex> frames_lock(frames_mutex_);
                        received_frames_.push_back(response_frame);
                        if (received_frames_.size() > MAX_PENDING_FRAMES) {
                            received_frames_.erase(received_frames_.begin());
                        }
                        frames_received_++;
                    }

                    // Call receive callback if set
                    {
                        std::lock_guard<std::mutex> callback_lock(callback_mutex_);
                        if (receive_callback_) {
                            receive_callback_(response_frame);
                        }
                    }

                    device.last_activity = now;
                }
            }
        }

        // Sleep for a short time
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    if (debug_enabled_) {
        std::cout << "🧵 Simulation thread stopped" << std::endl;
    }
}

void SimulationCommunicationAdapter::start_simulation_thread() {
    if (simulation_thread_running_) {
        return;
    }

    simulation_thread_running_ = true;
    simulation_thread_ = std::make_unique<std::thread>(
        &SimulationCommunicationAdapter::simulation_thread_function, this);
}

void SimulationCommunicationAdapter::stop_simulation_thread() {
    if (!simulation_thread_running_) {
        return;
    }

    simulation_thread_running_ = false;
    if (simulation_thread_ && simulation_thread_->joinable()) {
        simulation_thread_->join();
    }
    simulation_thread_.reset();
}

// ===================================
// CANCommunicationInterface IMPLEMENTATION
// ===================================

bool SimulationCommunicationAdapter::initialize() {
    return initialize_simulation_hardware();
}

bool SimulationCommunicationAdapter::initialize(const std::string& device) {
    // The device parameter is ignored for simulation
    return initialize_simulation_hardware();
}

bool SimulationCommunicationAdapter::is_connected() const {
    return is_ready();
}

void SimulationCommunicationAdapter::register_receive_callback(CANFrameCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    receive_callback_ = callback;
}

bool SimulationCommunicationAdapter::start_receiving() {
    // The simulation thread is already started in initialize()
    return is_ready();
}

void SimulationCommunicationAdapter::stop_receiving() {
    // Stop the simulation thread
    stop_simulation_thread();
}

SimulationCommunicationAdapter::CommStats SimulationCommunicationAdapter::get_statistics() const {
    CommStats stats;
    stats.frames_sent = frames_sent_.load();
    stats.frames_received = frames_received_.load();
    stats.send_errors = 0; // No errors in simulation
    stats.receive_errors = 0; // No errors in simulation
    return stats;
}

} // namespace ic_can