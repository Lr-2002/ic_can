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

#include <iostream>
#include <iomanip>

// Include the wrist kinematics
#include "../src/wrist_kinematics.cpp"

int main() {
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "========================================" << std::endl;
    std::cout << "   WRIST KINEMATICS TEST SUITE" << std::endl;
    std::cout << "========================================" << std::endl;

    // Run all tests
    ic_can::wrist_tests::runAllTests();

    // Additional practical usage examples
    std::cout << "\n========================================" << std::endl;
    std::cout << "   PRACTICAL USAGE EXAMPLES" << std::endl;
    std::cout << "========================================" << std::endl;

    std::cout << "\nExample 1: Zero Position" << std::endl;
    ic_can::WristKinematics::printWristState(0.0, 0.0);

    std::cout << "\nExample 2: Pitch Forward Only" << std::endl;
    double theta1 = ic_can::WristKinematics::degToRad(30.0);
    double theta2 = ic_can::WristKinematics::degToRad(30.0);
    ic_can::WristKinematics::printWristState(theta1, theta2);

    std::cout << "\nExample 3: Roll Right Only" << std::endl;
    theta1 = ic_can::WristKinematics::degToRad(20.0);
    theta2 = ic_can::WristKinematics::degToRad(-20.0);
    ic_can::WristKinematics::printWristState(theta1, theta2);

    std::cout << "\nExample 4: Combined Pitch and Roll" << std::endl;
    double alpha = ic_can::WristKinematics::degToRad(25.0);  // Desired pitch
    double beta = ic_can::WristKinematics::degToRad(15.0);   // Desired roll
    auto [motor1, motor2] = ic_can::WristKinematics::inverseKinematics(alpha, beta);
    std::cout << "Desired: α = " << ic_can::WristKinematics::radToDeg(alpha) << "°, β = " << ic_can::WristKinematics::radToDeg(beta) << "°" << std::endl;
    std::cout << "Required motor angles:" << std::endl;
    ic_can::WristKinematics::printWristState(motor1, motor2);

    std::cout << "\n========================================" << std::endl;
    std::cout << "All tests completed successfully!" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}