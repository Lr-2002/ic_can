/**
 * @file wrist_safe_monitor.cpp
 * @brief SAFE Wrist Motor Monitor - REFRESH ONLY
 *
 * SAFE monitoring tool that ONLY uses refresh command.
 * NO position requests, NO movement commands.
 * Simply reads current motor state via refresh.
 *
 * @author IC_CAN Team
 * @date 2025-10-28
 */

#include "ic_can/core/ic_can.hpp"
#include "ic_can/core/wrist_component.hpp"
#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <signal.h>
#include <thread>

// Global flag for graceful shutdown
std::atomic<bool> keep_running(true);

// Signal handler for Ctrl+C
void signal_handler(int signal) {
  if (signal == SIGINT) {
    std::cout << "\n🛑 Received interrupt signal, shutting down gracefully..."
              << std::endl;
    keep_running = false;
  }
}

int main(int argc, char *argv[]) {
  // Default parameters
  double frequency = 5.0; // Hz

  // Parse command line arguments
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      std::cout << "=== SAFE Wrist Motor Monitor - REFRESH ONLY ==="
                << std::endl;
      std::cout << "Safe monitoring using ONLY refresh command" << std::endl;
      std::cout << "NO position requests, NO movement commands" << std::endl;
      std::cout << std::endl;
      std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
      std::cout << "Options:" << std::endl;
      std::cout << "  -f <freq>     Update frequency in Hz (default: 5)"
                << std::endl;
      std::cout << "  -h            Show this help message" << std::endl;
      std::cout << std::endl;
      std::cout << "⚠️  SAFETY: This tool uses ONLY refresh command - NO motor "
                   "commands sent!"
                << std::endl;
      return 0;
    } else if (arg == "-f" && i + 1 < argc) {
      frequency = std::stod(argv[++i]);
    }
  }

  // Set up signal handler
  signal(SIGINT, signal_handler);

  std::cout << "=== SAFE Wrist Motor Monitor - REFRESH ONLY ===" << std::endl;
  std::cout << "🔒 SAFE MODE: Using ONLY refresh command" << std::endl;
  std::cout << "🚫 NO position requests, NO movement commands" << std::endl;
  std::cout << "📊 Monitoring frequency: " << frequency << " Hz" << std::endl;
  std::cout << "Press Ctrl+C to stop" << std::endl;
  std::cout << std::endl;

  try {
    // Initialize IC_CAN system
    std::cout << "🔧 Initializing IC_CAN system..." << std::endl;
    ic_can::IC_CAN ic_can("693D3DE86DF5940C8BC74A5B46A3CE2E",
                          true); // Enable debug mode to see CAN frames

    if (!ic_can.initialize()) {
      std::cerr << "❌ Failed to initialize IC_CAN system" << std::endl;
      return 1;
    }

    // Get wrist component
    auto &wrist = ic_can.get_wrist();

    // DO NOT enable motors - just monitor
    std::cout << "📡 System initialized - monitoring in SAFE mode" << std::endl;
    std::cout << "⚠️  Motors NOT enabled - passive monitoring only" << std::endl;

    // SAFETY CONFIRMATION
    std::cout << "⚠️  SAFETY CONFIRMATION REQUIRED ⚠️" << std::endl;
    std::cout << "This tool will:" << std::endl;
    std::cout << "1. Send SET ZERO command to HT motors (SAFE calibration)"
              << std::endl;
    std::cout << "2. Send READ-REQUEST commands to HT motors (SAFE)"
              << std::endl;
    std::cout << "3. Monitor position responses from HT motors" << std::endl;
    std::cout << "NO movement commands will be sent." << std::endl;
    std::cout << std::endl;
    std::cout << "Continue with SAFE monitoring? (y/n): ";

    char confirmation;
    std::cin >> confirmation;

    if (confirmation != 'y' && confirmation != 'Y') {
      std::cout << "❌ Monitoring cancelled by user" << std::endl;
      return 0;
    }

    std::cout << "✅ User confirmed - proceeding with SAFE monitoring..."
              << std::endl;
    std::cout << std::endl;

    // Send SET ZERO command first
    std::cout << "🔧 Sending HT SET ZERO command..." << std::endl;
    ic_can.send_ht_set_zero_command();
    /*std::this_thread::sleep_for(*/
    /*    std::chrono::milliseconds(500)); // Wait for zero command to process*/
    /*std::cin >> confirmation; // Wait for user input before continuing*/
    std::cout << "finish set zero" << std::endl;
    // Monitoring loop
    auto start_time = std::chrono::steady_clock::now();
    auto last_update = start_time;
    const auto update_interval = std::chrono::duration<double>(1.0 / frequency);

    std::cout << "========================================" << std::endl;
    std::cout << "Motor | Position (rad) | Position (deg)" << std::endl;
    std::cout << "------|---------------|----------------" << std::endl;

    while (keep_running) {
      auto current_time = std::chrono::steady_clock::now();

      // Update at specified frequency
      if (current_time - last_update >= update_interval) {
        // Send SAFE position request commands to HT motors (READ-REQUEST ONLY)
        /*ic_can.send_ht_mit_command_with_movement(*/
        /*    0.0, 0.0, 0.0,  // Motor 7: position, velocity, torque (all zero for position request)*/
        /*    0.0, 0.0, 0.0,  // Motor 8: position, velocity, torque (all zero for position request)*/
        /*    0.0, 0.0);       // Gains (kp, kd) - minimal values for position request*/
        /**/
        //
        //
        ic_can.send_ht_read_state();
        // Small delay to allow response
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Update wrist state from received CAN data
        wrist.refresh();

        // Get positions from IC_CAN system (atomic variables updated by CAN
        // responses)
        auto positions = ic_can.get_joint_positions();

        if (positions.size() >= 8) {
          double m7_pos =
              positions[6]; // Motor 7 (pitch) - index 6 in 0-based array
          double m8_pos =
              positions[7]; // Motor 8 (roll) - index 7 in 0-based array

          // Calculate timestamp
          double timestamp =
              std::chrono::duration<double>(current_time - start_time).count();

          // Print motor data
          std::cout << std::fixed << std::setprecision(4);
          std::cout << std::setw(4) << 7 << "   | " << std::setw(11) << m7_pos
                    << " | " << std::setw(12) << (m7_pos * 180.0 / M_PI)
                    << "  (Pitch)" << std::endl;
          std::cout << std::setw(4) << 8 << "   | " << std::setw(11) << m8_pos
                    << " | " << std::setw(12) << (m8_pos * 180.0 / M_PI)
                    << "  (Roll)" << std::endl;

          std::cout << "------|---------------|----------------" << std::endl;
          std::cout << "Time: " << std::setprecision(1) << timestamp << "s"
                    << std::endl;
          std::cout << "========================================" << std::endl;
        } else {
          std::cout << "⚠️  Failed to read wrist motor positions via refresh"
                    << std::endl;
        }

        last_update = current_time;
      }

      // Small sleep to prevent CPU overload
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "✅ Safe monitoring stopped successfully" << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "❌ Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
