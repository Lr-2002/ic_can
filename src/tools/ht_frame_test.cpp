/**
 * @file ht_frame_test.cpp
 * @brief HT Motor Frame Communication Test
 *
 * Test tool for the new 48-byte HT motor frame communication protocol.
 * Demonstrates independent control of HT motors 7 and 8 using structured frames.
 *
 * @author IC_CAN Team
 * @date 2025-10-30
 */

#include "ic_can/core/ic_can.hpp"
#include "ic_can/core/wrist_component.hpp"
#include "ic_can/motors/ht_motor_frame.hpp"
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
  double frequency = 2.0; // Hz

  // Parse command line arguments
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      std::cout << "=== HT Motor Frame Communication Test ===" << std::endl;
      std::cout << "Test tool for new 48-byte HT motor frame protocol" << std::endl;
      std::cout << std::endl;
      std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
      std::cout << "Options:" << std::endl;
      std::cout << "  -f <freq>     Update frequency in Hz (default: 2)" << std::endl;
      std::cout << "  -h            Show this help message" << std::endl;
      std::cout << std::endl;
      std::cout << "📋 Frame Protocol:" << std::endl;
      std::cout << "  - Motor 7: Send ID 0x8007, Receive ID 0x700" << std::endl;
      std::cout << "  - Motor 8: Send ID 0x8008, Receive ID 0x800" << std::endl;
      std::cout << "  - Frame size: 48 bytes with structured data" << std::endl;
      return 0;
    } else if (arg == "-f" && i + 1 < argc) {
      frequency = std::stod(argv[++i]);
    }
  }

  // Set up signal handler
  signal(SIGINT, signal_handler);

  std::cout << "=== HT Motor Frame Communication Test ===" << std::endl;
  std::cout << "🔧 Testing new 48-byte frame protocol" << std::endl;
  std::cout << "📊 Test frequency: " << frequency << " Hz" << std::endl;
  std::cout << "🔗 CAN IDs: M7=0x8007/0x700, M8=0x8008/0x800" << std::endl;
  std::cout << "Press Ctrl+C to stop" << std::endl;
  std::cout << std::endl;

  try {
    // Initialize IC_CAN system
    std::cout << "🔧 Initializing IC_CAN system..." << std::endl;
    ic_can::IC_CAN ic_can("693D3DE86DF5940C8BC74A5B46A3CE2E", true);

    if (!ic_can.initialize()) {
      std::cerr << "❌ Failed to initialize IC_CAN system" << std::endl;
      return 1;
    }

    // Get wrist component
    auto &wrist = ic_can.get_wrist();

    std::cout << "✅ System initialized - starting frame test..." << std::endl;
    std::cout << std::endl;

    // Test frame creation
    std::cout << "🏗️  Testing frame structure..." << std::endl;
    ic_can::HTMotorFrame test_frame;
    test_frame.set_control_params(0.1f, 0.0f, 0.5f, 50.0f, 0.8f);

    std::cout << "   Frame size: " << test_frame.get_size() << " bytes" << std::endl;
    std::cout << "   Frame data: ";
    const uint8_t* frame_data = test_frame.get_data();
    for (size_t i = 0; i < std::min(size_t(16), test_frame.get_size()); ++i) {
      std::cout << std::hex << "0x" << std::setw(2) << std::setfill('0')
                << (int)frame_data[i] << " ";
    }
    std::cout << std::dec << "... (showing first 16 bytes)" << std::endl;
    std::cout << std::endl;

    // Test loop with different poses
    auto start_time = std::chrono::steady_clock::now();
    auto last_update = start_time;
    const auto update_interval = std::chrono::duration<double>(1.0 / frequency);
    double phase = 0.0;

    std::cout << "🚀 Starting frame transmission test..." << std::endl;
    std::cout << "========================================" << std::endl;

    while (keep_running) {
      auto current_time = std::chrono::steady_clock::now();

      // Update at specified frequency
      if (current_time - last_update >= update_interval) {
        // Generate test positions (sinusoidal motion)
        phase += 0.1;
        double m7_pos = 0.2 * std::sin(phase);          // Motor 7: ±0.2 rad
        double m8_pos = 0.15 * std::cos(phase * 0.8);     // Motor 8: ±0.15 rad

        std::cout << "📤 Sending frames at "
                  << std::chrono::duration<double>(current_time - start_time).count()
                  << "s" << std::endl;
        std::cout << "   Motor 7 (pitch): " << std::fixed << std::setprecision(3)
                  << m7_pos << " rad (" << (m7_pos * 180.0 / M_PI) << "°)" << std::endl;
        std::cout << "   Motor 8 (roll):  " << std::fixed << std::setprecision(3)
                  << m8_pos << " rad (" << (m8_pos * 180.0 / M_PI) << "°)" << std::endl;

        // Send frame commands using wrist component
        bool success = wrist.send_ht_frame_both_motors(
            m7_pos, 0.0, 0.0,    // Motor 7: position, velocity, torque
            m8_pos, 0.0, 0.0,    // Motor 8: position, velocity, torque
            30.0, 0.5           // Gains: kp=30.0, kd=0.5
        );

        std::cout << "   Result: " << (success ? "✅ SUCCESS" : "❌ FAILED") << std::endl;
        std::cout << "----------------------------------------" << std::endl;

        last_update = current_time;
      }

      // Small sleep to prevent CPU overload
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "✅ Frame test completed successfully" << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "❌ Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}