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
 * @file wrist_gripper_demo.cpp
 * @brief Demo showing separated wrist and gripper component usage
 *
 * This example demonstrates how to use the newly separated WristComponent
 * (HT motors m7-m8) and GripperComponent (servo motor m9) independently.
 */

#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <cmath>
#include "ic_can/core/ic_can.hpp"
#include "ic_can/core/wrist_component.hpp"
#include "ic_can/core/gripper_component.hpp"

int main() {
    std::cout << "=== IC_CAN Wrist and Gripper Component Demo ===" << std::endl;
    std::cout << "This demo shows the separated wrist (m7-m8) and gripper (m9) control" << std::endl;
    std::cout << std::endl;

    try {
        // Initialize IC_CAN system
        ic_can::IC_CAN controller("IC_ARM_001", true); // Enable debug output

        std::cout << "✅ IC_CAN system initialized" << std::endl;
        std::cout << std::endl;

        // Get component references
        auto& wrist = controller.get_wrist();
        auto& gripper = controller.get_gripper();

        std::cout << "🔧 Components initialized:" << std::endl;
        std::cout << "  - Wrist Component: Controls HT motors m7-m8 (pitch and roll)" << std::endl;
        std::cout << "  - Gripper Component: Controls servo motor m9 (gripper actuation)" << std::endl;
        std::cout << std::endl;

        // Enable all motors
        std::cout << "🔌 Enabling all motors..." << std::endl;
        wrist.enable_all();
        gripper.enable_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout << "✅ All motors enabled" << std::endl;
        std::cout << std::endl;

        // === Wrist Control Demo ===
        std::cout << "=== WRIST CONTROL DEMO ===" << std::endl;
        std::cout << "Testing wrist pitch and roll movements..." << std::endl;
        std::cout << std::endl;

        // Test wrist pitch (m7)
        std::cout << "📍 Testing wrist pitch (m7)..." << std::endl;
        double pitch_angles[] = {0.0, 0.5, -0.5, 0.0}; // radians

        for (double pitch : pitch_angles) {
            std::cout << "  Setting pitch to " << std::fixed << std::setprecision(2)
                      << pitch * 180.0 / M_PI << " degrees..." << std::endl;

            wrist.set_pitch_angle(pitch, 1.0); // 1 rad/s velocity
            wrist.send_all_commands();

            std::this_thread::sleep_for(std::chrono::milliseconds(2000));

            double current_pitch = wrist.get_pitch_angle();
            std::cout << "  Current pitch: " << std::fixed << std::setprecision(2)
                      << current_pitch * 180.0 / M_PI << " degrees" << std::endl;
        }
        std::cout << std::endl;

        // Test wrist roll (m8)
        std::cout << "🔄 Testing wrist roll (m8)..." << std::endl;
        double roll_angles[] = {0.0, 0.8, -0.8, 0.0}; // radians

        for (double roll : roll_angles) {
            std::cout << "  Setting roll to " << std::fixed << std::setprecision(2)
                      << roll * 180.0 / M_PI << " degrees..." << std::endl;

            wrist.set_roll_angle(roll, 1.0); // 1 rad/s velocity
            wrist.send_all_commands();

            std::this_thread::sleep_for(std::chrono::milliseconds(2000));

            double current_roll = wrist.get_roll_angle();
            std::cout << "  Current roll: " << std::fixed << std::setprecision(2)
                      << current_roll * 180.0 / M_PI << " degrees" << std::endl;
        }
        std::cout << std::endl;

        // Test combined wrist motion
        std::cout << "🕺 Testing combined wrist motion..." << std::endl;
        wrist.set_wrist_pose(0.3, 0.4, 1.5); // pitch=0.3, roll=0.4 rad, velocity=1.5 rad/s
        wrist.send_all_commands();
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));

        auto wrist_pos = wrist.get_positions();
        std::cout << "  Current wrist pose - Pitch: " << std::fixed << std::setprecision(2)
                  << wrist_pos[0] * 180.0 / M_PI << "°, Roll: "
                  << wrist_pos[1] * 180.0 / M_PI << "°" << std::endl;
        std::cout << std::endl;

        // Return wrist to zero position
        wrist.set_wrist_pose(0.0, 0.0, 1.0);
        wrist.send_all_commands();
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        // === Gripper Control Demo ===
        std::cout << "=== GRIPPER CONTROL DEMO ===" << std::endl;
        std::cout << "Testing gripper open/close movements..." << std::endl;
        std::cout << std::endl;

        // Test gripper opening
        std::cout << "✋ Opening gripper..." << std::endl;
        gripper.open(0.6, 0.3); // speed=0.6, force=0.3
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        double openness = gripper.get_openness();
        std::cout << "  Current openness: " << std::fixed << std::setprecision(2)
                  << openness * 100 << "%" << std::endl;
        std::cout << std::endl;

        // Test gripper closing
        std::cout << "✊ Closing gripper..." << std::endl;
        gripper.close(0.4, 0.5); // speed=0.4, force=0.5
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        openness = gripper.get_openness();
        std::cout << "  Current openness: " << std::fixed << std::setprecision(2)
                  << openness * 100 << "%" << std::endl;
        std::cout << std::endl;

        // Test specific gripper positions
        std::cout << "🎯 Testing specific gripper positions..." << std::endl;
        double positions[] = {0.25, 0.5, 0.75, 0.5, 0.0};

        for (double pos : positions) {
            std::cout << "  Setting openness to " << std::fixed << std::setprecision(2)
                      << pos * 100 << "%" << std::endl;

            gripper.set_openness(pos, 0.5, 0.3);
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));

            openness = gripper.get_openness();
            std::cout << "  Actual openness: " << std::fixed << std::setprecision(2)
                      << openness * 100 << "%" << std::endl;
        }
        std::cout << std::endl;

        // === Coordinated Motion Demo ===
        std::cout << "=== COORDINATED MOTION DEMO ===" << std::endl;
        std::cout << "Testing simultaneous wrist and gripper movement..." << std::endl;
        std::cout << std::endl;

        // Open gripper first
        gripper.open(0.8, 0.2);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // Move wrist while gripper is open
        std::cout << "🤖 Moving wrist to grasp position..." << std::endl;
        wrist.set_wrist_pose(0.6, 0.2, 1.2);
        wrist.send_all_commands();
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));

        // Close gripper
        std::cout << "✊ Closing gripper to grasp..." << std::endl;
        gripper.close(0.5, 0.6);
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));

        // Move wrist with object
        std::cout << "🏋️ Lifting and rotating with object..." << std::endl;
        wrist.set_wrist_pose(-0.3, 1.0, 0.8);
        wrist.send_all_commands();
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));

        // Open gripper to release
        std::cout << "✋ Opening gripper to release..." << std::endl;
        gripper.open(0.7, 0.2);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // Return to home position
        std::cout << "🏠 Returning to home position..." << std::endl;
        wrist.set_wrist_pose(0.0, 0.0, 1.0);
        wrist.send_all_commands();
        gripper.set_openness(0.5, 0.5, 0.3);
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        // Print final status
        std::cout << std::endl;
        std::cout << "=== FINAL STATUS ===" << std::endl;

        std::cout << "📊 Wrist Status:" << std::endl;
        wrist.print_wrist_state();

        std::cout << std::endl;
        std::cout << "📊 Gripper Status:" << std::endl;
        gripper.print_gripper_state();

        // === Component Information ===
        std::cout << std::endl;
        std::cout << "=== COMPONENT INFORMATION ===" << std::endl;
        std::cout << "✅ Wrist Component manages:" << std::endl;
        std::cout << "  - Motor m7: HT4438 (Wrist Pitch)" << std::endl;
        std::cout << "  - Motor m8: HT4438 (Wrist Roll)" << std::endl;
        std::cout << std::endl;
        std::cout << "✅ Gripper Component manages:" << std::endl;
        std::cout << "  - Motor m9: Servo (Gripper Actuation)" << std::endl;
        std::cout << std::endl;

        // Disable all motors
        std::cout << "🔌 Disabling all motors..." << std::endl;
        wrist.disable_all();
        gripper.disable_all();
        std::cout << "✅ Demo completed successfully!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}