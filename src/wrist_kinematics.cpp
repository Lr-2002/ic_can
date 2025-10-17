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

#include <cmath>
#include <stdexcept>
#include <iostream>
#include <vector>

namespace ic_can {

/**
 * @brief Wrist kinematics for cross-wrist mechanism
 *
 * This class implements the kinematics for the cross-wrist with 3-umberal gear group.
 * The relationship between motor angles and output angles is:
 * - α = (θ₁ + θ₂) / 2  (pitch motion)
 * - β = (θ₁ - θ₂) / 2  (roll motion)
 *
 * And the inverse kinematics:
 * - θ₁ = α + β
 * - θ₂ = α - β
 */
class WristKinematics {
public:
    /**
     * @brief Forward kinematics: convert motor angles to output angles
     *
     * @param theta1 Motor 1 angle (θ₁) in radians
     * @param theta2 Motor 2 angle (θ₂) in radians
     * @return std::pair<double, double> Output angles (α, β) in radians
     */
    static std::pair<double, double> forwardKinematics(double theta1, double theta2) {
        double alpha = (theta1 + theta2) / 2.0;  // α = (θ₁ + θ₂) / 2
        double beta = (theta1 - theta2) / 2.0;   // β = (θ₁ - θ₂) / 2

        return {alpha, beta};
    }

    /**
     * @brief Inverse kinematics: convert desired output angles to motor angles
     *
     * @param alpha Desired pitch angle (α) in radians
     * @param beta Desired roll angle (β) in radians
     * @return std::pair<double, double> Motor angles (θ₁, θ₂) in radians
     */
    static std::pair<double, double> inverseKinematics(double alpha, double beta) {
        double theta1 = alpha + beta;  // θ₁ = α + β
        double theta2 = alpha - beta;  // θ₂ = α - β

        return {theta1, theta2};
    }

    /**
     * @brief Forward kinematics for vector inputs
     *
     * @param motor_angles Vector of motor angles [θ₁, θ₂] in radians
     * @return std::vector<double> Output angles [α, β] in radians
     */
    static std::vector<double> forwardKinematicsVector(const std::vector<double>& motor_angles) {
        if (motor_angles.size() != 2) {
            throw std::invalid_argument("Motor angles vector must have exactly 2 elements [θ₁, θ₂]");
        }

        auto [alpha, beta] = forwardKinematics(motor_angles[0], motor_angles[1]);
        return {alpha, beta};
    }

    /**
     * @brief Inverse kinematics for vector inputs
     *
     * @param output_angles Vector of desired output angles [α, β] in radians
     * @return std::vector<double> Motor angles [θ₁, θ₂] in radians
     */
    static std::vector<double> inverseKinematicsVector(const std::vector<double>& output_angles) {
        if (output_angles.size() != 2) {
            throw std::invalid_argument("Output angles vector must have exactly 2 elements [α, β]");
        }

        auto [theta1, theta2] = inverseKinematics(output_angles[0], output_angles[1]);
        return {theta1, theta2};
    }

    /**
     * @brief Convert degrees to radians
     *
     * @param degrees Angle in degrees
     * @return double Angle in radians
     */
    static double degToRad(double degrees) {
        return degrees * M_PI / 180.0;
    }

    /**
     * @brief Convert radians to degrees
     *
     * @param radians Angle in radians
     * @return double Angle in degrees
     */
    static double radToDeg(double radians) {
        return radians * 180.0 / M_PI;
    }

    /**
     * @brief Normalize angle to [-π, π]
     *
     * @param angle Input angle in radians
     * @return double Normalized angle in radians
     */
    static double normalizeAngle(double angle) {
        while (angle > M_PI) {
            angle -= 2 * M_PI;
        }
        while (angle < -M_PI) {
            angle += 2 * M_PI;
        }
        return angle;
    }

    /**
     * @brief Validate motor angles are within limits
     *
     * @param theta1 Motor 1 angle (θ₁) in radians
     * @param theta2 Motor 2 angle (θ₂) in radians
     * @param min_angle Minimum allowed angle in radians (default: -π)
     * @param max_angle Maximum allowed angle in radians (default: π)
     * @return true if angles are within limits
     */
    static bool validateMotorAngles(double theta1, double theta2,
                                   double min_angle = -M_PI,
                                   double max_angle = M_PI) {
        return (theta1 >= min_angle && theta1 <= max_angle &&
                theta2 >= min_angle && theta2 <= max_angle);
    }

    /**
     * @brief Validate output angles are within limits
     *
     * @param alpha Pitch angle (α) in radians
     * @param beta Roll angle (β) in radians
     * @param min_alpha Minimum allowed pitch angle in radians (default: -π/2)
     * @param max_alpha Maximum allowed pitch angle in radians (default: π/2)
     * @param min_beta Minimum allowed roll angle in radians (default: -π/2)
     * @param max_beta Maximum allowed roll angle in radians (default: π/2)
     * @return true if angles are within limits
     */
    static bool validateOutputAngles(double alpha, double beta,
                                   double min_alpha = -M_PI/2,
                                   double max_alpha = M_PI/2,
                                   double min_beta = -M_PI/2,
                                   double max_beta = M_PI/2) {
        return (alpha >= min_alpha && alpha <= max_alpha &&
                beta >= min_beta && beta <= max_beta);
    }

    /**
     * @brief Print current wrist state
     *
     * @param theta1 Motor 1 angle (θ₁) in radians
     * @param theta2 Motor 2 angle (θ₂) in radians
     */
    static void printWristState(double theta1, double theta2) {
        auto [alpha, beta] = forwardKinematics(theta1, theta2);

        std::cout << "Wrist State:" << std::endl;
        std::cout << "  Motor Angles: θ₁ = " << radToDeg(theta1) << "°, θ₂ = " << radToDeg(theta2) << "°" << std::endl;
        std::cout << "  Output Angles: α = " << radToDeg(alpha) << "°, β = " << radToDeg(beta) << "°" << std::endl;
        std::cout << "  α (pitch): " << alpha << " rad (" << radToDeg(alpha) << "°)" << std::endl;
        std::cout << "  β (roll): " << beta << " rad (" << radToDeg(beta) << "°)" << std::endl;
    }
};

// Example usage and test functions
namespace wrist_tests {

void testForwardKinematics() {
    std::cout << "\n=== Testing Forward Kinematics ===" << std::endl;

    // Test cases: (θ₁, θ₂) -> (α, β)
    std::vector<std::pair<std::pair<double, double>, std::pair<double, double>>> test_cases = {
        {{0.0, 0.0}, {0.0, 0.0}},        // Zero position
        {{M_PI/2, M_PI/2}, {M_PI/2, 0.0}}, // Both motors forward
        {{M_PI/2, -M_PI/2}, {0.0, M_PI/2}}, // Motors opposite
        {{0.5, 0.3}, {0.4, 0.1}},         // Arbitrary values
        {{-0.5, -0.3}, {-0.4, -0.1}}      // Negative values
    };

    for (const auto& test_case : test_cases) {
        auto [theta1, theta2] = test_case.first;
        auto [expected_alpha, expected_beta] = test_case.second;

        auto [alpha, beta] = WristKinematics::forwardKinematics(theta1, theta2);

        std::cout << "Input: θ₁=" << WristKinematics::radToDeg(theta1) << "°, θ₂=" << WristKinematics::radToDeg(theta2) << "°" << std::endl;
        std::cout << "Output: α=" << WristKinematics::radToDeg(alpha) << "°, β=" << WristKinematics::radToDeg(beta) << "°" << std::endl;
        std::cout << "Expected: α=" << WristKinematics::radToDeg(expected_alpha) << "°, β=" << WristKinematics::radToDeg(expected_beta) << "°" << std::endl;
        std::cout << "---" << std::endl;
    }
}

void testInverseKinematics() {
    std::cout << "\n=== Testing Inverse Kinematics ===" << std::endl;

    // Test cases: (α, β) -> (θ₁, θ₂)
    std::vector<std::pair<std::pair<double, double>, std::pair<double, double>>> test_cases = {
        {{0.0, 0.0}, {0.0, 0.0}},        // Zero position
        {{M_PI/4, M_PI/4}, {M_PI/2, 0.0}}, // Forward and roll
        {{M_PI/4, -M_PI/4}, {0.0, M_PI/2}}, // Forward and negative roll
        {{0.3, 0.2}, {0.5, 0.1}},         // Arbitrary values
        {{-0.3, -0.2}, {-0.5, -0.1}}      // Negative values
    };

    for (const auto& test_case : test_cases) {
        auto [alpha, beta] = test_case.first;
        auto [expected_theta1, expected_theta2] = test_case.second;

        auto [theta1, theta2] = WristKinematics::inverseKinematics(alpha, beta);

        std::cout << "Input: α=" << WristKinematics::radToDeg(alpha) << "°, β=" << WristKinematics::radToDeg(beta) << "°" << std::endl;
        std::cout << "Output: θ₁=" << WristKinematics::radToDeg(theta1) << "°, θ₂=" << WristKinematics::radToDeg(theta2) << "°" << std::endl;
        std::cout << "Expected: θ₁=" << WristKinematics::radToDeg(expected_theta1) << "°, θ₂=" << WristKinematics::radToDeg(expected_theta2) << "°" << std::endl;
        std::cout << "---" << std::endl;
    }
}

void testRoundTrip() {
    std::cout << "\n=== Testing Round-trip Consistency ===" << std::endl;

    std::vector<std::pair<double, double>> motor_angles = {
        {0.0, 0.0},
        {M_PI/4, M_PI/6},
        {-M_PI/3, M_PI/5},
        {0.7, -0.4},
        {-0.2, -0.8}
    };

    for (const auto& [theta1, theta2] : motor_angles) {
        // Forward then inverse kinematics
        auto [alpha, beta] = WristKinematics::forwardKinematics(theta1, theta2);
        auto [theta1_back, theta2_back] = WristKinematics::inverseKinematics(alpha, beta);

        double error1 = std::abs(theta1 - theta1_back);
        double error2 = std::abs(theta2 - theta2_back);

        std::cout << "Original: θ₁=" << WristKinematics::radToDeg(theta1) << "°, θ₂=" << WristKinematics::radToDeg(theta2) << "°" << std::endl;
        std::cout << "Round-trip: θ₁=" << WristKinematics::radToDeg(theta1_back) << "°, θ₂=" << WristKinematics::radToDeg(theta2_back) << "°" << std::endl;
        std::cout << "Errors: θ₁=" << error1 << " rad, θ₂=" << error2 << " rad" << std::endl;
        std::cout << "---" << std::endl;
    }
}

void runAllTests() {
    std::cout << "Running Wrist Kinematics Tests..." << std::endl;
    testForwardKinematics();
    testInverseKinematics();
    testRoundTrip();
    std::cout << "\nAll tests completed!" << std::endl;
}

} // namespace wrist_tests

} // namespace ic_can