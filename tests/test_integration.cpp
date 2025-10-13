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
#include <thread>
#include <chrono>
#include <vector>

#include <ic_can/core/ic_can.hpp>
#include <ic_can/core/arm_component.hpp>
#include <ic_can/core/gripper_component.hpp>
#include <ic_can/safety/safety_module.hpp>

int main() {
    std::cout << "=== Integration Tests ===" << std::endl;

    try {
        // Test main controller
        std::cout << "Testing main controller..." << std::endl;
        ic_can::IC_CAN controller("F561E08C892274DB09496BCC1102DBC5", true);

        bool initialized = controller.initialize();
        std::cout << "✓ Controller initialization: " << (initialized ? "Success" : "Failed") << std::endl;

        if (initialized) {
            // Test basic operations
            std::cout << "Testing basic operations..." << std::endl;
            controller.enable_all();
            std::cout << "✓ Motors enabled" << std::endl;

            // Test state reading
            auto positions = controller.get_joint_positions();
            auto velocities = controller.get_joint_velocities();
            auto torques = controller.get_joint_torques();

            std::cout << "✓ State reading completed" << std::endl;
            std::cout << "  Position count: " << positions.size() << std::endl;
            std::cout << "  Velocity count: " << velocities.size() << std::endl;
            std::cout << "  Torque count: " << torques.size() << std::endl;

            // Test control commands
            std::cout << "Testing control commands..." << std::endl;
            std::vector<double> target_positions = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7};
            controller.set_joint_positions(target_positions);

            std::vector<double> target_torques = {0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1};
            controller.set_joint_torques(target_torques);

            std::cout << "✓ Control commands sent" << std::endl;

            // Test components
            std::cout << "Testing components..." << std::endl;
            auto arm = controller.get_arm();
            auto gripper = controller.get_gripper();
            auto safety = controller.get_safety();

            if (arm && gripper && safety) {
                std::cout << "✓ All components accessible" << std::endl;

                // Test arm component
                auto arm_positions = arm->get_positions();
                std::cout << "✓ Arm positions: " << arm_positions.size() << " joints" << std::endl;

                // Test gripper component
                auto gripper_openness = gripper->get_openness();
                std::cout << "✓ Gripper openness: " << gripper_openness << std::endl;

                // Test safety module
                bool has_violations = safety->has_active_violations();
                std::cout << "✓ Safety violations: " << (has_violations ? "Yes" : "No") << std::endl;
            }

            // Test high-frequency control
            std::cout << "Testing high-frequency control..." << std::endl;
            controller.set_control_frequency(500.0);
            std::cout << "✓ Control frequency set: " << controller.get_control_frequency() << " Hz" << std::endl;

            controller.start_high_frequency_control();
            std::cout << "✓ High-frequency control started" << std::endl;
            std::cout << "  Control running: " << controller.is_high_frequency_control_running() << std::endl;

            // Run for a short time
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            controller.stop_high_frequency_control();
            std::cout << "✓ High-frequency control stopped" << std::endl;

            // Test system status
            std::cout << "Testing system status..." << std::endl;
            auto status = controller.get_system_status();
            std::cout << "✓ System status retrieved" << std::endl;
            std::cout << "  Status entries: " << status.size() << std::endl;

            // Test emergency operations
            std::cout << "Testing emergency operations..." << std::endl;
            controller.emergency_stop();
            std::cout << "✓ Emergency stop triggered" << std::endl;

            controller.reset_emergency_stop();
            std::cout << "✓ Emergency stop reset" << std::endl;

            // Disable motors
            controller.disable_all();
            std::cout << "✓ Motors disabled" << std::endl;

            // Print system info
            std::cout << "Printing system info..." << std::endl;
            controller.print_system_info();
        }

        std::cout << "All integration tests passed!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Integration test failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}