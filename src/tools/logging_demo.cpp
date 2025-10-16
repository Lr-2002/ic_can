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
 * @file logging_demo.cpp
 * @brief Logging and Frequency Monitoring Demo
 *
 * Demonstration of the new logging and frequency monitoring features:
 * - CSV logging of motor states and commands
 * - Real-time frequency monitoring
 * - Performance statistics tracking
 */

#include <chrono>
#include <ic_can/core/ic_can.hpp>
#include <iostream>
#include <thread>
#include <vector>

int main() {
  std::cout << "=== IC_CAN Logging and Frequency Monitoring Demo ==="
            << std::endl;
  std::cout << "This demo shows the new logging and monitoring capabilities"
            << std::endl;

  try {
    // Create IC_CAN controller with debug enabled
    auto controller = std::make_unique<ic_can::IC_CAN>(
        "F561E08C892274DB09496BCC1102DBC5", true);

    // Initialize system
    std::cout << "\n🔧 Initializing system..." << std::endl;
    if (!controller->initialize()) {
      std::cout << "❌ FAILED: System initialization failed" << std::endl;
      return -1;
    }
    std::cout << "✅ System initialized successfully" << std::endl;

    std::cout << "\n Now enable motors... " << std::endl;
    if (!controller->enable_all()) {
      std::cout << "Failed: could not enable" << std::endl;
    }
    std::cout << "Motors enabled" << std::endl;
    controller->enable_gravity_compensation();
    // Print system info
    controller->print_system_info();

    // Enable frequency monitoring
    std::cout << "\n📊 Enabling frequency monitoring..." << std::endl;
    controller->enable_frequency_monitoring();

    // Start logging to current directory (will create timestamped subdirectory)
    std::cout << "\n📝 Starting CSV logging with timestamped directory..."
              << std::endl;
    if (!controller->start_logging("./logs")) {
      std::cout << "❌ FAILED: Could not start logging" << std::endl;
      return -1;
    }
    std::cout
        << "✅ Logging started - writing to timestamped subdirectory in ./logs/"
        << std::endl;

    // Simulate some motor commands to generate data
    std::cout << "\n🤖 Simulating motor commands for 10 seconds..."
              << std::endl;

    // Create some test positions
    std::vector<std::vector<double>> test_positions = {
        {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},  // Zero position
        {0.0, 0.0, 0.0, 0.0, 0.0, 0.3, 0.0, 0.0, 0.0},  // Zero position
        {0.0, 0.0, 0.0, 0.0, 0.0, -0.3, 0.0, 0.0, 0.0}, // Zero position
        {0.0, 0.0, 0.0, 0.0, 0.4, -0.3, 0.0, 0.0, 0.0}, // Zero position
        /*{0.1, -0.1, 0.05, -0.05, 0.02, -0.02, 0.0, 0.0, 0.0}, // Small
           movement*/
        /*{0.2, -0.2, 0.1, -0.1, 0.04, -0.04, 0.0, 0.0, 0.0},   // Larger
           movement*/
        /*{-0.0, 0.1, -0.05, 0.05, -0.02, 0.02, 0.0, 0.0,*/
        /*0.0},                                        // Opposite movement*/
        {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0} // Back to zero
    };

    // Send commands and monitor performance
    for (int i = 0; i < 5; i++) {
      std::cout << "🎯 Sending test position set " << (i + 1) << "/5"
                << std::endl;

      // Set target positions with interpolation
      controller->set_target_positions_interpolated(test_positions[i], 1);

      // Start control loop briefly
      controller->start_control_loop(1000); // 50Hz
      std::this_thread::sleep_for(std::chrono::seconds(20));
      controller->stop_control_loop();

      // Print performance stats
      controller->print_performance_stats();

      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Show final performance statistics
    std::cout << "\n📊 Final Performance Statistics:" << std::endl;
    controller->print_performance_stats();

    // Get performance stats as map
    auto stats = controller->get_performance_stats();
    std::cout << "\n📈 Summary Statistics:" << std::endl;
    std::cout << "   Total commands sent: " << stats["total_commands_sent"]
              << std::endl;
    std::cout << "   Total messages received: "
              << stats["total_messages_received"] << std::endl;
    std::cout << "   Average send frequency: " << stats["send_frequency"]
              << " Hz" << std::endl;
    std::cout << "   Average receive frequency: " << stats["receive_frequency"]
              << " Hz" << std::endl;
    std::cout << "   Data send rate: " << stats["send_rate_kbps"] << " kB/s"
              << std::endl;
    std::cout << "   Data receive rate: " << stats["receive_rate_kbps"]
              << " kB/s" << std::endl;

    // Stop logging
    std::cout << "\n🛑 Stopping logging..." << std::endl;
    controller->stop_logging();

    // Disable frequency monitoring
    std::cout << "🔌 Disabling frequency monitoring..." << std::endl;
    controller->disable_frequency_monitoring();

    // Shutdown system
    std::cout << "🔄 Shutting down system..." << std::endl;
    controller->shutdown();

    std::cout << "\n✅ Demo completed successfully!" << std::endl;
    std::cout
        << "📁 Check the './logs' directory for timestamped subdirectories:"
        << std::endl;
    std::cout << "   Each subdirectory contains:" << std::endl;
    std::cout << "   - joint_commands.csv: Target positions/velocities/torques"
              << std::endl;
    std::cout << "   - motor_states.csv: Actual motor states" << std::endl;
    std::cout << "   Format: ic_can_log_YYYYMMDD_HHMMSS_mmm/" << std::endl;

    return 0;
  } catch (const std::exception &e) {
    std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
    return -1;
  }
}
