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
 * @file response_counting_test.cpp
 * @brief Response Counting Test with Direct Feedback Monitoring
 *
 * This test focuses specifically on counting motor responses accurately
 * by monitoring the IC_CAN system's internal state changes.
 */

#include <chrono>
#include <csignal>
#include <ic_can/core/ic_can.hpp>
#include <iostream>
#include <iomanip>
#include <thread>
#include <atomic>
#include <vector>

static volatile bool g_running = true;

void signal_handler(int signal) {
    std::cout << "\n⚠️  Received signal " << signal << ", stopping test..." << std::endl;
    g_running = false;
}

class ResponseCountingTest {
public:
    ResponseCountingTest() : test_duration_sec_(10) {}

    void run_test(ic_can::IC_CAN& controller) {
        std::cout << "\n🚀 Starting Response Counting Test" << std::endl;
        std::cout << "Test Duration: " << test_duration_sec_ << " seconds" << std::endl;
        std::cout << "Monitoring motor feedback responses" << std::endl;
        std::cout << "\nPress Ctrl+C to stop early" << std::endl;

        // Initialize counters
        commands_sent.store(0);
        responses_detected.store(0);

        // Get baseline state
        controller.refresh_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        auto baseline_positions = controller.get_joint_positions();
        auto baseline_velocities = controller.get_joint_velocities();
        auto baseline_torques = controller.get_joint_torques();

        std::cout << "\n📊 Baseline State:" << std::endl;
        std::cout << "Positions: ";
        for (int i = 0; i < 6; i++) {
            std::cout << "J" << (i+1) << "=" << std::fixed << std::setprecision(4)
                      << baseline_positions[i] << " ";
        }
        std::cout << std::endl;

        // Store last known state
        last_positions = baseline_positions;
        last_velocities = baseline_velocities;
        last_torques = baseline_torques;

        auto start_time = std::chrono::steady_clock::now();
        auto last_report_time = start_time;

        std::cout << "\n🔄 Running test..." << std::endl;
        std::cout << "Time (s) | Commands/s | Responses/s | Success Rate | Status" << std::endl;
        std::cout << "---------|------------|-------------|--------------|--------" << std::endl;

        // Test commands
        std::vector<double> zero_positions(9, 0.0);
        std::vector<double> zero_velocities(9, 0.0);
        std::vector<double> zero_torques(9, 0.0);

        while (g_running) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                current_time - start_time).count();

            if (elapsed >= test_duration_sec_) break;

            // Send command
            if (controller.set_joint_positions(zero_positions, zero_velocities, zero_torques)) {
                commands_sent.fetch_add(1);
            }

            // Request status
            controller.refresh_all();

            // Wait a moment for responses
            std::this_thread::sleep_for(std::chrono::microseconds(500));

            // Check for state changes (indicating responses)
            detect_responses(controller);

            // Report every second
            auto report_elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                current_time - last_report_time).count();

            if (report_elapsed >= 1) {
                print_performance_report(current_time - start_time);
                last_report_time = current_time;
            }

            // Small delay between iterations
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }

        auto total_time = std::chrono::steady_clock::now() - start_time;
        print_final_results(total_time, controller);
    }

private:
    void detect_responses(ic_can::IC_CAN& controller) {
        auto current_positions = controller.get_joint_positions();
        auto current_velocities = controller.get_joint_velocities();
        auto current_torques = controller.get_joint_torques();

        // Check for any changes in motors 1-6
        bool response_detected = false;
        const double position_threshold = 0.0001;
        const double velocity_threshold = 0.001;
        const double torque_threshold = 0.001;

        for (int i = 0; i < 6; i++) {
            // Check position changes
            if (std::abs(current_positions[i] - last_positions[i]) > position_threshold) {
                response_detected = true;
                last_positions[i] = current_positions[i];
            }

            // Check velocity changes
            if (std::abs(current_velocities[i] - last_velocities[i]) > velocity_threshold) {
                response_detected = true;
                last_velocities[i] = current_velocities[i];
            }

            // Check torque changes
            if (std::abs(current_torques[i] - last_torques[i]) > torque_threshold) {
                response_detected = true;
                last_torques[i] = current_torques[i];
            }
        }

        if (response_detected) {
            responses_detected.fetch_add(1);
        }
    }

    void print_performance_report(std::chrono::steady_clock::duration elapsed) {
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        double elapsed_sec = elapsed_ms / 1000.0;

        uint64_t cmds = commands_sent.load();
        uint64_t resp = responses_detected.load();

        double cmd_per_sec = cmds / elapsed_sec;
        double resp_per_sec = resp / elapsed_sec;
        double success_rate = (cmds > 0) ? (double(resp) / cmds * 100.0) : 0.0;

        std::cout << std::setw(8) << std::fixed << std::setprecision(1) << elapsed_sec << " | "
                  << std::setw(10) << std::setprecision(1) << cmd_per_sec << " | "
                  << std::setw(11) << std::setprecision(1) << resp_per_sec << " | "
                  << std::setw(12) << std::setprecision(1) << success_rate << "% | "
                  << (success_rate > 80 ? "✅ GOOD" : success_rate > 50 ? "⚠️  OK" : "❌ POOR") << std::endl;
    }

    void print_final_results(std::chrono::steady_clock::duration total_time, ic_can::IC_CAN& controller) {
        auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(total_time).count();
        double total_sec = total_ms / 1000.0;

        uint64_t final_commands = commands_sent.load();
        uint64_t final_responses = responses_detected.load();

        double avg_cmd_per_sec = final_commands / total_sec;
        double avg_resp_per_sec = final_responses / total_sec;
        double success_rate = (final_commands > 0) ? (double(final_responses) / final_commands * 100.0) : 0.0;

        std::cout << "\n🎯 FINAL RESPONSE COUNTING RESULTS:" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Test Duration: " << std::fixed << std::setprecision(2) << total_sec << " seconds" << std::endl;
        std::cout << "Total Commands Sent: " << final_commands << std::endl;
        std::cout << "Total Responses Detected: " << final_responses << std::endl;
        std::cout << "Average Command Rate: " << std::setprecision(1) << avg_cmd_per_sec << " cmd/s" << std::endl;
        std::cout << "Average Response Rate: " << std::setprecision(1) << avg_resp_per_sec << " resp/s" << std::endl;
        std::cout << "Response Success Rate: " << std::setprecision(1) << success_rate << "%" << std::endl;

        // Get final state
        controller.refresh_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        auto final_positions = controller.get_joint_positions();

        std::cout << "\n📊 Final Motor Analysis:" << std::endl;
        for (int i = 0; i < 6; i++) {
            std::cout << "Motor " << (i+1) << ": " << std::setprecision(6) << final_positions[i]
                      << " rad (from " << std::setprecision(6) << last_positions[i] << ")" << std::endl;
        }

        // Communication analysis
        std::cout << "\n🔍 COMMUNICATION ANALYSIS:" << std::endl;
        if (success_rate >= 90) {
            std::cout << "✅ EXCELLENT: Near-perfect response rate (>90%)" << std::endl;
        } else if (success_rate >= 70) {
            std::cout << "✅ GOOD: Reliable communication (>70%)" << std::endl;
        } else if (success_rate >= 50) {
            std::cout << "⚠️  ACCEPTABLE: Moderate reliability (>50%)" << std::endl;
        } else {
            std::cout << "❌ POOR: Unreliable communication (<50%)" << std::endl;
        }

        if (avg_cmd_per_sec >= 100) {
            std::cout << "✅ EXCELLENT: High-frequency command rate (>100Hz)" << std::endl;
        } else if (avg_cmd_per_sec >= 50) {
            std::cout << "✅ GOOD: Adequate command rate (>50Hz)" << std::endl;
        } else {
            std::cout << "❌ POOR: Low command rate (<50Hz)" << std::endl;
        }
    }

    std::atomic<uint64_t> commands_sent;
    std::atomic<uint64_t> responses_detected;
    std::vector<double> last_positions;
    std::vector<double> last_velocities;
    std::vector<double> last_torques;
    int test_duration_sec_;
};

int main() {
    std::cout << "=== IC_CAN Response Counting Test ===" << std::endl;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        auto controller = std::make_unique<ic_can::IC_CAN>("F561E08C892274DB09496BCC1102DBC5", true);

        if (!controller->initialize()) {
            std::cout << "❌ FAILED: System initialization failed" << std::endl;
            return -1;
        }

        controller->print_system_info();

        if (!controller->enable_all()) {
            std::cout << "⚠️  WARNING: Some motors failed to enable" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        ResponseCountingTest test;
        test.run_test(*controller);

        controller->disable_all();

        std::cout << "\n🎉 Response Counting Test Completed!" << std::endl;
        return 0;

    } catch (const std::exception &e) {
        std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
        return -1;
    }
}