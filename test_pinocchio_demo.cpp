/**
 * @file test_pinocchio_demo.cpp
 * @brief Simple Pinocchio Torque Prediction Demo
 *
 * Demonstrates the new Pinocchio-based torque prediction integrated into IC_CAN.
 *
 * @author IC_CAN Team
 * @date 2025-10-27
 */

#include "ic_can/core/ic_can.hpp"
#include <iostream>
#include <iomanip>

int main() {
    std::cout << "🔧 Pinocchio Torque Prediction Demo" << std::endl;
    std::cout << std::string(50, '=') << std::endl;

    try {
        // Create IC_CAN instance (this will initialize Pinocchio predictor by default)
        ic_can::IC_CAN can_controller("demo_device", true);

        // Show system information
        can_controller.print_system_info();

        // Test gravity compensation (this will use Pinocchio predictor)
        std::cout << "\n🌍 Enabling gravity compensation..." << std::endl;
        if (!can_controller.enable_gravity_compensation()) {
            std::cout << "❌ Failed to enable gravity compensation" << std::endl;
            return 1;
        }

        // Get gravity torques using Pinocchio
        auto gravity_torques = can_controller.get_gravity_compensation_torques();

        std::cout << "\n🌍 Pinocchio Gravity Torques (N⋅m):" << std::endl;
        if (gravity_torques.size() >= 6) {
            for (int i = 0; i < 6; ++i) {
                std::cout << "  Joint " << (i+1) << ": " << std::fixed << std::setprecision(4) << gravity_torques[i] << std::endl;
            }
        } else {
            std::cout << "  Gravity torques not available" << std::endl;
        }

        std::cout << "\n✅ Pinocchio integration demo completed successfully!" << std::endl;
        std::cout << std::string(50, '=') << std::endl;

        return 0;

    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }
}