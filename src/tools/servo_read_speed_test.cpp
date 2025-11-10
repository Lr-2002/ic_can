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
 * @file servo_read_speed_test.cpp
 * @brief Test USB servo read-only performance
 *
 * This program tests if read-only operations are faster than read-write
 * operations for the USB servo communication.
 */

#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>

#include "ic_can/core/gripper_component.hpp"

struct ReadTestStats {
    uint64_t reads_attempted = 0;
    uint64_t reads_successful = 0;
    uint64_t reads_failed = 0;
    double min_read_time_ms = 999999.0;
    double max_read_time_ms = 0.0;
    double total_read_time_ms = 0.0;
    std::atomic<bool> running{true};
};

void test_read_only_performance(ic_can::GripperComponent& gripper, ReadTestStats& stats,
                               double duration_sec) {
    std::cout << "🚀 Starting READ-ONLY performance test:" << std::endl;
    std::cout << "   Duration: " << duration_sec << " seconds" << std::endl;
    std::cout << "   Testing pure position reading without commands" << std::endl;
    std::cout << std::endl;

    const auto end_time = std::chrono::steady_clock::now() + std::chrono::seconds(static_cast<int64_t>(duration_sec));

    while (std::chrono::steady_clock::now() < end_time && stats.running) {
        auto start_time = std::chrono::high_resolution_clock::now();

        // READ-ONLY: Only read position, no commands
        uint16_t position = gripper.read_servo_position();

        auto end_time_read = std::chrono::high_resolution_clock::now();
        auto read_time = std::chrono::duration<double, std::milli>(end_time_read - start_time).count();

        // Update statistics
        stats.reads_attempted++;
        if (position > 0) {
            stats.reads_successful++;
            stats.total_read_time_ms += read_time;
            stats.min_read_time_ms = std::min(stats.min_read_time_ms, read_time);
            stats.max_read_time_ms = std::max(stats.max_read_time_ms, read_time);
        } else {
            stats.reads_failed++;
        }

        // Small delay to avoid overwhelming the USB
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void print_read_results(const ReadTestStats& stats, double duration_sec) {
    std::cout << std::endl;
    std::cout << "📊 Read-Only Test Results:" << std::endl;
    std::cout << "========================" << std::endl;

    double actual_freq = stats.reads_attempted / duration_sec;
    double success_rate = (stats.reads_successful * 100.0) / stats.reads_attempted;
    double avg_read_time = stats.reads_successful > 0 ?
        stats.total_read_time_ms / stats.reads_successful : 0.0;

    std::cout << "Test duration:       " << std::fixed << std::setprecision(1) << duration_sec << " seconds" << std::endl;
    std::cout << "Reads attempted:     " << stats.reads_attempted << std::endl;
    std::cout << "Reads successful:    " << stats.reads_successful << std::endl;
    std::cout << "Reads failed:        " << stats.reads_failed << std::endl;
    std::cout << "Actual read frequency: " << std::fixed << std::setprecision(1) << actual_freq << " Hz" << std::endl;
    std::cout << "Success rate:        " << std::fixed << std::setprecision(1) << success_rate << "%" << std::endl;
    std::cout << "Min read time:       " << std::fixed << std::setprecision(3) << stats.min_read_time_ms << " ms" << std::endl;
    std::cout << "Max read time:       " << std::fixed << std::setprecision(3) << stats.max_read_time_ms << " ms" << std::endl;
    std::cout << "Avg read time:       " << std::fixed << std::setprecision(3) << avg_read_time << " ms" << std::endl;

    // Performance assessment
    std::cout << std::endl;
    std::cout << "🎯 Read-Only Performance Assessment:" << std::endl;
    if (avg_read_time < 20.0) {
        std::cout << "✅ EXCELLENT: Read-only operations are fast (" << avg_read_time << " ms avg)" << std::endl;
    } else if (avg_read_time < 50.0) {
        std::cout << "✅ GOOD: Read-only operations are reasonably fast (" << avg_read_time << " ms avg)" << std::endl;
    } else if (avg_read_time < 100.0) {
        std::cout << "⚠️  MARGINAL: Read-only operations are slow (" << avg_read_time << " ms avg)" << std::endl;
    } else {
        std::cout << "❌ POOR: Read-only operations are very slow (" << avg_read_time << " ms avg)" << std::endl;
    }
}

int main() {
    std::cout << "=== USB Servo Read-Only Speed Test ===" << std::endl;
    std::cout << "Testing if read-only operations are faster than read-write" << std::endl;
    std::cout << std::endl;

    // Create gripper component
    ic_can::GripperComponent gripper;

    // Don't enable servo - just test reading
    std::cout << "📖 Testing read-only operations (servo not enabled)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Run read-only test
    ReadTestStats stats;
    test_read_only_performance(gripper, stats, 10.0);

    // Print results
    print_read_results(stats, 10.0);

    std::cout << std::endl;
    std::cout << "✅ Read-only speed test completed!" << std::endl;

    return 0;
}