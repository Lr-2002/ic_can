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
 * @file wrist_kinematics_test.cpp
 * @brief Test and demonstration of wrist kinematics functions
 */

#include <ic_can/core/ic_can.hpp>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main() {
    std::cout << "=== Wrist Kinematics Test ===" << std::endl;
    std::cout << std::fixed << std::setprecision(4);

    // Create IC_CAN controller (using debug mode)
    ic_can::IC_CAN controller("dummy_device", true);

    // Test cases for forward kinematics
    std::cout << "\n🔧 Testing Forward Kinematics (theta1, theta2 -> alpha, beta):" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    std::vector<std::pair<double, double>> test_cases = {
        {0.0, 0.0},        // Both motors at 0
        {0.5, -0.5},       // Motor 7 forward, Motor 8 backward
        {1.0, 1.0},        // Both motors forward
        {-0.5, 0.5},       // Motor 7 backward, Motor 8 forward
        {1.57, 0.0},       // Motor 7 at 90°, Motor 8 at 0°
        {0.0, -1.57}       // Motor 7 at 0°, Motor 8 at -90°
    };

    for (const auto& test_case : test_cases) {
        double theta1 = test_case.first;
        double theta2 = test_case.second;

        std::cout << "\nTest Case: θ₁=" << theta1 << " rad (" << (theta1 * 180.0 / M_PI) << "°), "
                  << "θ₂=" << theta2 << " rad (" << (theta2 * 180.0 / M_PI) << "°)" << std::endl;

        auto result = controller.wrist_forward_kinematics(theta1, theta2);
        double alpha = result[0];
        double beta = result[1];

        std::cout << "  → α=" << alpha << " rad (" << (alpha * 180.0 / M_PI) << "°), "
                  << "β=" << beta << " rad (" << (beta * 180.0 / M_PI) << "°)" << std::endl;

        // Test inverse kinematics (should return original values)
        auto inverse_result = controller.wrist_inverse_kinematics(alpha, beta);
        double theta1_back = inverse_result[0];
        double theta2_back = inverse_result[1];

        std::cout << "  → Inverse: θ₁=" << theta1_back << " rad, θ₂=" << theta2_back << " rad";

        // Check if inverse kinematics returns original values (within tolerance)
        double tolerance = 1e-10;
        if (std::abs(theta1_back - theta1) < tolerance && std::abs(theta2_back - theta2) < tolerance) {
            std::cout << " ✅" << std::endl;
        } else {
            std::cout << " ❌ (mismatch!)" << std::endl;
        }
    }

    // Test specific alpha/beta values
    std::cout << "\n\n🔧 Testing Inverse Kinematics (alpha, beta -> theta1, theta2):" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    std::vector<std::pair<double, double>> alpha_beta_cases = {
        {0.0, 0.0},        // Zero angles
        {0.5, 0.0},        // Alpha only
        {0.0, 0.5},        // Beta only
        {0.5, 0.5},        // Both positive
        {-0.5, -0.5},      // Both negative
        {1.0, -0.5}        // Mixed
    };

    for (const auto& test_case : alpha_beta_cases) {
        double alpha = test_case.first;
        double beta = test_case.second;

        std::cout << "\nTest Case: α=" << alpha << " rad (" << (alpha * 180.0 / M_PI) << "°), "
                  << "β=" << beta << " rad (" << (beta * 180.0 / M_PI) << "°)" << std::endl;

        auto result = controller.wrist_inverse_kinematics(alpha, beta);
        double theta1 = result[0];
        double theta2 = result[1];

        std::cout << "  → θ₁=" << theta1 << " rad (" << (theta1 * 180.0 / M_PI) << "°), "
                  << "θ₂=" << theta2 << " rad (" << (theta2 * 180.0 / M_PI) << "°)" << std::endl;

        // Verify forward kinematics
        auto forward_result = controller.wrist_forward_kinematics(theta1, theta2);
        double alpha_back = forward_result[0];
        double beta_back = forward_result[1];

        std::cout << "  → Forward: α=" << alpha_back << " rad, β=" << beta_back << " rad";

        double tolerance = 1e-10;
        if (std::abs(alpha_back - alpha) < tolerance && std::abs(beta_back - beta) < tolerance) {
            std::cout << " ✅" << std::endl;
        } else {
            std::cout << " ❌ (mismatch!)" << std::endl;
        }
    }

    std::cout << "\n\n✅ Wrist Kinematics Test Complete!" << std::endl;
    std::cout << "\n📋 Summary:" << std::endl;
    std::cout << "   Forward Kinematics: α = (θ₁ + θ₂) / 2, β = (θ₁ - θ₂) / 2" << std::endl;
    std::cout << "   Inverse Kinematics: θ₁ = α + β, θ₂ = α - β" << std::endl;
    std::cout << "   Where θ₁ = Motor 7 angle, θ₂ = Motor 8 angle" << std::endl;

    return 0;
}