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

#include "ic_can/core/ic_can.hpp"
#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <thread>

// Global flag for graceful shutdown
std::atomic<bool> keep_running(true);

void signal_handler(int signal) {
  std::cout << "\n🛑 Received signal " << signal
            << ", shutting down gracefully..." << std::endl;
  keep_running = false;
}

int main(int argc, char *argv[]) {
  std::cout << "🦾 IC_CAN Wrist Position Monitoring Test" << std::endl;
  std::cout << "========================================" << std::endl;

  // Set up signal handlers for graceful shutdown
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  // Parse command line arguments
  std::string device_sn =
      "693D3DE86DF5940C8BC74A5B46A3CE2E"; // Default device serial number
  bool debug_enabled = true;
  double monitor_frequency = 50.0;

  if (argc > 1) {
    device_sn = argv[1];
  }
  if (argc > 2 && std::string(argv[2]) == "--debug") {
    debug_enabled = true;
  }
  if (argc > 3) {
    monitor_frequency = std::stod(argv[3]);
  }

  std::cout << "Device SN: " << device_sn << std::endl;
  std::cout << "Debug: " << (debug_enabled ? "Enabled" : "Disabled")
            << std::endl;
  std::cout << "Monitor Frequency: " << monitor_frequency << " Hz" << std::endl;
  std::cout << std::endl;

  try {
    // Create IC_CAN instance
    std::cout << "🔧 Creating IC_CAN instance..." << std::endl;
    ic_can::IC_CAN ic_can("693D3DE86DF5940C8BC74A5B46A3CE2E", debug_enabled);

    // Initialize the system
    std::cout << "🔗 Initializing IC_CAN system..." << std::endl;
    if (!ic_can.initialize()) {
      std::cout << "❌ Failed to initialize IC_CAN system" << std::endl;
      return 1;
    }
    std::cout << "✅ IC_CAN system initialized successfully" << std::endl;

    // Print system info
    ic_can.print_system_info();
    std::cout << std::endl;

    // Enable all motors
    std::cout << "⚡ Enabling all motors..." << std::endl;
    if (!ic_can.enable_all()) {
      std::cout << "⚠️ Warning: Failed to enable all motors" << std::endl;
    } else {
      std::cout << "✅ All motors enabled" << std::endl;
    }

    // Test initial communication
    std::cout << "🧪 Testing CAN communication before monitoring..." << std::endl;
    if (!ic_can.refresh_all()) {
      std::cout << "⚠️ Warning: Initial refresh failed" << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Start wrist position monitoring
    std::cout << "🦾 Starting wrist position monitoring..." << std::endl;
    if (!ic_can.start_wrist_position_monitoring(monitor_frequency)) {
      std::cout << "❌ Failed to start wrist monitoring" << std::endl;
      return 1;
    }
    std::cout << "✅ Wrist monitoring started" << std::endl;

    // Main monitoring loop
    std::cout << "\n📊 Starting monitoring loop (Press Ctrl+C to stop)..."
              << std::endl;
    std::cout << "================================================="
              << std::endl;

    auto last_status_time = std::chrono::steady_clock::now();
    const auto status_interval = std::chrono::milliseconds(100);

    while (keep_running) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      auto now = std::chrono::steady_clock::now();

      // Print detailed status every 5 seconds
      if (now - last_status_time >= status_interval) {
        std::cout << "\n--- Wrist Status Update ---" << std::endl;
        ic_can.print_wrist_status();

        // Get monitoring data
        auto wrist_data = ic_can.get_wrist_monitoring_data();
        auto positions = wrist_data["positions"];
        auto velocities = wrist_data["velocities"];
        auto torques = wrist_data["torques"];
        auto update_age = wrist_data["update_age_ms"][0];

        std::cout << "Data freshness: " << update_age << " ms" << std::endl;
        std::cout << "Monitoring active: "
                  << (ic_can.is_wrist_monitoring_running() ? "Yes" : "No")
                  << std::endl;

        // Test individual methods
        std::cout << "\n--- Method Test Results ---" << std::endl;
        auto wrist_pos = ic_can.get_wrist_positions();
        auto wrist_vel = ic_can.get_wrist_velocities();
        auto wrist_tau = ic_can.get_wrist_torques();

        std::cout << "Motor 7: Pos=" << std::fixed << std::setprecision(3)
                  << wrist_pos[0] << " rad (" << (wrist_pos[0] * 180.0 / M_PI)
                  << "°), "
                  << "Vel=" << wrist_vel[0] << " rad/s, "
                  << "Tau=" << wrist_tau[0] << " Nm" << std::endl;
        std::cout << "Motor 8: Pos=" << std::fixed << std::setprecision(3)
                  << wrist_pos[1] << " rad (" << (wrist_pos[1] * 180.0 / M_PI)
                  << "°), "
                  << "Vel=" << wrist_vel[1] << " rad/s, "
                  << "Tau=" << wrist_tau[1] << " Nm" << std::endl;

        // Test refresh functionality
        if (ic_can.refresh_wrist_motors_only()) {
          std::cout << "✅ Wrist refresh successful" << std::endl;
        } else {
          std::cout << "❌ Wrist refresh failed" << std::endl;
        }

        last_status_time = now;
      }

      // Quick status indicator
      if (keep_running) {
        auto wrist_data = ic_can.get_wrist_monitoring_data();
        auto positions = wrist_data["positions"];
        std::cout << "\r🦾 Live: M7=" << std::fixed << std::setprecision(2)
                  << (positions[0] * 180.0 / M_PI)
                  << "° M8=" << (positions[1] * 180.0 / M_PI) << "°   "
                  << std::flush;
      }
    }

    std::cout << "\n🛑 Shutting down..." << std::endl;

    // Stop wrist monitoring
    std::cout << "🦾 Stopping wrist monitoring..." << std::endl;
    ic_can.stop_wrist_position_monitoring();

    // Disable motors
    std::cout << "🔌 Disabling all motors..." << std::endl;
    ic_can.disable_all();

    // Shutdown system
    std::cout << "🔗 Shutting down IC_CAN system..." << std::endl;
    ic_can.shutdown();

    std::cout << "✅ Test completed successfully" << std::endl;

  } catch (const std::exception &e) {
    std::cout << "❌ Exception caught: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
