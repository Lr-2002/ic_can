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
 * @file communication_speed_test.cpp
 * @brief Communication Speed and Frequency Test
 *
 * Tests the actual communication speed and frequency of the IC_CAN system.
 * Measures:
 * - Commands sent per second
 * - Responses received per second
 * - Full communication loop performance
 */

#include <chrono>
#include <csignal>
#include <ic_can/core/ic_can.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <iomanip>

static volatile bool g_running = true;

void signal_handler(int signal) {
    std::cout << "\n⚠️  Received signal " << signal << ", stopping test..." << std::endl;
    g_running = false;
}

class CommunicationSpeedTest {
public:
    CommunicationSpeedTest()
        : commands_sent_(0), responses_received_(0), test_duration_sec_(10) {}

    void run_test(ic_can::IC_CAN& controller) {
        // Set up a way to count responses by checking position changes
        last_positions_ = controller.get_joint_positions();
        std::cout << "\n🚀 Starting Communication Speed Test" << std::endl;
        std::cout << "Test Duration: " << test_duration_sec_ << " seconds" << std::endl;
        std::cout << "Testing MIT control with (pos=0, vel=0, torque=0, kp=0, kd=0)" << std::endl;
        std::cout << "Target: Motors 1-6" << std::endl;
        std::cout << "\nPress Ctrl+C to stop early" << std::endl;

        // Reset counters
        commands_sent_.store(0);
        responses_received_.store(0);

        // Get initial position for comparison
        controller.refresh_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto initial_positions = controller.get_joint_positions();
        std::vector<double> initial_velocities = controller.get_joint_velocities();
        std::vector<double> initial_torques = controller.get_joint_torques();

        std::cout << "\n📊 Initial State:" << std::endl;
        std::cout << "Positions: ";
        for (int i = 0; i < 6; i++) {
            std::cout << "J" << (i+1) << "=" << std::fixed << std::setprecision(3)
                      << initial_positions[i] << "rad ";
        }
        std::cout << std::endl;

        // Start test
        auto start_time = std::chrono::steady_clock::now();
        auto last_report_time = start_time;

        // Test command: zero position, zero velocity, zero torque, zero gains
        std::vector<double> zero_positions(9, 0.0);
        std::vector<double> zero_velocities(9, 0.0);
        std::vector<double> zero_torques(9, 0.0);

        std::cout << "\n🔄 Running test..." << std::endl;
        std::cout << "Time (s) | Commands/s | Responses/s | Total Sent | Total Recv | Status" << std::endl;
        std::cout << "---------|------------|-------------|------------|------------|--------" << std::endl;

        while (g_running) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                current_time - start_time).count();

            if (elapsed >= test_duration_sec_) {
                break;
            }

            // Send MIT commands to motors 1-6
            if (controller.set_joint_positions(zero_positions, zero_velocities, zero_torques)) {
                commands_sent_.fetch_add(1);
            }

            // Request status update to trigger responses
            controller.refresh_all();

            // Check for responses by monitoring position updates
            check_for_responses(controller);

            // Small delay to avoid overwhelming the bus
            std::this_thread::sleep_for(std::chrono::microseconds(100));

            // Report every second
            auto report_elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                current_time - last_report_time).count();

            if (report_elapsed >= 1) {
                print_performance_report(current_time - start_time);
                last_report_time = current_time;
            }
        }

        auto total_time = std::chrono::steady_clock::now() - start_time;
        print_final_results(total_time, controller);
    }

    void check_for_responses(ic_can::IC_CAN& controller) {
        auto current_positions = controller.get_joint_positions();

        // Check if any of the first 6 motors have updated their positions
        for (int i = 0; i < 6; i++) {
            if (std::abs(current_positions[i] - last_positions_[i]) > 0.0001) {
                responses_received_.fetch_add(1);
                last_positions_[i] = current_positions[i];
            }
        }
    }

private:
    void print_performance_report(std::chrono::steady_clock::duration elapsed) {
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        double elapsed_sec = elapsed_ms / 1000.0;

        uint64_t commands = commands_sent_.load();
        uint64_t responses = responses_received_.load();

        double cmd_per_sec = commands / elapsed_sec;
        double resp_per_sec = responses / elapsed_sec;

        std::cout << std::setw(8) << std::fixed << std::setprecision(1) << elapsed_sec << " | "
                  << std::setw(10) << std::setprecision(1) << cmd_per_sec << " | "
                  << std::setw(11) << std::setprecision(1) << resp_per_sec << " | "
                  << std::setw(10) << commands << " | "
                  << std::setw(10) << responses << " | "
                  << (cmd_per_sec > 100 ? "✅ FAST" : cmd_per_sec > 50 ? "⚠️  OK" : "❌ SLOW") << std::endl;
    }

    void print_final_results(std::chrono::steady_clock::duration total_time, ic_can::IC_CAN& controller) {
        auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(total_time).count();
        double total_sec = total_ms / 1000.0;

        uint64_t final_commands = commands_sent_.load();
        uint64_t final_responses = responses_received_.load();

        double avg_cmd_per_sec = final_commands / total_sec;
        double avg_resp_per_sec = final_responses / total_sec;
        double response_rate = (final_commands > 0) ? (double(final_responses) / final_commands * 100.0) : 0.0;

        std::cout << "\n🎯 FINAL RESULTS:" << std::endl;
        std::cout << "=====================================" << std::endl;
        std::cout << "Test Duration: " << std::fixed << std::setprecision(2) << total_sec << " seconds" << std::endl;
        std::cout << "Total Commands Sent: " << final_commands << std::endl;
        std::cout << "Total Responses Received: " << final_responses << std::endl;
        std::cout << "Average Command Rate: " << std::setprecision(1) << avg_cmd_per_sec << " cmd/s" << std::endl;
        std::cout << "Average Response Rate: " << std::setprecision(1) << avg_resp_per_sec << " resp/s" << std::endl;
        std::cout << "Response Success Rate: " << std::setprecision(1) << response_rate << "%" << std::endl;

        // Get final positions
        controller.refresh_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto final_positions = controller.get_joint_positions();

        std::cout << "\n📊 Final Motor Positions:" << std::endl;
        for (int i = 0; i < 6; i++) {
            double change = final_positions[i] - 0.0; // Change from zero position
            std::cout << "Motor " << (i+1) << ": " << std::setprecision(4) << final_positions[i]
                      << " rad (change: " << std::setprecision(4) << change << ")" << std::endl;
        }

        // Performance assessment
        std::cout << "\n🏆 PERFORMANCE ASSESSMENT:" << std::endl;
        if (avg_cmd_per_sec >= 500) {
            std::cout << "✅ EXCELLENT: Achieved 500Hz+ command rate!" << std::endl;
        } else if (avg_cmd_per_sec >= 200) {
            std::cout << "✅ GOOD: Achieved high-frequency control (>200Hz)" << std::endl;
        } else if (avg_cmd_per_sec >= 100) {
            std::cout << "⚠️  ACCEPTABLE: Moderate frequency (>100Hz)" << std::endl;
        } else {
            std::cout << "❌ POOR: Low frequency (<100Hz) - may need optimization" << std::endl;
        }

        if (response_rate >= 90) {
            std::cout << "✅ EXCELLENT: High response rate (>90%)" << std::endl;
        } else if (response_rate >= 70) {
            std::cout << "✅ GOOD: Good response rate (>70%)" << std::endl;
        } else if (response_rate >= 50) {
            std::cout << "⚠️  ACCEPTABLE: Moderate response rate (>50%)" << std::endl;
        } else {
            std::cout << "❌ POOR: Low response rate (<50%) - check connections" << std::endl;
        }
    }

    std::atomic<uint64_t> commands_sent_;
    std::atomic<uint64_t> responses_received_;
    std::vector<double> last_positions_;
    int test_duration_sec_;
};

int main() {
    std::cout << "=== IC_CAN Communication Speed Test ===" << std::endl;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        // Create IC_CAN controller
        std::cout << "🔧 Creating IC_CAN controller..." << std::endl;
        auto controller = std::make_unique<ic_can::IC_CAN>("F561E08C892274DB09496BCC1102DBC5", true);

        // Initialize the system
        std::cout << "🚀 Initializing IC_CAN system..." << std::endl;
        if (!controller->initialize()) {
            std::cout << "❌ FAILED: System initialization failed" << std::endl;
            return -1;
        }
        std::cout << "✅ SUCCESS: System initialized" << std::endl;

        // Print system information
        controller->print_system_info();

        // Enable all motors
        std::cout << "\n⚡ Enabling all motors..." << std::endl;
        if (!controller->enable_all()) {
            std::cout << "⚠️  WARNING: Some motors failed to enable" << std::endl;
        } else {
            std::cout << "✅ All motors enabled successfully" << std::endl;
        }

        // Wait for motors to initialize
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // Create and run speed test
        CommunicationSpeedTest speed_test;
        speed_test.run_test(*controller);

        // Disable all motors
        std::cout << "\n🔌 Disabling all motors..." << std::endl;
        if (!controller->disable_all()) {
            std::cout << "⚠️  WARNING: Some motors failed to disable" << std::endl;
        } else {
            std::cout << "✅ All motors disabled successfully" << std::endl;
        }

        std::cout << "\n🎉 Communication Speed Test Completed!" << std::endl;

        return 0;

    } catch (const std::exception &e) {
        std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
        return -1;
    }
}