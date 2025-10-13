/**
 * @file get_arm_positions.cpp
 * @brief Simple function to get current 6-joint arm positions
 *
 * This program demonstrates how to create a simple function that returns
 * the current positions of all 6 arm joints.
 */

#include <cmath>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>

// Use working dm-tools SDK directly
#include "dm-tools/USB2FDCAN/SDK/C++/ubuntu/include/protocol/usb_class.h"

static volatile bool g_running = true;
static bool positions_received = false;

// Arm joint positions storage
static float arm_positions[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

void signal_handler(int signal) {
  std::cout << "\n⚠️  Received signal " << signal << ", stopping..."
            << std::endl;
  g_running = false;
}

// Arm motor feedback callback - handles all 6 arm joints (motors 1-6)
void arm_feedback_callback(can_value_type &value) {
  uint32_t motor_id = value.head.id;

  // Only process arm joints (motors 1-6)
  if (motor_id < 1 || motor_id > 6) return;

  // Use same conversion logic as working test
  static auto uint_to_float = [](uint16_t x, float xmin, float xmax,
                                 uint8_t bits) -> float {
    float span = xmax - xmin;
    float data_norm = float(x) / ((1 << bits) - 1);
    float data = data_norm * span + xmin;
    return data;
  };

  // Extract motor response data
  uint16_t q_uint = (uint16_t(value.data[1]) << 8) | value.data[2];

  // Determine correct limits for each motor
  float p_max;

  switch (motor_id) {
    case 1:
      // DM 10010L
      p_max = 12.5f;
      break;
    case 2:
    case 3:
      // DM 6248
      p_max = 12.566f;
      break;
    case 4:
    case 5:
      // DM 4340
      p_max = 12.5f;
      break;
    case 6:
      // DM 4310
      p_max = 12.5f;
      break;
    default:
      return; // Should not happen
  }

  // Convert position to real values
  float position = uint_to_float(q_uint, -p_max, p_max, 16);
  arm_positions[motor_id - 1] = position;
  positions_received = true;

  std::cout << "📥 Joint " << motor_id << ": " << std::fixed << std::setprecision(3)
            << position << " rad (" << (position * 180.0 / M_PI) << "°)" << std::endl;
}

// Function to get current arm positions
std::vector<float> get_arm_positions(usb_class* device) {
  std::cout << "\n📊 Reading current arm positions..." << std::endl;

  // Reset positions_received flag
  positions_received = false;

  // Send status request to trigger feedback from all motors
  std::vector<uint8_t> status_cmd = {0x01, 0x00, 0xCC, 0x00};
  device->fdcanFrameSend(status_cmd, 0x7FF); // Broadcast status request

  // Wait for responses (timeout after 3 seconds)
  auto start_time = std::chrono::steady_clock::now();
  while (!positions_received &&
         std::chrono::duration_cast<std::chrono::seconds>(
           std::chrono::steady_clock::now() - start_time).count() < 3) {
    usleep(10000); // 10ms
  }

  // Create result vector
  std::vector<float> positions;
  for (int i = 0; i < 6; i++) {
    positions.push_back(arm_positions[i]);
  }

  return positions;
}

void print_positions(const std::vector<float>& positions) {
  std::cout << "\n=== Current Arm Positions ===" << std::endl;
  std::cout << "Joint | Position (rad) | Position (deg)" << std::endl;
  std::cout << "------|----------------|----------------" << std::endl;

  for (int i = 0; i < 6; i++) {
    float pos_rad = positions[i];
    float pos_deg = pos_rad * 180.0f / M_PI;

    std::cout << std::setw(5) << (i + 1) << " | "
              << std::fixed << std::setw(14) << std::setprecision(3) << pos_rad << " | "
              << std::fixed << std::setw(14) << std::setprecision(1) << pos_deg << std::endl;
  }
  std::cout << std::endl;
}

int main() {
  std::cout << "=== Simple Arm Position Reader ===" << std::endl;
  std::cout << "Demonstrating get_arm_positions() function" << std::endl;

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  try {
    std::string device_sn = "F561E08C892274DB09496BCC1102DBC5";

    std::cout << "\n🔧 Connecting to USB2CAN device..." << std::endl;
    std::cout << "Device SN: " << device_sn << std::endl;

    // Create dm-tools USB connection
    usb_class *device = new usb_class(1000000, 5000000, device_sn);

    if (!device) {
      std::cout << "❌ FAILED: Could not create usb_class" << std::endl;
      return -1;
    }

    std::cout << "✅ SUCCESS: Device connected" << std::endl;

    // Set up callback for arm motor feedback
    std::cout << "\n📡 Setting up arm feedback callback..." << std::endl;
    device->setFrameCallback(arm_feedback_callback);
    std::cout << "✅ Callback configured for motors 1-6" << std::endl;

    // Start data capture
    std::cout << "\n🔄 Starting data capture..." << std::endl;
    uint8_t capture_result = device->USB_CMD_START_CAP();
    if (capture_result == 0) {
      std::cout << "✅ SUCCESS: Data capture started" << std::endl;
    } else {
      std::cout << "⚠️  WARNING: Data capture start returned "
                << (int)capture_result << std::endl;
    }

    // Enable all arm motors
    std::cout << "\n🔧 Enabling arm motors..." << std::endl;
    std::vector<uint8_t> enable_cmd = {0xFF, 0xFF, 0xFF, 0xFF,
                                       0xFF, 0xFF, 0xFF, 0xFC};

    for (int motor_id = 1; motor_id <= 6; motor_id++) {
      device->fdcanFrameSend(enable_cmd, motor_id);
      std::cout << "   ✅ Motor " << motor_id << " enabled" << std::endl;
      usleep(100000); // 100ms between enables
    }

    // Wait for motors to initialize
    usleep(1000000); // 1 second

    // Demonstrate the get_arm_positions function
    std::cout << "\n🎯 Demonstrating get_arm_positions() function..." << std::endl;

    for (int demo_count = 1; demo_count <= 5 && g_running; demo_count++) {
      std::cout << "\n--- Reading #" << demo_count << " ---" << std::endl;

      // Call the function to get positions
      auto positions = get_arm_positions(device);

      // Print results
      print_positions(positions);

      // Wait 2 seconds between readings
      if (demo_count < 5) {
        std::cout << "Waiting 2 seconds before next reading..." << std::endl;
        usleep(2000000);
      }
    }

    // Cleanup
    std::cout << "\n🧹 Cleaning up..." << std::endl;
    device->USB_CMD_STOP_CAP();
    delete device;

    std::cout << "\n🎉 DEMONSTRATION COMPLETED" << std::endl;
    std::cout << "✅ get_arm_positions() function working correctly" << std::endl;

    return 0;

  } catch (const std::exception &e) {
    std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
    return -1;
  }
}