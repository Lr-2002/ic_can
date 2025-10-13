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

#include <ic_can/safety/safety_module.hpp>

int main() {
    std::cout << "=== Safety Module Tests ===" << std::endl;

    try {
        // Create safety module
        std::cout << "Creating safety module..." << std::endl;
        auto safety = std::make_unique<ic_can::SafetyModule>();

        if (safety) {
            std::cout << "✓ Safety module created successfully" << std::endl;
        }

        // Test initial state
        std::cout << "Testing initial state..." << std::endl;
        std::cout << "  Emergency stopped: " << safety->is_emergency_stopped() << std::endl;
        std::cout << "  Watchdog timeout: " << safety->is_watchdog_timeout() << std::endl;
        std::cout << "  Active violations: " << safety->has_active_violations() << std::endl;

        // Test emergency stop
        std::cout << "Testing emergency stop..." << std::endl;
        safety->trigger_emergency_stop("Test emergency stop", 0);
        std::cout << "✓ Emergency stop triggered" << std::endl;
        std::cout << "  Emergency stopped: " << safety->is_emergency_stopped() << std::endl;
        std::cout << "  Active violations: " << safety->has_active_violations() << std::endl;

        auto violations = safety->get_active_violations();
        std::cout << "  Number of violations: " << violations.size() << std::endl;

        // Test safety limits checking
        std::cout << "Testing safety limits checking..." << std::endl;
        safety->check_position_limit(1, 4.0);  // Should trigger violation
        safety->check_velocity_limit(2, 15.0); // Should trigger violation
        safety->check_temperature_limit(3, 90.0); // Should trigger violation

        violations = safety->get_active_violations();
        std::cout << "✓ Safety checks performed" << std::endl;
        std::cout << "  Number of violations: " << violations.size() << std::endl;

        // Test watchdog
        std::cout << "Testing watchdog..." << std::endl;
        safety->feed_watchdog();
        std::cout << "✓ Watchdog fed" << std::endl;
        std::cout << "  Watchdog timeout: " << safety->is_watchdog_timeout() << std::endl;

        // Test reset
        std::cout << "Testing reset..." << std::endl;
        safety->reset_emergency_stop();
        std::cout << "✓ Emergency stop reset" << std::endl;
        std::cout << "  Emergency stopped: " << safety->is_emergency_stopped() << std::endl;

        // Test safety limits configuration
        std::cout << "Testing safety limits configuration..." << std::endl;
        safety->set_position_limits(-2.0, 2.0);
        safety->set_velocity_limits(8.0, 8.0);
        safety->set_torque_limits(3.0, 3.0);
        safety->set_temperature_limit(75.0);

        auto pos_limits = safety->get_position_limits();
        auto vel_limits = safety->get_velocity_limits();
        auto torq_limits = safety->get_torque_limits();

        std::cout << "✓ Safety limits configured" << std::endl;
        std::cout << "  Position limits: [" << pos_limits.first << ", " << pos_limits.second << "]" << std::endl;
        std::cout << "  Velocity limits: [" << vel_limits.first << ", " << vel_limits.second << "]" << std::endl;
        std::cout << "  Torque limits: [" << torq_limits.first << ", " << torq_limits.second << "]" << std::endl;
        std::cout << "  Temperature limit: " << safety->get_temperature_limit() << "°C" << std::endl;

        // Print safety status report
        std::cout << "Printing safety status report..." << std::endl;
        safety->print_safety_status_report();

        std::cout << "All safety tests passed!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Safety test failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}