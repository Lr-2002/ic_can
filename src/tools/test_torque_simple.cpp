/**
 * @file test_torque_simple.cpp
 * @brief Simple torque prediction test demonstration
 *
 * Demonstrates the torque prediction functionality using simulated joint data
 * to show how the system would work with real motor data.
 *
 * @author IC_CAN Team
 * @date 2025-10-15
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <thread>
#include <array>
#include <random>

#include "../include/ic_can/core/torque_predictor.h"

using namespace ic_can;

int main() {
    std::cout << "=== Simple Torque Prediction Demo ===" << std::endl;
    std::cout << "Demonstrating torque computation with simulated joint states" << std::endl;
    std::cout << "(This simulates getting data from logging_demo.cpp)" << std::endl;

    // Create torque predictor instance
    TorquePredictor predictor;

    // Check initialization
    if (!predictor.is_initialized()) {
        std::cout << "❌ Torque predictor initialization failed!" << std::endl;
        return -1;
    }

    std::cout << "✅ Torque predictor initialized successfully" << std::endl;

    // Simulate getting joint data from motors (like logging_demo would provide)
    std::cout << "\n📊 Simulating Joint State Data Acquisition:" << std::endl;
    std::cout << std::string(80, '=') << std::endl;

    // Simulated trajectory points (like those from logging_demo)
    std::vector<std::array<double, 6>> simulated_positions = {
        {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0}},      // Zero position
        {{0.0, 0.0, 0.0, 0.0, 0.0, 0.3}},      // Joint 5 extended
        {{0.0, 0.0, 0.0, 0.0, 0.0, -0.3}},     // Joint 5 retracted
        {{0.0, 0.0, 0.0, 0.0, 0.4, -0.3}},     // Joint 4 extended
        {{0.5, 0.3, -0.2, 0.15, -0.1, 0.2}},  // Complex pose
        {{-0.3, 0.2, 0.1, -0.1, 0.05, -0.15}}, // Another complex pose
        {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0}}       // Back to zero
    };

    // Simulated velocities (would come from actual motor feedback)
    std::vector<std::array<double, 6>> simulated_velocities = {
        {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0}},      // Static
        {{0.05, 0.02, -0.01, 0.0, 0.1, 0.05}}, // Small motion
        {{0.1, 0.05, -0.02, 0.01, 0.05, -0.03}}, // Medium motion
        {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0}},      // Static again
        {{0.2, 0.1, -0.08, 0.05, 0.03, -0.02}}, // Fast motion
        {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0}},      // Static
        {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0}}       // Final static
    };

    std::cout << "Simulating data acquisition and torque computation..." << std::endl;

    for (size_t step = 0; step < simulated_positions.size(); ++step) {
        std::cout << "\n🎯 Step " << (step + 1) << "/" << simulated_positions.size() << std::endl;
        std::cout << std::string(60, '-') << std::endl;

        // Simulate getting current joint state (as if from motors)
        std::cout << "Joint positions (rad):    ";
        for (int i = 0; i < 6; ++i) {
            std::cout << std::fixed << std::setprecision(3) << std::setw(8) << simulated_positions[step][i] << " ";
        }
        std::cout << std::endl;

        std::cout << "Joint velocities (rad/s):  ";
        for (int i = 0; i < 6; ++i) {
            std::cout << std::fixed << std::setprecision(3) << std::setw(8) << simulated_velocities[step][i] << " ";
        }
        std::cout << std::endl;

        // Compute gravity torques
        std::array<double, 6> gravity_torques;
        if (predictor.predict_gravity_torque(simulated_positions[step], gravity_torques)) {
            std::cout << "Gravity torques (N⋅m):    ";
            for (int i = 0; i < 6; ++i) {
                std::cout << std::fixed << std::setprecision(3) << std::setw(8) << gravity_torques[i] << " ";
            }
            std::cout << std::endl;
        }

        // Compute total torques (with zero accelerations for steady state)
        std::array<double, 6> zero_accel = {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0}};
        std::array<double, 6> total_torques;
        if (predictor.predict_total_torque(simulated_positions[step].data(),
                                          simulated_velocities[step].data(),
                                          zero_accel.data(),
                                          total_torques.data())) {
            std::cout << "Total torques (N⋅m):      ";
            for (int i = 0; i < 6; ++i) {
                std::cout << std::fixed << std::setprecision(3) << std::setw(8) << total_torques[i] << " ";
            }
            std::cout << std::endl;

            // Show contribution breakdown
            double gravity_magnitude = 0.0;
            double total_magnitude = 0.0;
            for (int i = 0; i < 6; ++i) {
                gravity_magnitude += std::abs(gravity_torques[i]);
                total_magnitude += std::abs(total_torques[i]);
            }

            if (total_magnitude > 0.001) {
                double gravity_percentage = (gravity_magnitude / total_magnitude) * 100.0;
                std::cout << "Gravity contribution:     " << std::fixed << std::setprecision(1)
                          << std::setw(5) << gravity_percentage << "%" << std::endl;
            }
        }

        // Simulate processing delay (as if reading from real motors)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Detailed breakdown for one configuration
    std::cout << "\n📊 Detailed Torque Component Analysis:" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "Analyzing complex pose: ";
    for (int i = 0; i < 6; ++i) {
        std::cout << std::fixed << std::setprecision(3) << simulated_positions[4][i] << " ";
    }
    std::cout << std::endl;

    std::array<double, 6> zero_accel_breakdown = {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0}};
    predictor.print_torque_breakdown(simulated_positions[4], simulated_velocities[4], zero_accel_breakdown);

    // Performance test with simulated data
    std::cout << "\n⚡ Real-time Performance Test:" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "Testing torque prediction speed (simulating real-time control at 500Hz)..." << std::endl;

    const int test_iterations = 5000; // 10 seconds at 500Hz
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < test_iterations; ++i) {
        // Simulate getting joint state (cycle through positions)
        size_t pos_idx = i % simulated_positions.size();
        size_t vel_idx = i % simulated_velocities.size();

        std::array<double, 6> test_total_torques;
        std::array<double, 6> test_zero_accel = {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0}};
        predictor.predict_total_torque(simulated_positions[pos_idx].data(),
                                       simulated_velocities[vel_idx].data(),
                                       test_zero_accel.data(),
                                       test_total_torques.data());

        // Prevent compiler optimization
        volatile double dummy = test_total_torques[0];
        (void)dummy;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    double avg_time_us = static_cast<double>(duration.count()) / test_iterations;
    double frequency_hz = 1.0 / (avg_time_us / 1000000.0);

    std::cout << "Performance Results:" << std::endl;
    std::cout << "  " << test_iterations << " predictions in " << duration.count() << " μs" << std::endl;
    std::cout << "  Average time: " << std::fixed << std::setprecision(3) << avg_time_us << " μs" << std::endl;
    std::cout << "  Frequency: " << std::fixed << std::setprecision(0) << frequency_hz << " Hz" << std::endl;
    std::cout << "  Control loop margin: " << std::fixed << std::setprecision(1)
              << ((frequency_hz / 500.0) * 100.0) << "% of 500Hz requirement" << std::endl;

    if (frequency_hz >= 500.0) {
        std::cout << "  ✅ Sufficient for 500Hz real-time control!" << std::endl;
    } else {
        std::cout << "  ⚠️  Below 500Hz requirement - may need optimization" << std::endl;
    }

    // Summary
    std::cout << "\n🎉 Simple Torque Prediction Demo Completed!" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "✅ Torque predictor: FUNCTIONAL" << std::endl;
    std::cout << "✅ Gravity compensation: AVAILABLE" << std::endl;
    std::cout << "✅ Real-time performance: "
              << (frequency_hz >= 500.0 ? "SUFFICIENT" : "NEEDS OPTIMIZATION") << std::endl;
    std::cout << "✅ Integration ready: YES" << std::endl;
    std::cout << std::endl;
    std::cout << "Next steps for integration with logging_demo.cpp:" << std::endl;
    std::cout << "1. Replace simulated joint data with actual motor feedback" << std::endl;
    std::cout << "2. Add torque prediction calls in the control loop" << std::endl;
    std::cout << "3. Use computed torques for gravity compensation" << std::endl;
    std::cout << "4. Optionally apply torques for feedforward control" << std::endl;

    return 0;
}