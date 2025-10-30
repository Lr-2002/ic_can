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
  std::cout << "🦾 IC_CAN Wrist Monitoring Test (Minimal)" << std::endl;
  std::cout << "===========================================" << std::endl;

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
      std::cout << "   This is expected if no USB2CAN device is connected." << std::endl;
      std::cout << "   The test will continue with default values." << std::endl;
    } else {
      std::cout << "✅ IC_CAN system initialized successfully" << std::endl;
    }

    // Print system info
    ic_can.print_system_info();
    std::cout << std::endl;

    // Test 1: Test wrist position APIs
    std::cout << "--- Test 1: Wrist Position APIs ---" << std::endl;
    auto wrist_pos = ic_can.get_wrist_positions();
    auto wrist_vel = ic_can.get_wrist_velocities();
    auto wrist_tau = ic_can.get_wrist_torques();

    std::cout << "Motor 7: Pos=" << std::fixed << std::setprecision(3)
              << wrist_pos[0] << " rad (" << (wrist_pos[0] * 180.0 / M_PI) << "°), "
              << "Vel=" << wrist_vel[0] << " rad/s, "
              << "Tau=" << wrist_tau[0] << " Nm" << std::endl;
    std::cout << "Motor 8: Pos=" << std::fixed << std::setprecision(3)
              << wrist_pos[1] << " rad (" << (wrist_pos[1] * 180.0 / M_PI) << "°), "
              << "Vel=" << wrist_vel[1] << " rad/s, "
              << "Tau=" << wrist_tau[1] << " Nm" << std::endl;

    // Test 2: Test monitoring data structure
    std::cout << "\n--- Test 2: Monitoring Data Structure ---" << std::endl;
    auto wrist_data = ic_can.get_wrist_monitoring_data();
    std::cout << "Monitoring data structure:" << std::endl;
    for (const auto& [key, value] : wrist_data) {
      std::cout << "  " << key << ": [";
      for (size_t i = 0; i < value.size(); i++) {
        std::cout << value[i];
        if (i < value.size() - 1) std::cout << ", ";
      }
      std::cout << "]" << std::endl;
    }

    // Test 3: Test wrist refresh (may fail without connection)
    std::cout << "\n--- Test 3: Wrist Refresh ---" << std::endl;
    bool refresh_success = ic_can.refresh_wrist_motors_only();
    if (refresh_success) {
      std::cout << "✅ Wrist refresh command sent successfully" << std::endl;
    } else {
      std::cout << "⚠️  Wrist refresh failed (expected if not connected)" << std::endl;
    }

    // Test 4: Test wrist status printing
    std::cout << "\n--- Test 4: Print Wrist Status ---" << std::endl;
    ic_can.print_wrist_status();

    // Test 5: Test monitoring start/stop
    std::cout << "\n--- Test 5: Monitoring Start/Stop ---" << std::endl;
    std::cout << "Starting wrist monitoring for 3 seconds..." << std::endl;

    bool monitoring_started = ic_can.start_wrist_position_monitoring(monitor_frequency);
    if (monitoring_started) {
      std::cout << "✅ Wrist monitoring started" << std::endl;

      // Run for 3 seconds
      auto start_time = std::chrono::steady_clock::now();
      int update_count = 0;

      while (keep_running &&
             std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time).count() < 3.0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        update_count++;

        if (update_count % 5 == 0) {  // Every 1 second
          auto data = ic_can.get_wrist_monitoring_data();
          auto positions = data["positions"];
          auto update_age = data["update_age_ms"];
          bool is_running = ic_can.is_wrist_monitoring_running();

          std::cout << "Update " << (update_count/5) << ": M7="
                    << std::fixed << std::setprecision(1)
                    << (positions[0] * 180.0 / M_PI) << "° M8="
                    << (positions[1] * 180.0 / M_PI) << "° "
                    << "Age=" << update_age[0] << "ms "
                    << "Running=" << (is_running ? "Yes" : "No") << std::endl;
        }
      }

      std::cout << "Stopping wrist monitoring..." << std::endl;
      ic_can.stop_wrist_position_monitoring();
      std::cout << "✅ Wrist monitoring stopped" << std::endl;
    } else {
      std::cout << "⚠️  Wrist monitoring failed to start" << std::endl;
    }

    // Test 6: Final status check
    std::cout << "\n--- Test 6: Final Status Check ---" << std::endl;
    std::cout << "Is wrist monitoring running? "
              << (ic_can.is_wrist_monitoring_running() ? "Yes" : "No") << std::endl;

    // Final wrist status
    std::cout << "\n--- Final Wrist Status ---" << std::endl;
    ic_can.print_wrist_status();

    std::cout << "\n🧪 All tests completed!" << std::endl;
    std::cout << "📝 Results:" << std::endl;
    std::cout << "   - API tests: ✅ PASSED (all methods callable)" << std::endl;
    std::cout << "   - Data structure: ✅ PASSED (monitoring data accessible)" << std::endl;
    std::cout << "   - Communication: " << (refresh_success ? "✅ CONNECTED" : "⚠️  NO DEVICE") << std::endl;
    std::cout << "   - Real data: " << (wrist_pos[0] != 0.0 || wrist_pos[1] != 0.0 ? "✅ DETECTED" : "⚠️  DEFAULT VALUES") << std::endl;

    // Shutdown system
    std::cout << "\n🔗 Shutting down IC_CAN system..." << std::endl;
    ic_can.shutdown();

    std::cout << "✅ Minimal test completed successfully" << std::endl;

  } catch (const std::exception &e) {
    std::cout << "❌ Exception caught: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}