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
  std::cout << "🦾 IC_CAN Wrist Position Monitoring Test (Simple)" << std::endl;
  std::cout << "===============================================" << std::endl;

  // Set up signal handlers for graceful shutdown
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  // Parse command line arguments
  std::string device_sn = "FT4X2Y2A"; // Default device serial number
  bool debug_enabled = false;
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
    ic_can::IC_CAN ic_can(device_sn, debug_enabled);

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

    // Test basic wrist monitoring without enabling motors (safer)
    std::cout << "🦾 Testing wrist position monitoring..." << std::endl;

    // Test 1: Get wrist positions (should return defaults if not connected)
    std::cout << "\n--- Test 1: Get Wrist Positions ---" << std::endl;
    auto wrist_pos = ic_can.get_wrist_positions();
    std::cout << "Motor 7 Position: " << std::fixed << std::setprecision(3)
              << wrist_pos[0] << " rad (" << (wrist_pos[0] * 180.0 / M_PI) << "°)" << std::endl;
    std::cout << "Motor 8 Position: " << std::fixed << std::setprecision(3)
              << wrist_pos[1] << " rad (" << (wrist_pos[1] * 180.0 / M_PI) << "°)" << std::endl;

    // Test 2: Get wrist velocities
    std::cout << "\n--- Test 2: Get Wrist Velocities ---" << std::endl;
    auto wrist_vel = ic_can.get_wrist_velocities();
    std::cout << "Motor 7 Velocity: " << std::fixed << std::setprecision(3)
              << wrist_vel[0] << " rad/s" << std::endl;
    std::cout << "Motor 8 Velocity: " << std::fixed << std::setprecision(3)
              << wrist_vel[1] << " rad/s" << std::endl;

    // Test 3: Get wrist torques
    std::cout << "\n--- Test 3: Get Wrist Torques ---" << std::endl;
    auto wrist_tau = ic_can.get_wrist_torques();
    std::cout << "Motor 7 Torque: " << std::fixed << std::setprecision(3)
              << wrist_tau[0] << " Nm" << std::endl;
    std::cout << "Motor 8 Torque: " << std::fixed << std::setprecision(3)
              << wrist_tau[1] << " Nm" << std::endl;

    // Test 4: Test wrist refresh (without motor connection)
    std::cout << "\n--- Test 4: Wrist Refresh ---" << std::endl;
    if (ic_can.refresh_wrist_motors_only()) {
      std::cout << "✅ Wrist refresh command sent successfully" << std::endl;
    } else {
      std::cout << "❌ Wrist refresh failed (expected if not connected)" << std::endl;
    }

    // Test 5: Test monitoring data structure
    std::cout << "\n--- Test 5: Monitoring Data Structure ---" << std::endl;
    auto wrist_data = ic_can.get_wrist_monitoring_data();
    std::cout << "Data structure contains:" << std::endl;
    for (const auto& [key, value] : wrist_data) {
      std::cout << "  " << key << ": [";
      for (size_t i = 0; i < value.size(); i++) {
        std::cout << value[i];
        if (i < value.size() - 1) std::cout << ", ";
      }
      std::cout << "]" << std::endl;
    }

    // Test 6: Try to start monitoring (may fail without motors)
    std::cout << "\n--- Test 6: Start Wrist Monitoring ---" << std::endl;
    std::cout << "Attempting to start wrist monitoring at " << monitor_frequency << "Hz..." << std::endl;

    bool monitoring_started = ic_can.start_wrist_position_monitoring(monitor_frequency);
    if (monitoring_started) {
      std::cout << "✅ Wrist monitoring started successfully" << std::endl;

      // Let it run for a few seconds to test the monitoring thread
      std::cout << "📊 Running monitoring for 5 seconds..." << std::endl;
      auto start_time = std::chrono::steady_clock::now();

      while (keep_running &&
             std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time).count() < 5.0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Print status updates
        auto data = ic_can.get_wrist_monitoring_data();
        auto positions = data["positions"];
        auto update_age = data["update_age_ms"];

        std::cout << "\r🦾 Status: M7=" << std::fixed << std::setprecision(1)
                  << (positions[0] * 180.0 / M_PI) << "° M8="
                  << (positions[1] * 180.0 / M_PI) << "° Age="
                  << update_age[0] << "ms   " << std::flush;
      }
      std::cout << std::endl;

      // Stop monitoring
      std::cout << "🛑 Stopping wrist monitoring..." << std::endl;
      ic_can.stop_wrist_position_monitoring();
      std::cout << "✅ Wrist monitoring stopped" << std::endl;

    } else {
      std::cout << "⚠️  Wrist monitoring failed to start (expected without motor connection)" << std::endl;
    }

    // Test 7: Test wrist status printing
    std::cout << "\n--- Test 7: Print Wrist Status ---" << std::endl;
    ic_can.print_wrist_status();

    // Test 8: Test monitoring status query
    std::cout << "\n--- Test 8: Monitoring Status Query ---" << std::endl;
    std::cout << "Is wrist monitoring running? "
              << (ic_can.is_wrist_monitoring_running() ? "Yes" : "No") << std::endl;

    std::cout << "\n🧪 All tests completed successfully!" << std::endl;
    std::cout << "📝 Note: Some tests may show default values if wrist motors are not connected" << std::endl;

    // Shutdown system
    std::cout << "🔗 Shutting down IC_CAN system..." << std::endl;
    ic_can.shutdown();

    std::cout << "✅ Simple test completed successfully" << std::endl;

  } catch (const std::exception &e) {
    std::cout << "❌ Exception caught: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}