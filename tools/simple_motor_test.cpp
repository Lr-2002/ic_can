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
 * @file simple_motor_test.cpp
 * @brief Simple Motor Movement Test
 *
 * Basic test to verify motor movement:
 * - Connect to hardware
 * - Enable motors
 * - Send simple position commands
 * - Monitor feedback
 */

#include <chrono>
#include <ic_can/core/ic_can.hpp>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

int main() {
  std::cout << "=== Simple Motor Movement Test ===" << std::endl;
  std::cout << "This test requires actual hardware connection" << std::endl;

  try {
    // Create IC_CAN controller
    auto controller = std::make_unique<ic_can::IC_CAN>(
        "F561E08C892274DB09496BCC1102DBC5", true);

    // Initialize system
    std::cout << "\n🔧 Initializing system..." << std::endl;
    if (!controller->initialize()) {
      std::cout << "❌ FAILED: System initialization failed" << std::endl;
      std::cout << "💡 Make sure USB2CAN device is connected" << std::endl;
      return -1;
    }
    std::cout << "✅ System initialized" << std::endl;

    // IMPORTANT: Enable motors first!
    std::cout << "\n⚡ Enabling motors..." << std::endl;
    if (!controller->enable_all()) {
      std::cout << "❌ FAILED: Could not enable motors" << std::endl;
      return -1;
    }
    std::cout << "✅ Motors enabled" << std::endl;

    // Wait for motors to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Read initial positions
    std::cout << "\n📍 Reading initial positions..." << std::endl;
    controller->refresh_all();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    auto initial_pos = controller->get_joint_positions();

    std::cout << "Initial positions (rad): ";
    for (int i = 0; i < 6; i++) {
      std::cout << std::fixed << std::setprecision(3) << initial_pos[i] << " ";
    }
    std::cout << std::endl;

    // Check if motors are responding (should not all be exactly 0.0)
    bool all_zero = true;
    for (int i = 0; i < 6; i++) {
      if (std::abs(initial_pos[i]) > 0.001) {
        all_zero = false;
        break;
      }
    }

    if (all_zero) {
      std::cout
          << "⚠️  WARNING: All positions are 0.0 - motors may not be responding"
          << std::endl;
      std::cout << "💡 Check:" << std::endl;
      std::cout << "   - USB2CAN device connected" << std::endl;
      std::cout << "   - Motor power supply ON" << std::endl;
      std::cout << "   - CAN cable connected" << std::endl;
      std::cout << "   - Device serial number correct" << std::endl;
    }

    // Test simple movement
    std::cout << "\n🎯 Testing simple movement..." << std::endl;

    // Create small test movement
    std::vector<double> test_pos = initial_pos;
    test_pos[5] += 0.1; // Move motor 1 by 0.1 rad

    std::cout << "Moving motor 5 from " << initial_pos[5] << " to "
              << test_pos[5] << " rad" << std::endl;

    // Send position command directly (no interpolation)
    std::cout << "🔧 About to call set_joint_positions..." << std::endl;
    std::cout << "🔧 test_pos size: " << test_pos.size() << std::endl;
    bool result = controller->set_joint_positions(test_pos, {}, {});
    std::cout << "🔧 set_joint_positions returned: "
              << (result ? "true" : "false") << std::endl;

    // Wait and check result
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    controller->refresh_all();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    auto final_pos = controller->get_joint_positions();

    std::cout << "Final motor 6 position: " << std::fixed
              << std::setprecision(5) << final_pos[5] << " rad" << std::endl;

    // Check if movement happened
    double movement = std::abs(final_pos[5] - initial_pos[5]);
    if (movement > 0.01) { // 1cm movement threshold
      std::cout << "✅ SUCCESS: Motor moved by " << movement << " rad"
                << std::endl;
    } else {
      std::cout << "❌ NO MOVEMENT: Motor position unchanged" << std::endl;
      std::cout << "💡 Troubleshooting:" << std::endl;
      std::cout << "   - Check motor power supply" << std::endl;
      std::cout << "   - Verify CAN connections" << std::endl;
      std::cout << "   - Try higher motor gains" << std::endl;
      std::cout << "   - Check for motor errors" << std::endl;
    }

    // Return to initial position
    std::cout << "\n🏠 Returning to initial position..." << std::endl;
    controller->set_joint_positions(initial_pos, {}, {});
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // Disable motors
    controller->disable_all();
    std::cout << "✅ Motors disabled" << std::endl;

    // Shutdown
    controller->shutdown();
    std::cout << "\n✅ Test completed" << std::endl;

    return 0;

  } catch (const std::exception &e) {
    std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
    return -1;
  }
}
