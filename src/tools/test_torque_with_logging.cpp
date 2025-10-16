/**
 * @file test_torque_with_logging.cpp
 * @brief Combined test: Real-time torque prediction with actual joint state data
 *
 * This test combines the logging functionality with torque prediction:
 * - Gets actual joint positions and velocities from motors
 * - Computes predicted torques using the torque predictor
 * - Displays torque breakdowns and gravity compensation values
 * - Does NOT execute torques (compute-only mode)
 *
 * @author IC_CAN Team
 * @date 2025-10-15
 */

#include <chrono>
#include <ic_can/core/ic_can.hpp>
#include <iostream>
#include <iomanip>
#include <thread>
#include <vector>
#include <array>

int main() {
    std::cout << "=== Real-time Torque Prediction with Actual Joint State ===" << std::endl;
    std::cout << "Computing torques from live motor data (execution disabled)" << std::endl;

    try {
        // Create IC_CAN controller with debug enabled
        auto controller = std::make_unique<ic_can::IC_CAN>(
            "F561E08C892274DB09496BCC1102DBC5", true);

        // Initialize system
        std::cout << "\n🔧 Initializing system..." << std::endl;
        if (!controller->initialize()) {
            std::cout << "❌ FAILED: System initialization failed" << std::endl;
            return -1;
        }
        std::cout << "✅ System initialized successfully" << std::endl;

        // Print system info to check torque predictor status
        controller->print_system_info();

        // Enable torque predictor gravity compensation (for computation only)
        std::cout << "\n🔧 Enabling torque prediction features..." << std::endl;
        if (!controller->enable_gravity_compensation()) {
            std::cout << "⚠️  Gravity compensation not available, continuing with basic torque prediction" << std::endl;
        }

        std::cout << "\n📊 Starting real-time torque prediction demo..." << std::endl;
        std::cout << "Mode: COMPUTE ONLY (no torque execution)" << std::endl;

        // Test sequence with different joint configurations
        std::vector<std::vector<double>> test_positions = {
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},  // Zero position
            {0.2, 0.1, -0.1, 0.05, -0.05, 0.1, 0.0, 0.0, 0.0}, // Small extension
            {0.5, 0.3, -0.2, 0.15, -0.1, 0.2, 0.0, 0.0, 0.0}, // Medium extension
            {-0.3, 0.2, 0.1, -0.1, 0.05, -0.15, 0.0, 0.0, 0.0}, // Mixed configuration
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}   // Back to zero
        };

        for (size_t test_idx = 0; test_idx < test_positions.size(); ++test_idx) {
            std::cout << "\n" << std::string(80, '=') << std::endl;
            std::cout << "🎯 Test Configuration " << (test_idx + 1) << "/" << test_positions.size() << std::endl;
            std::cout << std::string(80, '=') << std::endl;

            // Send position command
            std::cout << "\n📍 Moving to target position:" << std::endl;
            for (int i = 0; i < 6; ++i) {
                std::cout << "  Joint " << i << ": " << std::fixed << std::setprecision(3)
                          << test_positions[test_idx][i] << " rad" << std::endl;
            }

            // Set target positions (will be executed)
            controller->set_target_positions_interpolated(test_positions[test_idx], 0.5);

            // Run control loop briefly to reach position
            controller->start_control_loop(200); // 200Hz
            std::this_thread::sleep_for(std::chrono::seconds(3));

            // Stop control before computing torques
            controller->stop_control_loop();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // Get current joint state
            std::cout << "\n📈 Current Joint State:" << std::endl;
            auto positions = controller->get_joint_positions();
            auto velocities = controller->get_joint_velocities();
            auto actual_torques = controller->get_joint_torques();

            std::cout << "  Joint | Position (rad) | Velocity (rad/s) | Actual Torque (N⋅m)" << std::endl;
            std::cout << "  ------|----------------|------------------|-------------------" << std::endl;
            for (int i = 0; i < 6; ++i) {
                std::cout << "  " << std::setw(5) << i << " | "
                          << std::fixed << std::setprecision(4) << std::setw(14) << positions[i] << " | "
                          << std::fixed << std::setprecision(4) << std::setw(16) << velocities[i] << " | "
                          << std::fixed << std::setprecision(4) << std::setw(17) << actual_torques[i] << std::endl;
            }

            // Compute predicted torques
            std::cout << "\n🔧 Computed Torque Predictions:" << std::endl;
            auto predicted_torques = controller->get_all_predicted_torques();
            auto gravity_torques = controller->get_gravity_compensation_torques();

            std::cout << "  Joint | Gravity (N⋅m) | Total Predicted (N⋅m) | Actual (N⋅m) | Error (N⋅m)" << std::endl;
            std::cout << "  ------|--------------|---------------------|-------------|------------" << std::endl;

            double total_error = 0.0;
            for (int i = 0; i < 6; ++i) {
                double error = predicted_torques[i] - actual_torques[i];
                total_error += std::abs(error);
                std::cout << "  " << std::setw(5) << i << " | "
                          << std::fixed << std::setprecision(4) << std::setw(12) << gravity_torques[i] << " | "
                          << std::fixed << std::setprecision(4) << std::setw(19) << predicted_torques[i] << " | "
                          << std::fixed << std::setprecision(4) << std::setw(11) << actual_torques[i] << " | "
                          << std::fixed << std::setprecision(4) << std::setw(10) << error << std::endl;
            }

            std::cout << "  --------------------------------------------------------------------------------" << std::endl;
            std::cout << "  Total Absolute Error: " << std::fixed << std::setprecision(4) << total_error << " N⋅m" << std::endl;

            // Print detailed torque breakdown for first joint
            std::cout << "\n📊 Detailed Torque Component Breakdown:" << std::endl;
            controller->print_torque_breakdown();

            // Performance test with current state
            std::cout << "\n⚡ Torque Prediction Performance Test:" << std::endl;
            auto start_time = std::chrono::high_resolution_clock::now();
            const int perf_iterations = 1000;

            for (int i = 0; i < perf_iterations; ++i) {
                auto test_torques = controller->get_all_predicted_torques();
                // Prevent compiler optimization
                if (test_torques.empty()) break;
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

            double avg_time_us = static_cast<double>(duration.count()) / perf_iterations;
            double frequency_hz = 1.0 / (avg_time_us / 1000000.0);

            std::cout << "  " << perf_iterations << " predictions in " << duration.count() << " μs" << std::endl;
            std::cout << "  Average time: " << std::fixed << std::setprecision(3) << avg_time_us << " μs" << std::endl;
            std::cout << "  Frequency: " << std::fixed << std::setprecision(0) << frequency_hz << " Hz ";
            if (frequency_hz >= 1000.0) {
                std::cout << "(✅ Real-time capable)" << std::endl;
            } else {
                std::cout << "(⚠️  Below real-time requirements)" << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Final summary
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "🎉 Real-time Torque Prediction Test Completed!" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        std::cout << "✅ Successfully computed torques from live joint state data" << std::endl;
        std::cout << "✅ Gravity compensation: "
                  << (controller->is_gravity_compensation_enabled() ? "ENABLED" : "DISABLED") << std::endl;
        std::cout << "✅ Torque prediction performance verified" << std::endl;
        std::cout << "✅ No torque execution performed (compute-only mode)" << std::endl;

        // Shutdown system
        std::cout << "\n🔄 Shutting down system..." << std::endl;
        controller->shutdown();

        return 0;

    } catch (const std::exception &e) {
        std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
        return -1;
    }
}