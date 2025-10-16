/**
 * @file demo_real_arm_torques.cpp
 * @brief Full IC_CAN Real Arm Torque Demo
 *
 * Simple demo that:
 * - Uses full IC_CAN system to connect to real arm
 * - Reads real joint positions and velocities from all 9 motors
 * - Computes and prints predicted torques using the integrated torque predictor
 * - Shows real-time torque analysis without execution
 */

#include <chrono>
#include <ic_can/core/ic_can.hpp>
#include <iostream>
#include <iomanip>
#include <thread>

int main() {
    std::cout << "=== Full IC_CAN Real Arm Torque Demo ===" << std::endl;
    std::cout << "Connecting to real arm, reading positions, computing torques" << std::endl;

    try {
        // Create IC_CAN controller with debug enabled
        auto controller = std::make_unique<ic_can::IC_CAN>(
            "F561E08C892274DB09496BCC1102DBC5", true);

        // Initialize system
        std::cout << "\n🔧 Initializing IC_CAN system..." << std::endl;
        if (!controller->initialize()) {
            std::cout << "❌ FAILED: System initialization failed" << std::endl;
            return -1;
        }
        std::cout << "✅ IC_CAN system initialized successfully" << std::endl;

        // Enable motors
        std::cout << "\n🔌 Enabling all motors..." << std::endl;
        if (!controller->enable_all()) {
            std::cout << "❌ Failed: could not enable motors" << std::endl;
            return -1;
        }
        std::cout << "✅ All 9 motors enabled" << std::endl;

        // Print system info to check torque predictor
        std::cout << "\n📋 System Information:" << std::endl;
        controller->print_system_info();

        // Enable torque prediction
        std::cout << "\n🔧 Enabling torque prediction..." << std::endl;
        if (!controller->enable_gravity_compensation()) {
            std::cout << "⚠️  Gravity compensation not available" << std::endl;
        } else {
            std::cout << "✅ Gravity compensation enabled" << std::endl;
        }

        // Test positions for the arm
        std::vector<std::vector<double>> test_positions = {
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},      // Home position
            {0.3, 0.2, -0.15, 0.1, -0.1, 0.2, 0.0, 0.0, 0.0},   // Extended position
            {-0.2, 0.15, 0.1, -0.05, 0.05, -0.1, 0.0, 0.0, 0.0}, // Retracted position
            {0.5, 0.3, -0.25, 0.2, -0.15, 0.3, 0.0, 0.0, 0.0}, // Far extended
            {0.1, -0.1, 0.05, -0.05, 0.02, -0.02, 0.0, 0.0, 0.0}, // Small movement
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}       // Back to home
        };

        for (size_t i = 0; i < test_positions.size(); ++i) {
            std::cout << "\n" << std::string(80, '=') << std::endl;
            std::cout << "🎯 Test " << (i + 1) << "/" << test_positions.size() << ": Moving Arm" << std::endl;
            std::cout << std::string(80, '=') << std::endl;

            // Show target positions
            std::cout << "Target positions (rad): ";
            for (int j = 0; j < 9; ++j) {
                std::cout << std::fixed << std::setprecision(3) << std::setw(7) << test_positions[i][j] << " ";
            }
            std::cout << std::endl;

            // Move arm to target position
            controller->set_target_positions_interpolated(test_positions[i], 0.3);

            // Run control loop for 3 seconds to reach position
            std::cout << "🤖 Moving arm to target position..." << std::endl;
            controller->start_control_loop(500); // 500Hz
            std::this_thread::sleep_for(std::chrono::seconds(3));

            // Stop control and get stable readings
            controller->stop_control_loop();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // Get REAL joint states from the actual arm
            std::cout << "\n📈 Real Joint States from ACTUAL ARM:" << std::endl;
            auto positions = controller->get_joint_positions();
            auto velocities = controller->get_joint_velocities();
            auto actual_torques = controller->get_joint_torques();

            std::cout << "Motor | Position (rad) | Velocity (rad/s) | Actual Torque (N⋅m)" << std::endl;
            std::cout << "------|----------------|------------------|-------------------" << std::endl;
            for (int j = 0; j < 9; ++j) {
                std::cout << std::setw(5) << (j+1) << " | "
                          << std::fixed << std::setprecision(4) << std::setw(14) << positions[j] << " | "
                          << std::fixed << std::setprecision(4) << std::setw(16) << velocities[j] << " | "
                          << std::fixed << std::setprecision(4) << std::setw(17) << actual_torques[j] << std::endl;
            }

            // Compute predicted torques using REAL joint states
            std::cout << "\n🔧 PREDICTED TORQUES from REAL Joint States:" << std::endl;
            auto predicted_torques = controller->get_all_predicted_torques();
            auto gravity_torques = controller->get_gravity_compensation_torques();

            std::cout << "Motor | Gravity (N⋅m) | Predicted (N⋅m) | Actual (N⋅m) | Error (N⋅m)" << std::endl;
            std::cout << "------|--------------|---------------|-------------|------------" << std::endl;

            double total_error = 0.0;
            for (int j = 0; j < 9; ++j) {
                double error = predicted_torques[j] - actual_torques[j];
                total_error += std::abs(error);
                std::cout << std::setw(5) << (j+1) << " | "
                          << std::fixed << std::setprecision(3) << std::setw(12) << gravity_torques[j] << " | "
                          << std::fixed << std::setprecision(3) << std::setw(13) << predicted_torques[j] << " | "
                          << std::fixed << std::setprecision(3) << std::setw(11) << actual_torques[j] << " | "
                          << std::fixed << std::setprecision(3) << std::setw(10) << error << std::endl;
            }
            std::cout << "--------------------------------------------------------------------------------" << std::endl;
            std::cout << "Total Absolute Error: " << std::fixed << std::setprecision(4) << total_error << " N⋅m" << std::endl;

            // Show detailed torque breakdown for first 6 arm joints
            std::cout << "\n📊 Detailed Torque Breakdown (First 6 Arm Joints):" << std::endl;
            controller->print_torque_breakdown();

            // Wait before next movement
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Final summary
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "🎉 Real Arm Torque Demo Completed!" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        std::cout << "✅ Connected to REAL IC_ARM system" << std::endl;
        std::cout << "✅ Read REAL joint states from 9 motors" << std::endl;
        std::cout << "✅ Computed REAL predicted torques" << std::endl;
        std::cout << "✅ Gravity compensation: "
                  << (controller->is_gravity_compensation_enabled() ? "ENABLED" : "DISABLED") << std::endl;
        std::cout << "✅ Torque prediction: COMPUTATION-ONLY (no execution)" << std::endl;

        // Shutdown system
        std::cout << "\n🔄 Shutting down IC_CAN system..." << std::endl;
        controller->shutdown();

        return 0;

    } catch (const std::exception &e) {
        std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
        return -1;
    }
}