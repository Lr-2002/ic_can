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
#include "ic_can/motors/base_motor.hpp"
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>

namespace ic_can {

/**
 * @brief Base class for motor protocols implementing dependency inversion
 *
 * This class provides the foundation for motor components (like WristComponent)
 * to receive CAN communication services from the upper layer instead of
 * creating their own communication infrastructure.
 */
class MotorProtocolBase : public CANProtocolInterface {
public:
    /**
     * @brief Constructor
     * @param protocol_name Name of the protocol
     * @param min_can_id Minimum CAN ID this protocol handles
     * @param max_can_id Maximum CAN ID this protocol handles
     */
    MotorProtocolBase(const std::string& protocol_name, uint32_t min_can_id, uint32_t max_can_id)
        : protocol_name_(protocol_name)
        , can_id_range_(std::make_pair(min_can_id, max_can_id))
        , enabled_(false)
        , debug_enabled_(false) {}

    virtual ~MotorProtocolBase() = default;

    // ========== CANProtocolInterface Implementation ==========

    std::string get_protocol_name() const override {
        return protocol_name_;
    }

    std::pair<uint32_t, uint32_t> get_can_id_range() const override {
        return can_id_range_;
    }

    bool send_can_frame(const CANFrame& frame) override {
        if (!enabled_) {
            if (debug_enabled_) {
                std::cout << "⚠️  " << protocol_name_ << ": Cannot send frame - protocol not enabled" << std::endl;
            }
            return false;
        }

        // Frame will be sent by the upper layer (CANFrameDispatcher)
        // For now, just log and return true
        if (debug_enabled_) {
            std::cout << "📤 " << protocol_name_ << ": Sending CAN frame ID 0x"
                      << std::hex << frame.id << std::dec << " with "
                      << frame.data.size() << " bytes" << std::endl;
        }

        return true;
    }

    bool is_ready() const override {
        return enabled_ && !motors_.empty();
    }

    bool enable() override {
        enabled_ = true;
        std::cout << "✅ " << protocol_name_ << " protocol enabled" << std::endl;
        return true;
    }

    bool disable() override {
        enabled_ = false;
        std::cout << "✅ " << protocol_name_ << " protocol disabled" << std::endl;
        return true;
    }

    // ========== Motor Management Interface ==========

    /**
     * @brief Add a motor to this protocol
     * @param motor Shared pointer to motor instance
     * @return True if motor added successfully
     */
    virtual bool add_motor(std::shared_ptr<BaseMotor> motor) {
        if (!motor) {
            std::cerr << "❌ " << protocol_name_ << ": Cannot add null motor" << std::endl;
            return false;
        }

        int motor_id = motor->get_motor_id();
        std::lock_guard<std::mutex> lock(motors_mutex_);

        if (motors_.find(motor_id) != motors_.end()) {
            std::cerr << "⚠️  " << protocol_name_ << ": Motor " << motor_id << " already exists" << std::endl;
            return false;
        }

        motors_[motor_id] = motor;
        std::cout << "✅ " << protocol_name_ << ": Added motor " << motor_id << std::endl;
        return true;
    }

    /**
     * @brief Remove a motor from this protocol
     * @param motor_id ID of motor to remove
     * @return True if motor removed successfully
     */
    virtual bool remove_motor(int motor_id) {
        std::lock_guard<std::mutex> lock(motors_mutex_);

        auto it = motors_.find(motor_id);
        if (it == motors_.end()) {
            std::cerr << "❌ " << protocol_name_ << ": Motor " << motor_id << " not found" << std::endl;
            return false;
        }

        motors_.erase(it);
        std::cout << "✅ " << protocol_name_ << ": Removed motor " << motor_id << std::endl;
        return true;
    }

    /**
     * @brief Get a motor by ID
     * @param motor_id Motor ID
     * @return Shared pointer to motor or nullptr if not found
     */
    virtual std::shared_ptr<BaseMotor> get_motor(int motor_id) const {
        std::lock_guard<std::mutex> lock(motors_mutex_);

        auto it = motors_.find(motor_id);
        if (it != motors_.end()) {
            return it->second;
        }
        return nullptr;
    }

    /**
     * @brief Get all motors in this protocol
     * @return Map of motor_id -> motor shared_ptr
     */
    virtual std::unordered_map<int, std::shared_ptr<BaseMotor>> get_all_motors() const {
        std::lock_guard<std::mutex> lock(motors_mutex_);
        return motors_;
    }

    /**
     * @brief Get number of motors in this protocol
     */
    virtual size_t get_motor_count() const {
        std::lock_guard<std::mutex> lock(motors_mutex_);
        return motors_.size();
    }

    /**
     * @brief Get list of motor IDs
     */
    virtual std::vector<int> get_motor_ids() const {
        std::lock_guard<std::mutex> lock(motors_mutex_);

        std::vector<int> ids;
        ids.reserve(motors_.size());
        for (const auto& pair : motors_) {
            ids.push_back(pair.first);
        }
        return ids;
    }

    // ========== Batch Operations ==========

    /**
     * @brief Enable all motors in this protocol
     */
    virtual bool enable_all_motors() {
        std::lock_guard<std::mutex> lock(motors_mutex_);
        bool all_success = true;

        for (auto& [motor_id, motor] : motors_) {
            if (!motor->enable()) {
                std::cerr << "❌ " << protocol_name_ << ": Failed to enable motor " << motor_id << std::endl;
                all_success = false;
            }
        }

        if (all_success) {
            std::cout << "✅ " << protocol_name_ << ": All motors enabled" << std::endl;
        }
        return all_success;
    }

    /**
     * @brief Disable all motors in this protocol
     */
    virtual bool disable_all_motors() {
        std::lock_guard<std::mutex> lock(motors_mutex_);
        bool all_success = true;

        for (auto& [motor_id, motor] : motors_) {
            if (!motor->disable()) {
                std::cerr << "❌ " << protocol_name_ << ": Failed to disable motor " << motor_id << std::endl;
                all_success = false;
            }
        }

        if (all_success) {
            std::cout << "✅ " << protocol_name_ << ": All motors disabled" << std::endl;
        }
        return all_success;
    }

    /**
     * @brief Set zero position for all motors
     */
    virtual bool set_zero_all_motors() {
        std::lock_guard<std::mutex> lock(motors_mutex_);
        bool all_success = true;

        for (auto& [motor_id, motor] : motors_) {
            if (!motor->set_zero()) {
                std::cerr << "❌ " << protocol_name_ << ": Failed to set zero for motor " << motor_id << std::endl;
                all_success = false;
            }
        }

        if (all_success) {
            std::cout << "✅ " << protocol_name_ << ": All motors set to zero" << std::endl;
        }
        return all_success;
    }

    /**
     * @brief Update states for all motors
     */
    virtual void update_all_motor_states() {
        std::lock_guard<std::mutex> lock(motors_mutex_);

        for (auto& [motor_id, motor] : motors_) {
            motor->update_state();
        }
    }

    /**
     * @brief Send commands for all motors
     */
    virtual bool send_all_motor_commands() {
        std::lock_guard<std::mutex> lock(motors_mutex_);
        bool all_success = true;

        for (auto& [motor_id, motor] : motors_) {
            auto command_data = motor->get_command_data();
            if (!command_data.empty()) {
                CANFrame frame(motor->get_can_send_id(), command_data);
                if (!send_can_frame(frame)) {
                    std::cerr << "❌ " << protocol_name_ << ": Failed to send command to motor " << motor_id << std::endl;
                    all_success = false;
                }
            }
        }

        return all_success;
    }

    // ========== Configuration ==========

    /**
     * @brief Set debug mode for this protocol
     */
    virtual void set_debug_mode(bool enabled) {
        debug_enabled_ = enabled;
    }

    /**
     * @brief Check if debug mode is enabled
     */
    virtual bool is_debug_enabled() const {
        return debug_enabled_;
    }

protected:
    std::string protocol_name_;
    std::pair<uint32_t, uint32_t> can_id_range_;
    bool enabled_;
    bool debug_enabled_;

    mutable std::mutex motors_mutex_;
    std::unordered_map<int, std::shared_ptr<BaseMotor>> motors_;

    /**
     * @brief Helper method to check if motor ID is valid for this protocol
     */
    virtual bool is_valid_motor_id(int motor_id) const {
        // Base implementation accepts any motor ID
        // Derived classes can override for specific validation
        return true;
    }
};

} // namespace ic_can