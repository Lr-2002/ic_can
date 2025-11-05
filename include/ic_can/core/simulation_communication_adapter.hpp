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
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <map>

namespace ic_can {

/**
 * @brief Simulation Communication Adapter
 *
 * A lightweight simulation adapter that provides CAN communication functionality
 * without requiring hardware. This adapter is useful for testing and development.
 */
class SimulationCommunicationAdapter : public CANCommunicationInterface {
public:
    /**
     * @brief Constructor
     * @param debug Enable debug output
     */
    explicit SimulationCommunicationAdapter(bool debug = false);

    /**
     * @brief Destructor
     */
    virtual ~SimulationCommunicationAdapter();

    // ===================================
    // CORE COMMUNICATION INTERFACE
    // ===================================

    /**
     * @brief Initialize the simulation adapter
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * @brief Initialize simulation hardware (internal method)
     * @return true if initialization successful
     */
    bool initialize_simulation_hardware();

    /**
     * @brief Shutdown the adapter
     */
    void shutdown();

    /**
     * @brief Check if adapter is ready
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
     * @param frame CAN frame to send
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
    // SIMULATION-SPECIFIC METHODS
    // ===================================

    /**
     * @brief Add simulated device
     * @param device_id Device identifier
     * @param response_delay_ms Delay in milliseconds before responding
     */
    void add_simulated_device(uint32_t device_id, uint32_t response_delay_ms = 10);

    /**
     * @brief Remove simulated device
     * @param device_id Device identifier
     * @return true if device removed
     */
    bool remove_simulated_device(uint32_t device_id);

    /**
     * @brief Get simulation statistics
     * @return Map of simulation statistics
     */
    std::map<std::string, uint64_t> get_simulation_stats() const;

    /**
     * @brief Reset simulation statistics
     */
    void reset_simulation_stats();

private:
    // Simulation data structures
    struct SimulatedDevice {
        uint32_t device_id;
        uint32_t response_delay_ms;
        std::vector<CANFrame> pending_frames;
        std::chrono::steady_clock::time_point last_activity;
    };

    // Thread management
    void simulation_thread_function();
    void start_simulation_thread();
    void stop_simulation_thread();

    // Member variables
    bool debug_enabled_;
    std::atomic<bool> initialized_;
    std::atomic<bool> running_;
    std::atomic<bool> simulation_thread_running_;

    // Simulation state
    std::map<uint32_t, SimulatedDevice> simulated_devices_;
    std::vector<CANFrame> sent_frames_;
    std::vector<CANFrame> received_frames_;
    mutable std::mutex devices_mutex_;
    mutable std::mutex frames_mutex_;

    // Callback management
    std::function<void(const CANFrame&)> receive_callback_;
    mutable std::mutex callback_mutex_;

    // Thread management
    std::unique_ptr<std::thread> simulation_thread_;

    // Statistics
    std::atomic<uint64_t> frames_sent_;
    std::atomic<uint64_t> frames_received_;
    std::chrono::steady_clock::time_point start_time_;

    // Simulation parameters
    static constexpr uint32_t DEFAULT_RESPONSE_DELAY_MS = 10;
    static constexpr uint32_t MAX_PENDING_FRAMES = 1000;
};

} // namespace ic_can