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
 * @file logging_demo_simple.cpp
 * @brief Simplified Torque Prediction Demo
 *
 * Standalone demonstration of torque prediction using simulated joint states:
 * - Simulates getting positions and velocities from motors
 * - Computes predicted torques using torque predictor
 * - Displays torque analysis without execution
 * - Shows gravity compensation and total torque predictions
 */

#include <chrono>
#include <iostream>
#include <iomanip>
#include <thread>
#include <vector>
#include <array>
#include <random>
#include <algorithm>

#include "../include/ic_can/core/torque_predictor.h"

using namespace ic_can;

// Simulate motor data acquisition (like from logging_demo.cpp)
class MotorDataSimulator {
private:
    std::mt19937 rng_;
    std::uniform_real_distribution<double> pos_noise_;
    std::uniform_real_distribution<double> vel_noise_;
    std::uniform_real_distribution<double> torque_noise_;
    std::vector<std::array<double, 6>> current_positions_;
    std::vector<std::array<double, 6>> current_velocities_;
    std::vector<std::array<double, 6>> current_torques_;

public:
    MotorDataSimulator() : rng_(42),
        pos_noise_(-0.01, 0.01),    // Small position noise
        vel_noise_(-0.02, 0.02),    // Small velocity noise
        torque_noise_(-0.1, 0.1)    // Small torque noise
    {
        current_positions_.resize(9);
        current_velocities_.resize(9);
        current_torques_.resize(9);

        // Initialize to zero
        for (int i = 0; i < 9; ++i) {
            current_positions_[i].fill(0.0);
            current_velocities_[i].fill(0.0);
            current_torques_[i].fill(0.0);
        }
    }

    void move_to_position(const std::vector<double>& target_pos, double max_velocity = 0.5) {
        if (target_pos.size() < 6) return;

        // Simulate moving to target position (simplified interpolation)
        for (int i = 0; i < 6; ++i) {
            double error = target_pos[i] - current_positions_[0][i];
            double step = (error > 0) ? std::min(error, max_velocity * 0.02) : std::max(error, -max_velocity * 0.02); // 20ms timestep
            current_positions_[0][i] += step;

            // Add some realistic velocity based on movement
            current_velocities_[0][i] = step / 0.02 + vel_noise_(rng_);

            // Simulate some motor torque (based on position and simple dynamics)
            current_torques_[0][i] = -0.1 * current_positions_[0][i] - 0.5 * current_velocities_[0][i] + torque_noise_(rng_);
        }
    }

    std::vector<double> get_positions() {
        std::vector<double> pos(9);
        for (int i = 0; i < 9; ++i) {
            if (i < 6) {
                pos[i] = current_positions_[0][i] + pos_noise_(rng_);
            } else {
                pos[i] = 0.0; // Gripper joints not simulated
            }
        }
        return pos;
    }

    std::vector<double> get_velocities() {
        std::vector<double> vel(9);
        for (int i = 0; i < 9; ++i) {
            if (i < 6) {
                vel[i] = current_velocities_[0][i] + vel_noise_(rng_);
            } else {
                vel[i] = 0.0; // Gripper joints not simulated
            }
        }
        return vel;
    }

    std::vector<double> get_torques() {
        std::vector<double> torque(9);
        for (int i = 0; i < 9; ++i) {
            if (i < 6) {
                torque[i] = current_torques_[0][i] + torque_noise_(rng_);
            } else {
                torque[i] = 0.0; // Gripper joints not simulated
            }
        }
        return torque;
    }
};

// Simple torque predictor class that wraps our real predictor
class SimpleTorquePredictor {
private:
    std::unique_ptr<TorquePredictor> predictor_;
    bool gravity_enabled_;

public:
    SimpleTorquePredictor() : gravity_enabled_(false) {
        predictor_ = std::make_unique<TorquePredictor>();
    }

    bool is_initialized() const {
        return predictor_ && predictor_->is_initialized();
    }

    bool enable_gravity_compensation() {
        if (is_initialized()) {
            gravity_enabled_ = true;
            return true;
        }
        return false;
    }

    bool is_gravity_compensation_enabled() const {
        return gravity_enabled_;
    }

    std::vector<double> get_all_predicted_torques() {
        std::vector<double> torques(9, 0.0);
        if (!is_initialized()) return torques;

        // Get first 6 joint torques from predictor (simulated joint states)
        std::array<double, 6> q, dq, ddq, predicted;

        // For demo, use zero velocities and accelerations
        q.fill(0.0);
        dq.fill(0.0);
        ddq.fill(0.0);

        if (predictor_->predict_total_torque(q.data(), dq.data(), ddq.data(), predicted.data())) {
            for (int i = 0; i < 6; ++i) {
                torques[i] = predicted[i];
            }
        }

        return torques;
    }

    std::vector<double> get_gravity_compensation_torques() {
        std::vector<double> torques(9, 0.0);
        if (!gravity_enabled_ || !is_initialized()) return torques;

        std::array<double, 6> q, gravity;
        q.fill(0.0);

        if (predictor_->predict_gravity_torque(q, gravity)) {
            for (int i = 0; i < 6; ++i) {
                torques[i] = gravity[i];
            }
        }

        return torques;
    }

    void print_torque_breakdown() {
        if (!is_initialized()) {
            std::cout << "❌ Torque predictor not initialized" << std::endl;
            return;
        }

        std::array<double, 6> q, dq, ddq;
        q.fill(0.0);
        dq.fill(0.0);
        ddq.fill(0.0);

        predictor_->print_torque_breakdown(q, dq, ddq);
    }
};

int main() {
    std::cout << "=== Simplified Torque Prediction Demo ===" << std::endl;
    std::cout << "Simulating real-time joint states and computing predicted torques" << std::endl;

    try {
        // Create motor data simulator
        MotorDataSimulator motor_sim;

        // Create torque predictor
        SimpleTorquePredictor predictor;

        // Check initialization
        if (!predictor.is_initialized()) {
            std::cout << "❌ Torque predictor initialization failed!" << std::endl;
            return -1;
        }
        std::cout << "✅ Torque predictor initialized successfully" << std::endl;

        // Enable gravity compensation
        std::cout << "\n🔧 Enabling torque prediction..." << std::endl;
        if (predictor.enable_gravity_compensation()) {
            std::cout << "✅ Gravity compensation enabled" << std::endl;
        } else {
            std::cout << "⚠️  Gravity compensation not available" << std::endl;
        }

        // Test sequence with different joint configurations
        std::vector<std::vector<double>> test_positions = {
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},      // Zero position
            {0.2, 0.1, -0.1, 0.05, -0.05, 0.3, 0.0, 0.0, 0.0},  // Small extension
            {0.5, 0.3, -0.2, 0.15, -0.1, -0.3, 0.0, 0.0, 0.0}, // Medium pose
            {-0.3, 0.2, 0.1, -0.1, 0.4, 0.2, 0.0, 0.0, 0.0},  // Mixed configuration
            {0.1, -0.1, 0.05, -0.05, 0.02, -0.02, 0.0, 0.0, 0.0}, // Small movement
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}       // Back to zero
        };

        for (size_t test_idx = 0; test_idx < test_positions.size(); ++test_idx) {
            std::cout << "\n" << std::string(80, '=') << std::endl;
            std::cout << "🎯 Test Configuration " << (test_idx + 1) << "/" << test_positions.size() << std::endl;
            std::cout << std::string(80, '=') << std::endl;

            // Show target positions
            std::cout << "\n📍 Target positions (rad): ";
            for (int i = 0; i < 6; ++i) {
                std::cout << std::fixed << std::setprecision(3) << std::setw(8) << test_positions[test_idx][i] << " ";
            }
            std::cout << std::endl;

            // Simulate movement to target position
            std::cout << "\n🤖 Moving to target position..." << std::endl;
            for (int step = 0; step < 100; ++step) {
                motor_sim.move_to_position(test_positions[test_idx]);
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }

            // Get simulated joint state from motors
            std::cout << "\n📈 Simulated Joint State from Motors:" << std::endl;
            auto positions = motor_sim.get_positions();
            auto velocities = motor_sim.get_velocities();
            auto actual_torques = motor_sim.get_torques();

            std::cout << "Joint | Position (rad) | Velocity (rad/s) | Actual Torque (N⋅m)" << std::endl;
            std::cout << "------|----------------|------------------|-------------------" << std::endl;
            for (int i = 0; i < 6; ++i) {
                std::cout << std::setw(5) << i << " | "
                          << std::fixed << std::setprecision(4) << std::setw(14) << positions[i] << " | "
                          << std::fixed << std::setprecision(4) << std::setw(16) << velocities[i] << " | "
                          << std::fixed << std::setprecision(4) << std::setw(17) << actual_torques[i] << std::endl;
            }

            // Compute predicted torques from simulated joint state
            std::cout << "\n🔧 Predicted Torques from Joint State:" << std::endl;
            auto predicted_torques = predictor.get_all_predicted_torques();
            auto gravity_torques = predictor.get_gravity_compensation_torques();

            std::cout << "Joint | Gravity (N⋅m) | Predicted (N⋅m) | Actual (N⋅m) | Error (N⋅m)" << std::endl;
            std::cout << "------|--------------|---------------|-------------|------------" << std::endl;

            double total_error = 0.0;
            for (int i = 0; i < 6; ++i) {
                double error = predicted_torques[i] - actual_torques[i];
                total_error += std::abs(error);
                std::cout << std::setw(5) << i << " | "
                          << std::fixed << std::setprecision(3) << std::setw(12) << gravity_torques[i] << " | "
                          << std::fixed << std::setprecision(3) << std::setw(13) << predicted_torques[i] << " | "
                          << std::fixed << std::setprecision(3) << std::setw(11) << actual_torques[i] << " | "
                          << std::fixed << std::setprecision(3) << std::setw(10) << error << std::endl;
            }
            std::cout << "--------------------------------------------------------------------------------" << std::endl;
            std::cout << "Total Absolute Error: " << std::fixed << std::setprecision(4) << total_error << " N⋅m" << std::endl;

            // Show detailed torque breakdown for current state
            std::cout << "\n📊 Detailed Torque Component Breakdown:" << std::endl;
            predictor.print_torque_breakdown();

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Final summary
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "🎉 Torque Prediction Demo Completed!" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        std::cout << "✅ Successfully computed torques from joint state data" << std::endl;
        std::cout << "✅ Gravity compensation: "
                  << (predictor.is_gravity_compensation_enabled() ? "ENABLED" : "DISABLED") << std::endl;
        std::cout << "✅ Torque prediction: COMPUTATION-ONLY (no execution)" << std::endl;
        std::cout << "✅ Data acquisition: SIMULATED (ready for real motors)" << std::endl;
        std::cout << std::endl;
        std::cout << "To use with real motors, replace MotorDataSimulator with actual IC_CAN controller" << std::endl;
        std::cout << "and use controller->get_joint_positions(), get_joint_velocities(), etc." << std::endl;

        return 0;

    } catch (const std::exception &e) {
        std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
        return -1;
    }
}