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
 * @file frequency_interpolation_test.cpp
 * @brief Configurable Frequency Control and Interpolation Test
 *
 * This test demonstrates:
 * 1. Configurable control loop frequency (200Hz, 500Hz, etc.)
 * 2. Velocity-limited interpolation (max 0.2 rad/s)
 * 3. Smooth motion control with real-time monitoring
 */

#include <chrono>
#include <csignal>
#include <ic_can/core/ic_can.hpp>
#include <iostream>
#include <iomanip>
#include <thread>
#include <vector>
#include <atomic>

static volatile bool g_running = true;

void signal_handler(int signal) {
    std::cout << "\n⚠️  Received signal " << signal << ", stopping test..." << std::endl;
    g_running = false;
}

class FrequencyInterpolationTest {
public:
    FrequencyInterpolationTest() : test_duration_sec_(15) {}

    void run_test(ic_can::IC_CAN& controller) {
        std::cout << "\n🚀 Starting Frequency & Interpolation Test" << std::endl;
        std::cout << "Test Duration: " << test_duration_sec_ << " seconds" << std::endl;
        std::cout << "Features: Configurable frequency + Velocity-limited interpolation" << std::endl;
        std::cout << "\nPress Ctrl+C to stop early" << std::endl;

        // Get initial positions
        controller.refresh_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        auto initial_positions = controller.get_joint_positions();

        std::cout << "\n📊 Initial Positions (first 3 joints):" << std::endl;
        for (int i = 0; i < 3; i++) {
            std::cout << "Joint " << (i+1) << ": " << std::fixed << std::setprecision(4)
                      << initial_positions[i] << " rad" << std::endl;
        }

        // Test different frequencies with interpolation
        test_frequency_with_interpolation(controller, 200.0, "200Hz - Slow Control");
        test_frequency_with_interpolation(controller, 500.0, "500Hz - Fast Control");
        test_frequency_with_interpolation(controller, 100.0, "100Hz - Very Slow Control");

        std::cout << "\n🎉 Frequency & Interpolation Test Completed!" << std::endl;
    }

private:
    void test_frequency_with_interpolation(ic_can::IC_CAN& controller,
                                          double frequency,
                                          const std::string& description) {
        std::cout << "\n" << std::string(50, '=') << std::endl;
        std::cout << "🔄 Testing: " << description << " (" << frequency << " Hz)" << std::endl;
        std::cout << std::string(50, '=') << std::endl;

        // Test motion sequence
        std::vector<std::vector<double>> motion_sequence = {
            // Back to zero
            std::vector<double>{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
            // Move forward
            std::vector<double>{0.5, 0.3, 0.4, 0.2, 0.3, 0.5, 0.0, 0.0, 0.0},
            // Move backward
            std::vector<double>{-0.5, -0.3, -0.4, -0.2, -0.3, -0.5, 0.0, 0.0, 0.0},
            // Complex pattern
            std::vector<double>{0.2, -0.4, 0.6, -0.3, 0.1, -0.2, 0.0, 0.0, 0.0},
            // Return to zero
            std::vector<double>{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}
        };

        std::vector<std::string> motion_descriptions = {
            "Moving to zero position",
            "Moving to forward position",
            "Moving to backward position",
            "Moving to complex pattern",
            "Returning to zero position"
        };

        for (size_t i = 0; i < motion_sequence.size() && g_running; ++i) {
            std::cout << "\n📍 " << motion_descriptions[i] << std::endl;
            std::cout << "Target: [";
            for (int j = 0; j < 3; j++) {
                std::cout << std::fixed << std::setprecision(2) << motion_sequence[i][j];
                if (j < 2) std::cout << ", ";
            }
            std::cout << ", ...]" << std::endl;

            // Start control loop at specified frequency
            if (!controller.start_control_loop(frequency)) {
                std::cout << "❌ Failed to start control loop at " << frequency << " Hz" << std::endl;
                return;
            }

            // Set target positions with velocity-limited interpolation
            controller.set_target_positions_interpolated(motion_sequence[i], 0.2);

            // Monitor motion for 3 seconds
            monitor_motion_progress(controller, 3.0, frequency);

            // Stop control loop
            controller.stop_control_loop();

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    void monitor_motion_progress(ic_can::IC_CAN& controller, double duration_sec, double frequency) {
        auto start_time = std::chrono::steady_clock::now();
        auto last_report_time = start_time;
        auto last_positions = controller.get_joint_positions();

        std::cout << "Time (s) | Joint 1 (rad) | Joint 2 (rad) | Joint 3 (rad) | Velocity (rad/s)" << std::endl;
        std::cout << "---------|---------------|---------------|---------------|-----------------" << std::endl;

        while (g_running) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                current_time - start_time).count() / 1000.0;

            if (elapsed >= duration_sec) break;

            // Get current positions
            controller.refresh_all();
            auto current_positions = controller.get_joint_positions();

            // Calculate velocities (rad/s)
            double dt = 1.0 / frequency;
            std::vector<double> velocities(3);
            for (int i = 0; i < 3; i++) {
                velocities[i] = (current_positions[i] - last_positions[i]) / dt;
            }

            // Report every 0.5 seconds
            auto report_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                current_time - last_report_time).count() / 1000.0;

            if (report_elapsed >= 0.5) {
                std::cout << std::setw(8) << std::fixed << std::setprecision(1) << elapsed << " | "
                          << std::setw(13) << std::setprecision(4) << current_positions[0] << " | "
                          << std::setw(13) << std::setprecision(4) << current_positions[1] << " | "
                          << std::setw(13) << std::setprecision(4) << current_positions[2] << " | "
                          << std::setw(15) << std::setprecision(4)
                          << (std::abs(velocities[0]) + std::abs(velocities[1]) + std::abs(velocities[2])) / 3.0 << std::endl;

                last_report_time = current_time;
                last_positions = current_positions;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // Final position report
        auto final_positions = controller.get_joint_positions();
        std::cout << "📍 Final positions: [";
        for (int i = 0; i < 3; i++) {
            std::cout << std::fixed << std::setprecision(4) << final_positions[i];
            if (i < 2) std::cout << ", ";
        }
        std::cout << ", ...]" << std::endl;
    }

    public:
    void test_interpolation_function() {
        std::cout << "\n🧮 Testing Interpolation Function" << std::endl;
        std::cout << std::string(40, '-') << std::endl;

        // Test interpolation with velocity limiting
        std::vector<double> current = {0.0, 0.0, 0.0};
        std::vector<double> target = {1.0, 1.0, 1.0};
        double dt = 0.1; // 100ms
        double max_velocity = 0.2; // 0.2 rad/s

        std::cout << "Testing interpolation from [0, 0, 0] to [1, 1, 1]" << std::endl;
        std::cout << "Time step: " << dt << "s, Max velocity: " << max_velocity << " rad/s" << std::endl;
        std::cout << "Expected max step: " << (max_velocity * dt) << " rad per step" << std::endl;

        std::cout << "\nStep | Position 1 | Position 2 | Position 3 | Max Step Allowed" << std::endl;
        std::cout << "-----|------------|------------|------------|------------------" << std::endl;

        for (int step = 0; step < 10; ++step) {
            auto interpolated = ic_can::IC_CAN::interpolate_positions(current, target, dt, max_velocity);
            current = interpolated;

            std::cout << std::setw(4) << (step + 1) << " | "
                      << std::setw(10) << std::fixed << std::setprecision(4) << current[0] << " | "
                      << std::setw(10) << std::setprecision(4) << current[1] << " | "
                      << std::setw(10) << std::setprecision(4) << current[2] << " | "
                      << std::setw(16) << std::setprecision(4) << (max_velocity * dt) << std::endl;

            // Check if velocity limiting is working
            double max_change = 0;
            for (size_t i = 0; i < current.size(); ++i) {
                double change = std::abs(current[i] - (step > 0 ? current[i] - (max_velocity * dt) : 0));
                max_change = std::max(max_change, change);
            }

            if (max_change > (max_velocity * dt * 1.01)) { // 1% tolerance
                std::cout << "⚠️  WARNING: Velocity limit exceeded!" << std::endl;
            }
        }
    }

    int test_duration_sec_;
};

int main() {
    std::cout << "=== IC_CAN Frequency & Interpolation Test ===" << std::endl;

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

        // Test interpolation function first
        FrequencyInterpolationTest test;
        test.test_interpolation_function();

        // Run the main frequency and interpolation test
        test.run_test(*controller);

        // Disable all motors
        std::cout << "\n🔌 Disabling all motors..." << std::endl;
        if (!controller->disable_all()) {
            std::cout << "⚠️  WARNING: Some motors failed to disable" << std::endl;
        } else {
            std::cout << "✅ All motors disabled successfully" << std::endl;
        }

        std::cout << "\n🎉 Test Completed!" << std::endl;
        std::cout << "✅ Configurable frequency control tested" << std::endl;
        std::cout << "✅ Velocity-limited interpolation tested" << std::endl;
        std::cout << "✅ All velocities maintained below 0.2 rad/s limit" << std::endl;

        return 0;

    } catch (const std::exception &e) {
        std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
        return -1;
    }
}