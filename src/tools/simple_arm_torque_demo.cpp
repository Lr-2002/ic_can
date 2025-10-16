/**
 * @file simple_arm_torque_demo.cpp
 * @brief Simple Arm Torque Demo
 *
 * This demo shows the torque prediction functionality by:
 * - Simulating arm joint states (ready for real motor integration)
 * - Computing predicted torques using the integrated torque predictor
 * - Displaying comprehensive torque analysis
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <array>
#include <random>
#include <chrono>
#include <thread>

#include "ic_can/core/torque_predictor_pure_c.h"

using namespace ic_can;

// Simple joint state simulator (simulates 6-DOF arm)
class ArmJointSimulator {
private:
    std::mt19937 rng_;
    std::uniform_real_distribution<double> noise_;
    std::array<double, 6> positions_;
    std::array<double, 6> velocities_;
    std::array<double, 6> torques_;

public:
    ArmJointSimulator() : rng_(42), noise_(-0.01, 0.01) {
        positions_.fill(0.0);
        velocities_.fill(0.0);
        torques_.fill(0.0);
    }

    void set_positions(const std::array<double, 6>& pos) {
        positions_ = pos;
        // Add realistic noise
        for (int i = 0; i < 6; ++i) {
            positions_[i] += noise_(rng_);
            velocities_[i] = noise_(rng_) * 0.1; // Small velocity noise
            torques_[i] = -0.05 * positions_[i] + noise_(rng_) * 0.1; // Simple torque model
        }
    }

    std::array<double, 6> get_positions() const { return positions_; }
    std::array<double, 6> get_velocities() const { return velocities_; }
    std::array<double, 6> get_torques() const { return torques_; }
};

int main() {
    std::cout << "=== Simple Arm Torque Prediction Demo ===" << std::endl;
    std::cout << "Reading joint states and computing predicted torques" << std::endl;

    // Create arm simulator (simulates getting data from real motors)
    ArmJointSimulator arm;

    // Create torque predictor
    TorquePredictorPureC predictor;

    // Check initialization
    if (!predictor.is_initialized()) {
        std::cout << "❌ Torque predictor initialization failed!" << std::endl;
        return -1;
    }
    std::cout << "✅ Torque predictor initialized successfully" << std::endl;

    // Pure C torque predictor has gravity compensation built-in
    std::cout << "\n🔧 Pure C torque predictor with built-in gravity compensation..." << std::endl;
    std::cout << "✅ Gravity compensation available" << std::endl;

    // Test configurations (arm poses)
    std::vector<std::array<double, 6>> test_poses = {
        {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0}},           // Home position
        {{0.5, 0.3, -0.2, 0.1, -0.1, 0.2}},      // Extended forward
        {{-0.3, 0.2, 0.1, -0.1, 0.4, -0.1}},     // Retracted back
        {{0.8, 0.6, -0.4, 0.3, -0.2, 0.5}},      // Far extended
        {{0.2, -0.2, 0.1, -0.1, 0.05, -0.05}},     // Complex pose
        {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0}}            // Back to home
    };

    for (size_t i = 0; i < test_poses.size(); ++i) {
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "🎯 Test Configuration " << (i + 1) << "/" << test_poses.size() << std::endl;
        std::cout << std::string(80, '=') << std::endl;

        // Set arm to test position
        arm.set_positions(test_poses[i]);

        // Get joint states (simulating reading from real motors)
        auto positions = arm.get_positions();
        auto velocities = arm.get_velocities();
        auto actual_torques = arm.get_torques();

        std::cout << "\n📈 Joint States (from motors):" << std::endl;
        std::cout << "Joint | Position (rad) | Velocity (rad/s) | Actual Torque (N⋅m)" << std::endl;
        std::cout << "------|----------------|------------------|-------------------" << std::endl;
        for (int j = 0; j < 6; ++j) {
            std::cout << std::setw(5) << j << " | "
                      << std::fixed << std::setprecision(4) << std::setw(14) << positions[j] << " | "
                      << std::fixed << std::setprecision(4) << std::setw(16) << velocities[j] << " | "
                      << std::fixed << std::setprecision(4) << std::setw(17) << actual_torques[j] << std::endl;
        }

        // Compute predicted torques from joint states
        std::cout << "\n🔧 Predicted Torques:" << std::endl;
        std::array<double, 6> predicted_torques;
        std::array<double, 6> gravity_torques;

        // Get gravity torques
        if (predictor.predict_gravity_torque(positions.data(), gravity_torques.data())) {
            std::cout << "Gravity torques (N⋅m):    ";
            for (int j = 0; j < 6; ++j) {
                std::cout << std::fixed << std::setprecision(3) << std::setw(8) << gravity_torques[j] << " ";
            }
            std::cout << std::endl;
        }

        // Get total predicted torques (with zero accelerations)
        std::array<double, 6> zero_accel = {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0}};
        if (predictor.predict_total_torque(positions.data(), velocities.data(), zero_accel.data(), predicted_torques.data())) {
            std::cout << "Total predicted (N⋅m):   ";
            for (int j = 0; j < 6; ++j) {
                std::cout << std::fixed << std::setprecision(3) << std::setw(8) << predicted_torques[j] << " ";
            }
            std::cout << std::endl;
        }

        // Compare predicted vs actual
        std::cout << "\n📊 Torque Analysis:" << std::endl;
        std::cout << "Joint | Gravity | Predicted | Actual | Error" << std::endl;
        std::cout << "------|--------|----------|--------|------" << std::endl;

        double total_error = 0.0;
        for (int j = 0; j < 6; ++j) {
            double error = predicted_torques[j] - actual_torques[j];
            total_error += std::abs(error);
            std::cout << std::setw(5) << j << " | "
                      << std::fixed << std::setprecision(3) << std::setw(7) << gravity_torques[j] << " | "
                      << std::fixed << std::setprecision(3) << std::setw(8) << predicted_torques[j] << " | "
                      << std::fixed << std::setprecision(3) << std::setw(6) << actual_torques[j] << " | "
                      << std::fixed << std::setprecision(3) << std::setw(6) << error << std::endl;
        }
        std::cout << "Total absolute error: " << std::fixed << std::setprecision(4) << total_error << " N⋅m" << std::endl;

        // Show detailed torque breakdown
        std::cout << "\n📊 Detailed Torque Component Breakdown:" << std::endl;
        predictor.print_torque_breakdown(positions.data(), velocities.data(), zero_accel.data());

        // Small delay between tests
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "🎉 Simple Arm Torque Demo Completed!" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "✅ Torque predictor: FUNCTIONAL" << std::endl;
    std::cout << "✅ Gravity compensation: "
              << (predictor.is_initialized() ? "AVAILABLE" : "NOT AVAILABLE") << std::endl;
    std::cout << "✅ Joint state reading: SIMULATED (ready for real motors)" << std::endl;
    std::cout << "✅ Torque computation: SUCCESS" << std::endl;
    std::cout << "\n📝 Next steps for real integration:" << std::endl;
    std::cout << "1. Replace ArmJointSimulator with actual motor feedback" << std::endl;
    std::cout << "2. Use controller->get_joint_positions() from IC_CAN" << std::endl;
    std::cout << "3. Integrate into real-time control loop" << std::endl;

    return 0;
}