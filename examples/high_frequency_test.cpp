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
 * @file high_frequency_test.cpp
 * @brief High-Frequency Control Test
 *
 * Tests the maximum achievable frequency of the communication loop.
 * Pushes the system to its limits to measure real performance.
 */

#include <chrono>
#include <csignal>
#include <ic_can/core/ic_can.hpp>
#include <iostream>
#include <iomanip>
#include <thread>
#include <atomic>

static volatile bool g_running = true;

void signal_handler(int signal) {
    std::cout << "\n⚠️  Received signal " << signal << ", stopping test..." << std::endl;
    g_running = false;
}

int main() {
    std::cout << "=== IC_CAN High-Frequency Test ===" << std::endl;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        // Create IC_CAN controller
        std::cout << "🔧 Creating IC_CAN controller..." << std::endl;
        auto controller = std::make_unique<ic_can::IC_CAN>("F561E08C892274DB09496BCC1102DBC5", true);

        // Initialize the system
        if (!controller->initialize()) {
            std::cout << "❌ FAILED: System initialization failed" << std::endl;
            return -1;
        }
        std::cout << "✅ SUCCESS: System initialized" << std::endl;

        // Enable all motors
        if (!controller->enable_all()) {
            std::cout << "⚠️  WARNING: Some motors failed to enable" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        std::cout << "\n🚀 Starting High-Frequency Test (10 seconds)" << std::endl;
        std::cout << "Sending MIT commands at maximum speed..." << std::endl;

        // Test parameters
        std::atomic<uint64_t> loop_count{0};
        std::atomic<uint64_t> success_count{0};

        auto start_time = std::chrono::steady_clock::now();
        auto last_report = start_time;

        // Zero commands for testing
        std::vector<double> zero_positions(9, 0.0);
        std::vector<double> zero_velocities(9, 0.0);
        std::vector<double> zero_torques(9, 0.0);

        while (g_running) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                current_time - start_time).count();

            if (elapsed >= 10) break; // 10 second test

            // Send commands as fast as possible
            if (controller->set_joint_positions(zero_positions, zero_velocities, zero_torques)) {
                success_count.fetch_add(1);
            }

            // Request status
            controller->refresh_all();

            loop_count.fetch_add(1);

            // Report every 2 seconds
            auto report_elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                current_time - last_report).count();

            if (report_elapsed >= 2) {
                auto report_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    current_time - start_time).count();
                double report_sec = report_ms / 1000.0;

                uint64_t loops = loop_count.load();
                uint64_t successes = success_count.load();

                std::cout << "Time: " << std::fixed << std::setprecision(1) << report_sec << "s, "
                          << "Frequency: " << std::setprecision(1) << (loops / report_sec) << " Hz, "
                          << "Success Rate: " << std::setprecision(1) << (double(successes) / loops * 100.0) << "%" << std::endl;

                last_report = current_time;
            }

            // No artificial delay - maximum speed test
        }

        auto total_time = std::chrono::steady_clock::now() - start_time;
        auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(total_time).count();
        double total_sec = total_ms / 1000.0;

        uint64_t final_loops = loop_count.load();
        uint64_t final_successes = success_count.load();

        std::cout << "\n🎯 HIGH-FREQUENCY TEST RESULTS:" << std::endl;
        std::cout << "======================================" << std::endl;
        std::cout << "Total Test Time: " << std::setprecision(3) << total_sec << " seconds" << std::endl;
        std::cout << "Total Loops: " << final_loops << std::endl;
        std::cout << "Successful Commands: " << final_successes << std::endl;
        std::cout << "Average Frequency: " << std::setprecision(1) << (final_loops / total_sec) << " Hz" << std::endl;
        std::cout << "Success Rate: " << std::setprecision(1) << (double(final_successes) / final_loops * 100.0) << "%" << std::endl;

        // Performance assessment
        double avg_freq = final_loops / total_sec;
        std::cout << "\n🏆 PERFORMANCE ASSESSMENT:" << std::endl;
        if (avg_freq >= 500) {
            std::cout << "🚀 OUTSTANDING: Achieved 500Hz+ - ready for real-time control!" << std::endl;
        } else if (avg_freq >= 200) {
            std::cout << "✅ EXCELLENT: High-frequency performance achieved!" << std::endl;
        } else if (avg_freq >= 100) {
            std::cout << "✅ GOOD: Suitable for most control applications" << std::endl;
        } else if (avg_freq >= 50) {
            std::cout << "⚠️  ACCEPTABLE: May work for slow control loops" << std::endl;
        } else {
            std::cout << "❌ NEEDS OPTIMIZATION: Too slow for real-time control" << std::endl;
        }

        // Test built-in 500Hz control
        std::cout << "\n🔄 Testing Built-in 500Hz Control (5 seconds)..." << std::endl;
        if (controller->start_high_frequency_control()) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            controller->stop_high_frequency_control();
            std::cout << "✅ Built-in 500Hz control test completed" << std::endl;
        } else {
            std::cout << "❌ Failed to start built-in high-frequency control" << std::endl;
        }

        // Disable all motors
        controller->disable_all();

        std::cout << "\n🎉 High-Frequency Test Completed!" << std::endl;

        return 0;

    } catch (const std::exception &e) {
        std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
        return -1;
    }
}