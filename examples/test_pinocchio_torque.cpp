/**
 * @file test_pinocchio_torque.cpp
 * @brief Test Pinocchio-based Torque Prediction
 *
 * Simple test to verify Pinocchio torque predictor implementation
 * with the IC arm URDF.
 *
 * @author IC_CAN Team
 * @date 2025-10-27
 */

#include <iostream>
#include <iomanip>
#include "ic_can/core/torque_predictor_unified.h"

int main() {
    std::cout << "🔧 Testing Pinocchio-based Torque Prediction System..." << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    try {
        // Create the unified torque predictor
        ic_can::TorquePredictorUnified predictor;

        // Check if initialized
        if (!predictor.is_initialized()) {
            std::cout << "❌ Failed to initialize torque predictor!" << std::endl;
            return 1;
        }

        // Print method status
        predictor.print_method_status();

        // Test joint configuration (zero position)
        double q[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};     // joint positions
        double dq[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};    // joint velocities
        double ddq[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};   // joint accelerations

        std::cout << "\n🦾 Test Configuration:" << std::endl;
        std::cout << "Joint positions: [";
        for (int i = 0; i < 6; ++i) {
            std::cout << q[i] << (i < 5 ? ", " : "");
        }
        std::cout << "]" << std::endl;
        std::cout << "Joint velocities: [";
        for (int i = 0; i < 6; ++i) {
            std::cout << dq[i] << (i < 5 ? ", " : "");
        }
        std::cout << "]" << std::endl;
        std::cout << "Joint accelerations: [";
        for (int i = 0; i < 6; ++i) {
            std::cout << ddq[i] << (i < 5 ? ", " : "");
        }
        std::cout << "]" << std::endl;

        // Test gravity torque only (should be non-zero at zero position due to gravity)
        double gravity_torque[6];
        if (predictor.predict_gravity_torque(q, gravity_torque)) {
            std::cout << "\n🌍 Gravity Torques:" << std::endl;
            for (int i = 0; i < 6; ++i) {
                std::cout << "  Joint " << (i+1) << ": " << std::fixed << std::setw(8) << std::setprecision(4) << gravity_torque[i] << " Nm" << std::endl;
            }
        } else {
            std::cout << "❌ Failed to compute gravity torques!" << std::endl;
            return 1;
        }

        // Test full torque breakdown
        double M_torque[6], C_torque[6], G_torque[6], total_torque[6];
        if (predictor.predict_torques(q, dq, ddq, M_torque, C_torque, G_torque, total_torque)) {
            std::cout << "\n⚙️  Full Torque Breakdown:" << std::endl;
            std::cout << std::setw(8) << "Joint" << " | "
                      << std::setw(10) << "M*ddq" << " | "
                      << std::setw(10) << "C*dq" << " | "
                      << std::setw(10) << "G" << " | "
                      << std::setw(10) << "Total" << std::endl;
            std::cout << std::string(55, '-') << std::endl;

            for (int i = 0; i < 6; ++i) {
                std::cout << std::setw(8) << ("j" + std::to_string(i + 1)) << " | "
                          << std::setw(10) << std::fixed << std::setprecision(3) << M_torque[i] << " | "
                          << std::setw(10) << C_torque[i] << " | "
                          << std::setw(10) << G_torque[i] << " | "
                          << std::setw(10) << total_torque[i] << std::endl;
            }
        } else {
            std::cout << "❌ Failed to compute full torque breakdown!" << std::endl;
            return 1;
        }

        // Test with non-zero velocities
        double dq_nonzero[6] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6};
        std::cout << "\n🚀 Testing with non-zero velocities:" << std::endl;
        predictor.print_torque_breakdown(q, dq_nonzero, ddq);

        std::cout << "\n✅ Pinocchio torque prediction test completed successfully!" << std::endl;
        std::cout << std::string(60, '=') << std::endl;

        return 0;

    } catch (const std::exception& e) {
        std::cout << "❌ Exception occurred: " << e.what() << std::endl;
        return 1;
    }
}