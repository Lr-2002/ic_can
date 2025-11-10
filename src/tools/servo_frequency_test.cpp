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
 * @file servo_frequency_test.cpp
 * @brief High-frequency servo motor testing
 *
 * This program tests the maximum command frequency that the USB servo
 * can handle, measuring response times and command throughput.
 */

#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>

#include "ic_can/core/gripper_component.hpp"

struct TestStats {
    uint64_t commands_sent = 0;
    uint64_t commands_success = 0;
    uint64_t commands_failed = 0;
    double min_response_time_ms = 999999.0;
    double max_response_time_ms = 0.0;
    double total_response_time_ms = 0.0;
    std::atomic<bool> running{true};
};

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --usb-port PORT    USB port for servo (default: /dev/ttyUSB0)" << std::endl;
    std::cout << "  --freq HZ          Target frequency in Hz (default: 100)" << std::endl;
    std::cout << "  --duration SEC     Test duration in seconds (default: 10)" << std::endl;
    std::cout << "  --help              Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Test Modes:" << std::endl;
    std::cout << "  --freq 100         Test 100 Hz (10ms intervals)" << std::endl;
    std::cout << "  --freq 200         Test 200 Hz (5ms intervals)" << std::endl;
    std::cout << "  --freq 500         Test 500 Hz (2ms intervals)" << std::endl;
    std::cout << "  --freq 1000        Test 1000 Hz (1ms intervals)" << std::endl;
}

void run_frequency_test(ic_can::GripperComponent& gripper, TestStats& stats,
                        double target_freq_hz, double duration_sec) {

    const double interval_ms = 1000.0 / target_freq_hz;
    const auto interval = std::chrono::microseconds(static_cast<int64_t>(interval_ms * 1000));
    const auto end_time = std::chrono::steady_clock::now() + std::chrono::seconds(static_cast<int64_t>(duration_sec));

    std::cout << "🚀 Starting frequency test:" << std::endl;
    std::cout << "   Target frequency: " << target_freq_hz << " Hz" << std::endl;
    std::cout << "   Interval: " << interval_ms << " ms" << std::endl;
    std::cout << "   Duration: " << duration_sec << " seconds" << std::endl;
    std::cout << "   Expected commands: " << static_cast<int64_t>(target_freq_hz * duration_sec) << std::endl;
    std::cout << std::endl;

    uint16_t position = 1000;
    uint8_t direction = 1; // 1 = increasing, 0 = decreasing
    int cycle_count = 0;

    while (std::chrono::steady_clock::now() < end_time && stats.running) {
        auto start_time = std::chrono::high_resolution_clock::now();

        // Generate position pattern (sine wave-like)
        if (direction) {
            position += 50;
            if (position >= 2100) {
                position = 2100;
                direction = 0;
            }
        } else {
            position -= 50;
            if (position <= 1000) {
                position = 1000;
                direction = 1;
                cycle_count++;
            }
        }

        // Send position command
        bool success = gripper.set_openness(
            (position - 1000.0) / 1100.0,  // Convert to 0.0-1.0
            0.5,  // Medium speed
            0.3   // Medium force
        );

        auto end_time_cmd = std::chrono::high_resolution_clock::now();
        auto response_time = std::chrono::duration<double, std::milli>(end_time_cmd - start_time).count();

        // Update statistics
        stats.commands_sent++;
        if (success) {
            stats.commands_success++;
            stats.total_response_time_ms += response_time;
            stats.min_response_time_ms = std::min(stats.min_response_time_ms, response_time);
            stats.max_response_time_ms = std::max(stats.max_response_time_ms, response_time);
        } else {
            stats.commands_failed++;
        }

        // Wait for next interval
        auto next_time = start_time + interval;
        std::this_thread::sleep_until(next_time);
    }
}

void print_results(const TestStats& stats, double target_freq_hz, double duration_sec) {
    std::cout << std::endl;
    std::cout << "📊 Test Results:" << std::endl;
    std::cout << "================" << std::endl;

    double actual_freq = stats.commands_sent / duration_sec;
    double success_rate = (stats.commands_success * 100.0) / stats.commands_sent;
    double avg_response_time = stats.commands_success > 0 ?
        stats.total_response_time_ms / stats.commands_success : 0.0;

    std::cout << "Target frequency:   " << std::fixed << std::setprecision(1) << target_freq_hz << " Hz" << std::endl;
    std::cout << "Actual frequency:   " << std::fixed << std::setprecision(1) << actual_freq << " Hz" << std::endl;
    std::cout << "Commands sent:      " << stats.commands_sent << std::endl;
    std::cout << "Commands success:   " << stats.commands_success << std::endl;
    std::cout << "Commands failed:    " << stats.commands_failed << std::endl;
    std::cout << "Success rate:       " << std::fixed << std::setprecision(1) << success_rate << "%" << std::endl;
    std::cout << "Min response time:  " << std::fixed << std::setprecision(3) << stats.min_response_time_ms << " ms" << std::endl;
    std::cout << "Max response time:  " << std::fixed << std::setprecision(3) << stats.max_response_time_ms << " ms" << std::endl;
    std::cout << "Avg response time:  " << std::fixed << std::setprecision(3) << avg_response_time << " ms" << std::endl;

    // Performance assessment
    std::cout << std::endl;
    std::cout << "🎯 Performance Assessment:" << std::endl;
    if (success_rate >= 95.0 && actual_freq >= target_freq_hz * 0.95) {
        std::cout << "✅ EXCELLENT: Motor handles " << target_freq_hz << " Hz very well" << std::endl;
    } else if (success_rate >= 90.0 && actual_freq >= target_freq_hz * 0.90) {
        std::cout << "✅ GOOD: Motor handles " << target_freq_hz << " Hz adequately" << std::endl;
    } else if (success_rate >= 75.0) {
        std::cout << "⚠️  MARGINAL: Motor struggles at " << target_freq_hz << " Hz" << std::endl;
    } else {
        std::cout << "❌ POOR: Motor cannot handle " << target_freq_hz << " Hz reliably" << std::endl;
    }

    if (avg_response_time > 5.0) {
        std::cout << "⚠️  High latency detected (" << avg_response_time << " ms avg)" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::string usb_port = "/dev/ttyUSB0";
    double target_freq = 100.0;
    double duration = 10.0;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "--usb-port" && i + 1 < argc) {
            usb_port = argv[++i];
        } else if (arg == "--freq" && i + 1 < argc) {
            target_freq = std::stod(argv[++i]);
        } else if (arg == "--duration" && i + 1 < argc) {
            duration = std::stod(argv[++i]);
        }
    }

    std::cout << "=== USB Servo High-Frequency Test ===" << std::endl;
    std::cout << "USB Port: " << usb_port << std::endl;
    std::cout << std::endl;

    // Create gripper component
    ic_can::GripperComponent gripper;

    // Enable servo
    std::cout << "🔌 Enabling servo..." << std::endl;
    if (!gripper.open(0.5, 0.3)) {
        std::cout << "❌ Failed to enable servo" << std::endl;
        return 1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Run frequency test
    TestStats stats;
    run_frequency_test(gripper, stats, target_freq, duration);

    // Print results
    print_results(stats, target_freq, duration);

    // Move to safe position and stop
    std::cout << std::endl;
    std::cout << "🏠 Moving to safe position..." << std::endl;
    gripper.set_openness(0.5, 0.5, 0.3);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "🛑 Stopping servo..." << std::endl;
    gripper.stop();

    std::cout << std::endl;
    std::cout << "✅ High-frequency test completed!" << std::endl;

    return 0;
}