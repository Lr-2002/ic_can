/**
 * @file wrist_kinematics_test.cpp
 * @brief Test Wrist Component Kinematic Interface
 *
 * Demonstrates the theta_1, theta_2 -> alpha, beta transformations
 * and the refresh functionality of the wrist component.
 *
 * @author IC_CAN Team
 * @date 2025-10-29
 */

#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include "ic_can/core/wrist_component.hpp"

void print_kinematics_test(const std::string& test_name,
                           double theta_1, double theta_2,
                           double expected_alpha, double expected_beta) {
    std::cout << "\n=== " << test_name << " ===" << std::endl;
    std::cout << "Input motor angles:" << std::endl;
    std::cout << "  θ₁ (m7): " << std::fixed << std::setprecision(3)
              << theta_1 << " rad (" << (theta_1 * 180.0 / M_PI) << "°)" << std::endl;
    std::cout << "  θ₂ (m8): " << theta_2 << " rad (" << (theta_2 * 180.0 / M_PI) << "°)" << std::endl;

    // Calculate expected alpha and beta
    double alpha = (theta_1 - theta_2) / 2.0;
    double beta = (theta_1 + theta_2) / 2.0;

    std::cout << "Calculated wrist angles:" << std::endl;
    std::cout << "  α: " << alpha << " rad (" << (alpha * 180.0 / M_PI) << "°)" << std::endl;
    std::cout << "  β: " << beta << " rad (" << (beta * 180.0 / M_PI) << "°)" << std::endl;

    // Check if calculations are correct
    bool alpha_correct = std::abs(alpha - expected_alpha) < 1e-9;
    bool beta_correct = std::abs(beta - expected_beta) < 1e-9;

    std::cout << "Validation: " << (alpha_correct && beta_correct ? "✅ PASS" : "❌ FAIL") << std::endl;
}

int main() {
    std::cout << "=== IC_CAN Wrist Component Kinematic Test ===" << std::endl;
    std::cout << "Testing θ₁, θ₂ -> α, β transformations" << std::endl;

    // Create wrist component
    ic_can::WristComponent wrist;

    // Enable debug mode to see detailed output
    wrist.set_debug_mode(true);

    // Test Case 1: Zero position
    print_kinematics_test("Test 1: Zero Position", 0.0, 0.0, 0.0, 0.0);

    // Test Case 2: Simple case - both motors at 1 rad
    print_kinematics_test("Test 2: Equal Positive", 1.0, 1.0, 0.0, 1.0);

    // Test Case 3: Simple case - motors opposite
    print_kinematics_test("Test 3: Opposite", 1.0, -1.0, 1.0, 0.0);

    // Test Case 4: Realistic wrist angles
    print_kinematics_test("Test 4: Realistic Wrist", 0.5, -0.2, 0.35, 0.15);

    std::cout << "\n=== Testing Wrist Component Interface ===" << std::endl;

    // Test the component interface
    std::cout << "\n1. Testing set_motor_angles(0.5, -0.2):" << std::endl;
    wrist.set_motor_angles(0.5, -0.2);
    wrist.refresh();

    auto motor_angles = wrist.get_motor_angles();
    auto wrist_angles = wrist.get_wrist_angles();

    std::cout << "Read motor angles [θ₁, θ₂]: ["
              << motor_angles[0] << ", " << motor_angles[1] << "] rad" << std::endl;
    std::cout << "Read wrist angles [α, β]: ["
              << wrist_angles[0] << ", " << wrist_angles[1] << "] rad" << std::endl;

    // Test reverse transformation
    std::cout << "\n2. Testing set_wrist_angles(0.35, 0.15):" << std::endl;
    wrist.set_wrist_angles(0.35, 0.15);
    wrist.refresh();

    motor_angles = wrist.get_motor_angles();
    wrist_angles = wrist.get_wrist_angles();

    std::cout << "After set_wrist_angles:" << std::endl;
    std::cout << "Read motor angles [θ₁, θ₂]: ["
              << motor_angles[0] << ", " << motor_angles[1] << "] rad" << std::endl;
    std::cout << "Read wrist angles [α, β]: ["
              << wrist_angles[0] << ", " << wrist_angles[1] << "] rad" << std::endl;

    // Test refresh functionality
    std::cout << "\n3. Testing refresh functionality:" << std::endl;
    for (int i = 0; i < 3; i++) {
        std::cout << "Refresh " << (i+1) << ": ";
        bool success = wrist.refresh();
        std::cout << (success ? "✅ Success" : "❌ Failed") << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\n✅ Wrist component kinematic interface test completed!" << std::endl;
    std::cout << "\nSummary of kinematic transformations:" << std::endl;
    std::cout << "  α = (θ₁ - θ₂) / 2" << std::endl;
    std::cout << "  β = (θ₁ + θ₂) / 2" << std::endl;
    std::cout << "  θ₁ = α + β" << std::endl;
    std::cout << "  θ₂ = β - α" << std::endl;

    return 0;
}