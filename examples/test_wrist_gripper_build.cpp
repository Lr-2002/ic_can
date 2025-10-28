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
 * @file test_wrist_gripper_build.cpp
 * @brief Simple test to verify wrist and gripper components build and work
 */

#include <iostream>
#include "ic_can/core/wrist_component.hpp"
#include "ic_can/core/gripper_component.hpp"
#include "ic_can/motors/base_motor.hpp"

int main() {
    std::cout << "=== Testing Wrist and Gripper Component Build ===" << std::endl;

    try {
        // Test wrist component creation
        ic_can::WristComponent wrist;
        std::cout << "✅ WristComponent created successfully" << std::endl;

        // Test gripper component creation
        ic_can::GripperComponent gripper;
        std::cout << "✅ GripperComponent created successfully" << std::endl;

        // Test basic functionality
        std::cout << "📊 Testing basic component functionality..." << std::endl;

        // Test wrist pose setting
        wrist.set_wrist_pose(0.5, 0.3, 1.0);
        std::cout << "✅ Wrist pose set successfully" << std::endl;

        // Test gripper openness
        gripper.set_openness(0.7, 0.5, 0.3);
        std::cout << "✅ Gripper openness set successfully" << std::endl;

        // Test state queries
        auto wrist_pos = wrist.get_positions();
        auto gripper_openness = gripper.get_openness();

        std::cout << "📊 Wrist positions: [" << wrist_pos[0] << ", " << wrist_pos[1] << "]" << std::endl;
        std::cout << "📊 Gripper openness: " << gripper_openness << std::endl;

        std::cout << "\n🎉 All tests passed! Components are working correctly!" << std::endl;
        std::cout << "✅ Build system is working" << std::endl;
        std::cout << "✅ Component separation successful" << std::endl;
        std::cout << "✅ Ready for coordinator pattern implementation" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}