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
#include <chrono>
#include <thread>

#include <ic_can/core/ic_can.hpp>
#include <ic_can/motors/base_motor.hpp>
#include <ic_can/motors/dm_motor.hpp>

int main() {
    std::cout << "=== Motor Tests ===" << std::endl;

    try {
        // Test DM motor creation
        std::cout << "Testing DM motor creation..." << std::endl;
        auto dm_motor = std::make_unique<ic_can::DMMotor>(1, ic_can::DMMotorType::DM4310, "DM4310_001");

        if (dm_motor) {
            std::cout << "✓ DM motor created successfully" << std::endl;
            std::cout << "  Motor ID: " << dm_motor->get_motor_id() << std::endl;
            std::cout << "  Motor Type: " << static_cast<int>(dm_motor->get_motor_type()) << std::endl;
        }

        // Test motor state operations
        std::cout << "Testing motor state operations..." << std::endl;
        dm_motor->enable();
        std::cout << "✓ Motor enabled: " << dm_motor->is_enabled() << std::endl;

        dm_motor->update_state(0.5, 0.1, 0.2, 25.0, 0);
        std::cout << "✓ State updated" << std::endl;
        std::cout << "  Position: " << dm_motor->get_position() << std::endl;
        std::cout << "  Velocity: " << dm_motor->get_velocity() << std::endl;
        std::cout << "  Torque: " << dm_motor->get_torque() << std::endl;
        std::cout << "  Temperature: " << dm_motor->get_temperature() << std::endl;

        // Test safety limits
        std::cout << "Testing safety limits..." << std::endl;
        dm_motor->set_position_limits(-3.14159, 3.14159);
        dm_motor->set_velocity_limits(10.0, 10.0);
        dm_motor->set_torque_limits(5.0, 5.0);

        std::cout << "✓ Safety limits set" << std::endl;
        std::cout << "  Position limits: [" << dm_motor->get_min_position() << ", " << dm_motor->get_max_position() << "]" << std::endl;
        std::cout << "  Max velocity: " << dm_motor->get_max_velocity() << std::endl;
        std::cout << "  Max torque: " << dm_motor->get_max_torque() << std::endl;

        // Test MIT control command encoding
        std::cout << "Testing MIT control command encoding..." << std::endl;
        auto command = dm_motor->encode_mit_command(1.0, 0.5, 0.2, 50.0, 5.0);
        if (!command.empty()) {
            std::cout << "✓ MIT command encoded successfully (" << command.size() << " bytes)" << std::endl;
        }

        std::cout << "All motor tests passed!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Motor test failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}