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

#include <ic_can/safety/safety_module.hpp>
#include <iostream>
#include <algorithm>
#include <chrono>

namespace ic_can {

SafetyModule::SafetyModule(int motor_count)
    : motor_count_(motor_count)
    , emergency_stop_triggered_(false)
    , last_watchdog_feed_(std::chrono::steady_clock::now())
    , watchdog_mutex_()
    , last_communication_(motor_count, std::chrono::steady_clock::now())
    , violations_()
    , violations_mutex_() {

    // Initialize safety limits with default values
    limits_.position_min.resize(motor_count, -3.14159);
    limits_.position_max.resize(motor_count, 3.14159);
    limits_.velocity_max.resize(motor_count, 10.0);
    limits_.torque_max.resize(motor_count, 5.0);
    limits_.temperature_max.resize(motor_count, 80.0);
}

SafetyModule::~SafetyModule() {
    stop_monitoring();
}

// ========== 配置接口实现 ==========

void SafetyModule::set_safety_limits(const SafetyLimits& limits) {
    std::lock_guard<std::mutex> lock(violations_mutex_);
    limits_ = limits;

    // Ensure vectors have correct size
    if (limits.position_min.size() != motor_count_) {
        limits_.position_min.resize(motor_count_, -3.14159);
    }
    if (limits.position_max.size() != motor_count_) {
        limits_.position_max.resize(motor_count_, 3.14159);
    }
    if (limits.velocity_max.size() != motor_count_) {
        limits_.velocity_max.resize(motor_count_, 10.0);
    }
    if (limits.torque_max.size() != motor_count_) {
        limits_.torque_max.resize(motor_count_, 5.0);
    }
    if (limits.temperature_max.size() != motor_count_) {
        limits_.temperature_max.resize(motor_count_, 80.0);
    }
}

SafetyLimits SafetyModule::get_safety_limits() const {
    return limits_;
}

void SafetyModule::set_position_limits(int motor_id, double min_pos, double max_pos) {
    if (!is_valid_motor_id(motor_id)) {
        return;
    }

    std::lock_guard<std::mutex> lock(violations_mutex_);
    limits_.position_min[motor_id] = min_pos;
    limits_.position_max[motor_id] = max_pos;
}

void SafetyModule::set_velocity_limit(int motor_id, double max_vel) {
    if (!is_valid_motor_id(motor_id)) {
        return;
    }

    std::lock_guard<std::mutex> lock(violations_mutex_);
    limits_.velocity_max[motor_id] = max_vel;
}

void SafetyModule::set_torque_limit(int motor_id, double max_torque) {
    if (!is_valid_motor_id(motor_id)) {
        return;
    }

    std::lock_guard<std::mutex> lock(violations_mutex_);
    limits_.torque_max[motor_id] = max_torque;
}

void SafetyModule::set_temperature_limit(int motor_id, double max_temp) {
    if (!is_valid_motor_id(motor_id)) {
        return;
    }

    std::lock_guard<std::mutex> lock(violations_mutex_);
    limits_.temperature_max[motor_id] = max_temp;
}

// ========== 安全检查接口实现 ==========

bool SafetyModule::check_position_limits(int motor_id, double position) {
    if (!is_valid_motor_id(motor_id)) {
        return false;
    }

    bool in_limits = (position >= limits_.position_min[motor_id] &&
                      position <= limits_.position_max[motor_id]);

    if (!in_limits) {
        add_violation(SafetyViolation(SafetyViolationType::POSITION_LIMIT, motor_id,
                      "Position out of bounds: " + std::to_string(position),
                      position, limits_.position_max[motor_id]));
    }

    return in_limits;
}

bool SafetyModule::check_velocity_limits(int motor_id, double velocity) {
    if (!is_valid_motor_id(motor_id)) {
        return false;
    }

    double abs_velocity = std::abs(velocity);
    bool in_limits = (abs_velocity <= limits_.velocity_max[motor_id]);

    if (!in_limits) {
        add_violation(SafetyViolation(SafetyViolationType::VELOCITY_LIMIT, motor_id,
                      "Velocity out of bounds: " + std::to_string(velocity),
                      abs_velocity, limits_.velocity_max[motor_id]));
    }

    return in_limits;
}

bool SafetyModule::check_torque_limits(int motor_id, double torque) {
    if (!is_valid_motor_id(motor_id)) {
        return false;
    }

    double abs_torque = std::abs(torque);
    bool in_limits = (abs_torque <= limits_.torque_max[motor_id]);

    if (!in_limits) {
        add_violation(SafetyViolation(SafetyViolationType::TORQUE_LIMIT, motor_id,
                      "Torque out of bounds: " + std::to_string(torque),
                      abs_torque, limits_.torque_max[motor_id]));
    }

    return in_limits;
}

bool SafetyModule::check_temperature_limits(int motor_id, double temperature) {
    if (!is_valid_motor_id(motor_id)) {
        return false;
    }

    bool in_limits = (temperature <= limits_.temperature_max[motor_id]);

    if (!in_limits) {
        add_violation(SafetyViolation(SafetyViolationType::TEMPERATURE_LIMIT, motor_id,
                      "Temperature too high: " + std::to_string(temperature) + "°C",
                      temperature, limits_.temperature_max[motor_id]));
    }

    return in_limits;
}

bool SafetyModule::check_all_limits(int motor_id, double position, double velocity,
                                   double torque, double temperature) {
    return check_position_limits(motor_id, position) &&
           check_velocity_limits(motor_id, velocity) &&
           check_torque_limits(motor_id, torque) &&
           check_temperature_limits(motor_id, temperature);
}

// ========== 紧急停止接口实现 ==========

void SafetyModule::trigger_emergency_stop(const std::string& reason, int motor_id) {
    std::lock_guard<std::mutex> lock(violations_mutex_);

    emergency_stop_triggered_ = true;
    emergency_stop_reason_ = reason;
    emergency_stop_time_ = std::chrono::steady_clock::now();

    SafetyViolation violation(SafetyViolationType::EMERGENCY_STOP, motor_id, reason, 0.0, 0.0);
    violations_.push_back(violation);
    emergency_stops_++;

    // Trigger callback if set
    trigger_emergency_stop_callback(reason);

    std::cout << "EMERGENCY STOP TRIGGERED: " << reason
              << " (Motor ID: " << motor_id << ")" << std::endl;
}

bool SafetyModule::reset_emergency_stop() {
    std::lock_guard<std::mutex> lock(violations_mutex_);

    emergency_stop_triggered_ = false;
    emergency_stop_reason_.clear();

    // Clear emergency stop violations
    auto it = std::remove_if(violations_.begin(), violations_.end(),
        [](const SafetyViolation& v) {
            return v.type == SafetyViolationType::EMERGENCY_STOP;
        });
    violations_.erase(it, violations_.end());

    std::cout << "Emergency stop reset" << std::endl;
    return true;
}

// ========== 看门狗接口实现 ==========

void SafetyModule::feed_watchdog() {
    std::lock_guard<std::mutex> lock(watchdog_mutex_);
    last_watchdog_feed_ = std::chrono::steady_clock::now();
}

bool SafetyModule::is_watchdog_timeout() const {
    // Check if watchdog has timed out
    std::lock_guard<std::mutex> lock(watchdog_mutex_);
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_watchdog_feed_).count();

    return elapsed > watchdog_timeout_ms_;
}

void SafetyModule::set_watchdog_timeout(double timeout_ms) {
    watchdog_timeout_ms_ = timeout_ms;
    limits_.watchdog_timeout_ms = timeout_ms;
}

// ========== 通信监控接口实现 ==========

void SafetyModule::report_communication_success(int motor_id) {
    if (is_valid_motor_id(motor_id)) {
        last_communication_[motor_id] = std::chrono::steady_clock::now();
    }
}

bool SafetyModule::is_communication_timeout(int motor_id) {
    if (!is_valid_motor_id(motor_id)) {
        return true;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_communication_[motor_id]).count();

    return elapsed > communication_timeout_ms_;
}

void SafetyModule::set_communication_timeout(double timeout_ms) {
    communication_timeout_ms_ = timeout_ms;
    limits_.communication_timeout_ms = timeout_ms;
}

// ========== 违规管理接口实现 ==========

void SafetyModule::add_violation(const SafetyViolation& violation) {
    std::lock_guard<std::mutex> lock(violations_mutex_);

    // Check if this violation already exists
    for (auto& existing_violation : violations_) {
        if (existing_violation.type == violation.type &&
            existing_violation.motor_id == violation.motor_id &&
            existing_violation.active) {
            // Update existing violation
            existing_violation.timestamp = violation.timestamp;
            existing_violation.description = violation.description;
            existing_violation.value = violation.value;
            existing_violation.limit = violation.limit;
            return;
        }
    }

    // Add new violation
    violations_.push_back(violation);
    total_violations_++;

    // Trigger callback if set
    trigger_violation_callback(violation);

    std::cout << "SAFETY VIOLATION: " << violation.description
              << " (Motor ID: " << violation.motor_id << ")" << std::endl;
}

void SafetyModule::clear_violations(SafetyViolationType type, int motor_id) {
    std::lock_guard<std::mutex> lock(violations_mutex_);

    for (auto& violation : violations_) {
        if (violation.type == type &&
            (motor_id == -1 || violation.motor_id == motor_id)) {
            violation.active = false;
        }
    }
}

void SafetyModule::clear_all_violations() {
    std::lock_guard<std::mutex> lock(violations_mutex_);

    for (auto& violation : violations_) {
        violation.active = false;
    }
}

std::vector<SafetyViolation> SafetyModule::get_active_violations() const {
    std::lock_guard<std::mutex> lock(violations_mutex_);

    std::vector<SafetyViolation> active;
    for (const auto& violation : violations_) {
        if (violation.active) {
            active.push_back(violation);
        }
    }

    return active;
}

std::vector<SafetyViolation> SafetyModule::get_motor_violations(int motor_id) const {
    std::lock_guard<std::mutex> lock(violations_mutex_);

    std::vector<SafetyViolation> motor_violations;
    for (const auto& violation : violations_) {
        if (violation.motor_id == motor_id) {
            motor_violations.push_back(violation);
        }
    }

    return motor_violations;
}

bool SafetyModule::has_active_violations() const {
    std::lock_guard<std::mutex> lock(violations_mutex_);

    return std::any_of(violations_.begin(), violations_.end(),
        [](const SafetyViolation& v) { return v.active; });
}

// ========== 回调接口实现 ==========

void SafetyModule::set_emergency_stop_callback(std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    emergency_stop_callback_ = callback;
}

void SafetyModule::set_violation_callback(std::function<void(const SafetyViolation&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    violation_callback_ = callback;
}

// ========== 监控接口实现 ==========

bool SafetyModule::start_monitoring() {
    if (monitoring_active_) {
        return true;
    }

    try {
        stop_monitoring_ = false;
        monitoring_thread_ = std::thread(&SafetyModule::monitoring_thread_main, this);
        monitoring_active_ = true;

        std::cout << "Safety monitoring started" << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to start safety monitoring: " << e.what() << std::endl;
        return false;
    }
}

bool SafetyModule::stop_monitoring() {
    if (!monitoring_active_) {
        return true;
    }

    stop_monitoring_ = true;

    if (monitoring_thread_.joinable()) {
        monitoring_thread_.join();
    }

    monitoring_active_ = false;
    std::cout << "Safety monitoring stopped" << std::endl;
    return true;
}

// ========== 诊断接口实现 ==========

std::map<std::string, std::string> SafetyModule::get_safety_status_summary() {
    std::map<std::string, std::string> status;

    status["emergency_stop"] = emergency_stop_triggered_ ? "ACTIVE" : "INACTIVE";
    status["watchdog_timeout"] = is_watchdog_timeout() ? "ACTIVE" : "INACTIVE";
    status["monitoring_active"] = monitoring_active_ ? "ACTIVE" : "INACTIVE";

    auto active_violations = get_active_violations();
    status["active_violations"] = std::to_string(active_violations.size());

    status["total_violations"] = std::to_string(total_violations_.load());
    status["emergency_stops"] = std::to_string(emergency_stops_.load());
    status["watchdog_timeouts"] = std::to_string(watchdog_timeouts_.load());
    status["communication_timeouts"] = std::to_string(communication_timeouts_.load());

    return status;
}

void SafetyModule::print_safety_status_report() {
    auto status = get_safety_status_summary();

    std::cout << "\n=== SAFETY STATUS REPORT ===" << std::endl;
    std::cout << "Emergency Stop: " << status["emergency_stop"] << std::endl;
    std::cout << "Watchdog Timeout: " << status["watchdog_timeout"] << std::endl;
    std::cout << "Monitoring Active: " << status["monitoring_active"] << std::endl;
    std::cout << "Active Violations: " << status["active_violations"] << std::endl;
    std::cout << "Total Violations: " << status["total_violations"] << std::endl;
    std::cout << "Emergency Stops: " << status["emergency_stops"] << std::endl;
    std::cout << "Watchdog Timeouts: " << status["watchdog_timeouts"] << std::endl;
    std::cout << "Communication Timeouts: " << status["communication_timeouts"] << std::endl;

    auto active_violations = get_active_violations();
    for (const auto& violation : active_violations) {
        std::cout << "  - Motor " << violation.motor_id << ": "
                  << violation_type_to_string(violation.type)
                  << " - " << violation.description << std::endl;
    }

    std::cout << "==========================\n" << std::endl;
}

std::map<std::string, uint64_t> SafetyModule::get_safety_statistics() {
    std::map<std::string, uint64_t> stats;

    stats["total_violations"] = total_violations_.load();
    stats["emergency_stops"] = emergency_stops_.load();
    stats["watchdog_timeouts"] = watchdog_timeouts_.load();
    stats["communication_timeouts"] = communication_timeouts_.load();

    std::lock_guard<std::mutex> lock(violations_mutex_);
    stats["current_active_violations"] = std::count_if(violations_.begin(), violations_.end(),
        [](const SafetyViolation& v) { return v.active; });

    return stats;
}

// ========== 私有方法实现 ==========

void SafetyModule::monitoring_thread_main() {
    while (!stop_monitoring_) {
        try {
            perform_safety_checks();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } catch (const std::exception& e) {
            std::cerr << "Safety monitoring thread error: " << e.what() << std::endl;
        }
    }
}

void SafetyModule::perform_safety_checks() {
    check_watchdog_timeout();
    check_communication_timeouts();
    cleanup_expired_violations();
}

void SafetyModule::check_watchdog_timeout() {
    if (is_watchdog_timeout()) {
        watchdog_timeouts_++;
        add_violation(SafetyViolation(SafetyViolationType::WATCHDOG_TIMEOUT, 0,
                      "Watchdog timeout detected", 0.0, 0.0));
    }
}

void SafetyModule::check_communication_timeouts() {
    for (int i = 0; i < motor_count_; ++i) {
        if (is_communication_timeout(i)) {
            communication_timeouts_++;
            add_violation(SafetyViolation(SafetyViolationType::COMMUNICATION_ERROR, i,
                          "Communication timeout", 0.0, 0.0));
        }
    }
}

void SafetyModule::cleanup_expired_violations() {
    std::lock_guard<std::mutex> lock(violations_mutex_);

    auto now = std::chrono::steady_clock::now();
    auto it = std::remove_if(violations_.begin(), violations_.end(),
        [now](const SafetyViolation& v) {
            // Remove violations older than 1 hour
            auto age = std::chrono::duration_cast<std::chrono::hours>(now - v.timestamp);
            return age.count() > 1 && !v.active;
        });

    violations_.erase(it, violations_.end());
}

void SafetyModule::trigger_violation_callback(const SafetyViolation& violation) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (violation_callback_) {
        try {
            violation_callback_(violation);
        } catch (const std::exception& e) {
            std::cerr << "Violation callback error: " << e.what() << std::endl;
        }
    }
}

void SafetyModule::trigger_emergency_stop_callback(const std::string& reason) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (emergency_stop_callback_) {
        try {
            emergency_stop_callback_(reason);
        } catch (const std::exception& e) {
            std::cerr << "Emergency stop callback error: " << e.what() << std::endl;
        }
    }
}

std::string SafetyModule::violation_type_to_string(SafetyViolationType type) const {
    switch (type) {
        case SafetyViolationType::POSITION_LIMIT: return "POSITION_LIMIT";
        case SafetyViolationType::VELOCITY_LIMIT: return "VELOCITY_LIMIT";
        case SafetyViolationType::TORQUE_LIMIT: return "TORQUE_LIMIT";
        case SafetyViolationType::TEMPERATURE_LIMIT: return "TEMPERATURE_LIMIT";
        case SafetyViolationType::COMMUNICATION_ERROR: return "COMMUNICATION_ERROR";
        case SafetyViolationType::MOTOR_ERROR: return "MOTOR_ERROR";
        case SafetyViolationType::EMERGENCY_STOP: return "EMERGENCY_STOP";
        case SafetyViolationType::WATCHDOG_TIMEOUT: return "WATCHDOG_TIMEOUT";
        case SafetyViolationType::SOFTWARE_ERROR: return "SOFTWARE_ERROR";
        default: return "UNKNOWN";
    }
}

bool SafetyModule::is_valid_motor_id(int motor_id) const {
    return motor_id >= 0 && motor_id < motor_count_;
}

std::chrono::steady_clock::time_point SafetyModule::get_current_time() const {
    return std::chrono::steady_clock::now();
}

} // namespace ic_can