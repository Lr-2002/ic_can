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

/**
 * @file motor_profiler.hpp
 * @brief Standalone Motor Communication Profiler
 *
 * Independent profiler for analyzing motor communication performance:
 * - Per-motor timeout tracking
 * - CAN frame reception counting
 * - Send/receive timing analysis
 * - Feedback frequency measurement
 * - Comprehensive performance reporting
 */

#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <vector>
#include <algorithm>

/**
 * @brief Motor Communication Profiler
 *
 * Standalone profiler class for tracking motor communication performance.
 * Maps CAN IDs to motor IDs and provides detailed statistics.
 */
class MotorProfiler {
public:
    /**
     * @brief Constructor
     */
    MotorProfiler() {
        start_time_ = std::chrono::high_resolution_clock::now();
        initialize_counters();
    }

    /**
     * @brief Record a CAN frame reception
     * @param can_id The CAN ID received
     */
    void record_can_frame(uint32_t can_id) {
        int motor_id = can_id_to_motor_id(can_id);
        if (motor_id > 0 && motor_id <= 9) {
            feedback_counts_[motor_id]++;
            last_feedback_time_[motor_id] = std::chrono::high_resolution_clock::now();
        }
    }

    /**
     * @brief Record a motor send attempt
     * @param motor_id Motor ID (1-9)
     */
    void record_send_attempt(int motor_id) {
        if (motor_id > 0 && motor_id <= 9) {
            total_send_attempts_[motor_id]++;
        }
    }

    /**
     * @brief Record a timeout event
     * @param motor_id Motor ID (1-9)
     */
    void record_timeout(int motor_id) {
        if (motor_id > 0 && motor_id <= 9) {
            timeout_counts_[motor_id]++;
            total_timeouts_++;
        }
    }

    /**
     * @brief Record timing for send operation
     * @param motor_id Motor ID (1-9)
     * @param send_duration_us Send duration in microseconds
     */
    void record_send_timing(int motor_id, double send_duration_us) {
        if (motor_id > 0 && motor_id <= 9) {
            send_durations_[motor_id].push_back(send_duration_us);
        }
    }

    /**
     * @brief Increment total loop counter
     */
    void increment_loop_count() {
        total_loops_++;
    }

    /**
     * @brief Print comprehensive performance summary
     */
    void print_summary() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration_seconds = std::chrono::duration<double>(now - start_time_).count();

        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "📊 MOTOR PROFILING SUMMARY (ran for "
                  << std::fixed << std::setprecision(1) << duration_seconds << "s)" << std::endl;
        std::cout << std::string(80, '=') << std::endl;

        print_overall_statistics(duration_seconds);
        print_timeout_analysis();
        print_timing_analysis();
        print_feedback_analysis(duration_seconds);
        print_critical_motors();

        std::cout << std::string(80, '=') << std::endl;
    }

    /**
     * @brief Reset all counters
     */
    void reset() {
        initialize_counters();
        start_time_ = std::chrono::high_resolution_clock::now();
    }

    /**
     * @brief Get feedback count for specific motor
     * @param motor_id Motor ID (1-9)
     * @return Feedback count
     */
    int get_feedback_count(int motor_id) const {
        auto it = feedback_counts_.find(motor_id);
        return (it != feedback_counts_.end()) ? it->second : 0;
    }

    /**
     * @brief Get timeout count for specific motor
     * @param motor_id Motor ID (1-9)
     * @return Timeout count
     */
    int get_timeout_count(int motor_id) const {
        auto it = timeout_counts_.find(motor_id);
        return (it != timeout_counts_.end()) ? it->second : 0;
    }

private:
    // Timing
    std::chrono::high_resolution_clock::time_point start_time_;
    std::map<int, std::chrono::high_resolution_clock::time_point> last_feedback_time_;

    // Counters
    std::map<int, int> feedback_counts_;
    std::map<int, int> total_send_attempts_;
    std::map<int, int> timeout_counts_;
    std::map<int, std::vector<double>> send_durations_;

    // Overall statistics
    int total_loops_ = 0;
    int total_timeouts_ = 0;

    /**
     * @brief Initialize all counters to zero
     */
    void initialize_counters() {
        total_loops_ = 0;
        total_timeouts_ = 0;

        for (int i = 1; i <= 9; i++) {
            feedback_counts_[i] = 0;
            total_send_attempts_[i] = 0;
            timeout_counts_[i] = 0;
            send_durations_[i].clear();
        }
    }

    /**
     * @brief Convert CAN ID to motor ID
     * @param can_id CAN ID
     * @return Motor ID (1-9) or -1 if unknown
     */
    int can_id_to_motor_id(uint32_t can_id) {
        // DM motors 1-6: IDs 0x11-0x16
        if (can_id >= 0x11 && can_id <= 0x16) {
            return can_id - 0x10;  // 0x11->1, 0x16->6
        }

        // HT motor 7: IDs 0x700-0x7FF
        if (can_id >= 0x700 && can_id < 0x800) {
            return 7;
        }

        // HT motor 8: IDs 0x800-0x8FF
        if (can_id >= 0x800 && can_id < 0x900) {
            return 8;
        }

        // Servo motor 9: ID 0x19
        if (can_id == 0x19) {
            return 9;
        }

        return -1;  // Unknown CAN ID
    }

    /**
     * @brief Print overall statistics
     */
    void print_overall_statistics(double duration_seconds) {
        std::cout << "🔢 OVERALL STATISTICS:" << std::endl;
        std::cout << "  Total Loops: " << total_loops_ << std::endl;

        if (duration_seconds > 0) {
            std::cout << "  Average Loop Frequency: "
                      << std::fixed << std::setprecision(1)
                      << (total_loops_ / duration_seconds) << " Hz" << std::endl;
        }

        std::cout << "  Total Timeouts: " << total_timeouts_ << std::endl;

        if (total_loops_ > 0) {
            std::cout << "  Overall Timeout Rate: "
                      << std::fixed << std::setprecision(1)
                      << (total_timeouts_ * 100.0 / total_loops_) << "%" << std::endl;
        }

        std::cout << std::endl;
    }

    /**
     * @brief Print timeout analysis per motor
     */
    void print_timeout_analysis() {
        std::cout << "🚨 TIMEOUT ANALYSIS PER MOTOR:" << std::endl;
        std::cout << "Motor | Send Attempts | Timeouts | Timeout Rate" << std::endl;
        std::cout << "------|---------------|----------|--------------" << std::endl;

        for (int i = 1; i <= 9; i++) {
            double timeout_rate = 0.0;
            if (total_send_attempts_[i] > 0) {
                timeout_rate = timeout_counts_[i] * 100.0 / total_send_attempts_[i];
            }

            std::cout << std::setw(5) << i << " | "
                      << std::setw(13) << total_send_attempts_[i] << " | "
                      << std::setw(8) << timeout_counts_[i] << " | "
                      << std::setw(12) << std::fixed << std::setprecision(1)
                      << timeout_rate << "%" << std::endl;
        }
        std::cout << std::endl;
    }

    /**
     * @brief Print timing analysis
     */
    void print_timing_analysis() {
        std::cout << "⏱️ TIMING ANALYSIS (microseconds):" << std::endl;
        std::cout << "Motor | Send Count | Avg Send | Max Send | Min Send" << std::endl;
        std::cout << "------|------------|----------|----------|----------" << std::endl;

        for (int i = 1; i <= 9; i++) {
            const auto& durations = send_durations_[i];

            if (durations.empty()) {
                std::cout << std::setw(5) << i << " | "
                          << std::setw(10) << 0 << " | "
                          << std::setw(8) << "-" << " | "
                          << std::setw(8) << "-" << " | "
                          << std::setw(8) << "-" << std::endl;
                continue;
            }

            double avg = std::accumulate(durations.begin(), durations.end(), 0.0) / durations.size();
            double max = *std::max_element(durations.begin(), durations.end());
            double min = *std::min_element(durations.begin(), durations.end());

            std::cout << std::setw(5) << i << " | "
                      << std::setw(10) << durations.size() << " | "
                      << std::setw(8) << std::fixed << std::setprecision(0) << avg << " | "
                      << std::setw(8) << max << " | "
                      << std::setw(8) << min << std::endl;
        }
        std::cout << std::endl;
    }

    /**
     * @brief Print feedback analysis
     */
    void print_feedback_analysis(double duration_seconds) {
        std::cout << "📡 FEEDBACK ANALYSIS:" << std::endl;
        std::cout << "Motor | Feedback Count | Feedback Frequency" << std::endl;
        std::cout << "------|----------------|-------------------" << std::endl;

        for (int i = 1; i <= 9; i++) {
            double freq = 0.0;
            if (duration_seconds > 0) {
                freq = feedback_counts_[i] / duration_seconds;
            }

            std::cout << std::setw(5) << i << " | "
                      << std::setw(14) << feedback_counts_[i] << " | "
                      << std::setw(17) << std::fixed << std::setprecision(1)
                      << freq << " Hz" << std::endl;
        }
        std::cout << std::endl;
    }

    /**
     * @brief Print critical motors with highest timeout rates
     */
    void print_critical_motors() {
        std::cout << "🔥 CRITICAL MOTORS (highest timeout rates):" << std::endl;

        std::vector<std::pair<int, double>> motor_timeout_rates;
        for (int i = 1; i <= 9; i++) {
            if (total_send_attempts_[i] > 0) {
                double rate = timeout_counts_[i] * 100.0 / total_send_attempts_[i];
                motor_timeout_rates.push_back({i, rate});
            }
        }

        std::sort(motor_timeout_rates.begin(), motor_timeout_rates.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        for (size_t i = 0; i < std::min(size_t(3), motor_timeout_rates.size()); i++) {
            if (motor_timeout_rates[i].second > 0) {
                std::cout << "  #" << (i+1) << ". Motor " << motor_timeout_rates[i].first
                          << " (" << std::fixed << std::setprecision(1)
                          << motor_timeout_rates[i].second << "% timeout rate)" << std::endl;
            }
        }
    }
};