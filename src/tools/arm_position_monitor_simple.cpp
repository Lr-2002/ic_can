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
 * @file arm_position_monitor_simple.cpp
 * @brief Simple Robot System Position Monitoring Tool
 *
 * Simplified monitoring tool that only does position reading and setting:
 * - Gets current joint positions from all 9 motors
 * - Sets the same positions back (for torque compensation)
 * - No profiling, no display, no logging - just the essential operations
 */

#include <chrono>
#include <csignal>
#include <ic_can/core/ic_can.hpp>
#include <iostream>

static volatile bool g_running = true;

void signal_handler(int signal) {
  std::cout << "\n⚠️  Received signal " << signal << ", stopping monitor..."
            << std::endl;
  g_running = false;
}

int main(int argc, char *argv[]) {
  try {
    std::cout << "=== IC_CAN Simple Position Monitor ===" << std::endl;
    std::cout << "Real-time monitoring for 9-joint system (motors 1-9)" << std::endl;
    std::cout << "Only position reading and setting - no extra operations" << std::endl;

    // Parse command line arguments
    double frequency = 10.0; // Default frequency
    bool enable_logging = false;
    double duration_seconds = 0.0; // Run continuously by default

    // Simple command line parsing
    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "-f" && i + 1 < argc) {
        frequency = std::stod(argv[++i]);
      } else if (arg == "-l") {
        enable_logging = true;
      } else if (arg == "-t" && i + 1 < argc) {
        duration_seconds = std::stod(argv[++i]);
      } else if (arg == "-h") {
        std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  -f <freq>     Update frequency in Hz (default: 10)" << std::endl;
        std::cout << "  -l            Enable comprehensive data logging to timestamped directory" << std::endl;
        std::cout << "  -t <seconds>  Run for specified time, then exit" << std::endl;
        std::cout << "  -h            Show this help message" << std::endl;
        return 0;
      }
    }

    std::cout << "📊 Monitoring frequency: " << frequency << " Hz" << std::endl;
    std::cout << "📝 Logging: " << (enable_logging ? "Enabled" : "Disabled") << std::endl;
    if (duration_seconds > 0) {
      std::cout << "⏱️  Duration: " << duration_seconds << " seconds" << std::endl;
    } else {
      std::cout << "⏱️  Duration: Continuous (Ctrl+C to stop)" << std::endl;
    }

    // Set up signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Create IC_CAN controller
    auto controller =
        std::make_unique<ic_can::IC_CAN>("693D3DE86DF5940C8BC74A5B46A3CE2E",
                                         true); // Debug enabled

    // Initialize system
    if (!controller->initialize()) {
      std::cout << "❌ FAILED: System initialization failed" << std::endl;
      return -1;
    }

    std::cout << "✅ System initialized successfully" << std::endl;

    // Set to TEACH_MODE for monitoring (gravity + friction compensation only)
    std::cout << "\n🎓 Setting TEACH_MODE for monitoring..." << std::endl;
    controller->set_control_mode(ic_can::IC_CAN::ControlMode::TEACH_MODE);

    // Enable compensation for monitoring
    controller->enable_gravity_compensation();
    controller->enable_friction_compensation();

    // Start logging if requested
    if (enable_logging) {
      controller->start_logging("simple_position_monitor");
    }

    std::cout << "\n🚀 Starting simple position monitoring..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;

    // Simple monitoring loop - only get positions and set them
    auto start_time = std::chrono::steady_clock::now();

    while (g_running) {
      // Check duration limit
      if (duration_seconds > 0) {
        auto elapsed =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time).count();
        if (elapsed >= duration_seconds) {
          std::cout << "\n⏱️  Time limit reached, stopping monitor..."
                    << std::endl;
          break;
        }
      }

      // Get current positions
      auto positions = controller->get_joint_positions();

      // Set the same positions back (this does torque compensation)
      std::vector<double> empty = {0, 0, 0, 0, 0, 0, 0, 0, 0};
      controller->set_joint_positions(positions, empty, empty);
    }

    // Stop logging if enabled
    if (enable_logging) {
      controller->stop_logging();
    }

    // Disable motors
    controller->disable_all();
    std::cout << "\n🎉 Simple position monitoring completed!" << std::endl;

    return 0;

  } catch (const std::exception &e) {
    std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
    return -1;
  }
}