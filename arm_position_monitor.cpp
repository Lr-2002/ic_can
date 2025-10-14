/**
 * @file arm_position_monitor.cpp
 * @brief Real-time 6-Joint Arm Position Monitor
 *
 * This program monitors and displays the positions of all 6 arm joints (motors
 * 1-6) in real-time using the working dm-tools SDK integration.
 */

#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

// Use working dm-tools SDK directly
#include "dm-tools/USB2FDCAN/SDK/C++/ubuntu/include/protocol/usb_class.h"

static volatile bool g_running = true;

// Arm joint positions storage
static std::atomic<float> arm_positions[6] = {0.0f, 0.0f, 0.0f,
                                              0.0f, 0.0f, 0.0f};
static std::atomic<bool> position_updated[6] = {false, false, false,
                                                false, false, false};

void signal_handler(int signal) {
  std::cout << "\n⚠️  Received signal " << signal << ", stopping monitor..."
            << std::endl;
  g_running = false;
}

// Arm motor feedback callback - handles all 6 arm joints (motors 1-6)
void arm_feedback_callback(can_value_type &value) {
  uint32_t motor_id = value.head.id;

  std::cout << "received data from callback" << motor_id << std::endl;
  // Only process arm joints (motors 1-6)
  motor_id = motor_id - 16;
  if (motor_id < 1 || motor_id > 6)
    return;

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
  uint16_t dq_uint = (uint16_t(value.data[3]) << 4) | (value.data[4] >> 4);
  uint16_t tau_uint = (uint16_t(value.data[4] & 0xf) << 8) | value.data[5];

  // Determine correct limits for each motor
  float p_max, v_max, t_max;

  switch (motor_id) {
  case 1:
    // DM 10010L
    p_max = 12.5f;
    v_max = 25.0f;
    t_max = 200.0f;
    break;
  case 2:
  case 3:
    // DM 6248
    p_max = 12.566f;
    v_max = 20.0f;
    t_max = 120.0f;
    break;
  case 4:
  case 5:
    // DM 4340
    p_max = 12.5f;
    v_max = 10.0f;
    t_max = 28.0f;
    break;
  case 6:
    // DM 4310
    p_max = 12.5f;
    v_max = 30.0f;
    t_max = 10.0f;
    break;
  default:
    return; // Should not happen
  }

  // Convert to real values
  float position = uint_to_float(q_uint, -p_max, p_max, 16);
  float velocity = uint_to_float(dq_uint, -v_max, v_max, 12);
  float torque = uint_to_float(tau_uint, -t_max, t_max, 12);

  // Update atomic position storage
  arm_positions[motor_id - 1].store(position);
  position_updated[motor_id - 1].store(true);

  // Optional: Print detailed feedback (comment out for cleaner display)
  std::cout << "📥 Motor " << motor_id << ": p=" << std::fixed
            << std::setprecision(3) << position << " rad ("
            << (position * 180.0 / M_PI) << "°), "
            << "v=" << velocity << " rad/s, τ=" << torque << " Nm" << std::endl;
}

void print_arm_positions() {
  // Clear screen (works on Linux/Mac)
  std::cout << "\033[2J\033[H";

  std::cout << "=== 6-Joint Arm Position Monitor ===" << std::endl;
  std::cout << "Real-time position monitoring for motors 1-6" << std::endl;
  std::cout << "Press Ctrl+C to stop" << std::endl;
  std::cout << std::endl;

  // Print header
  std::cout << "Joint | Motor | Position (rad) | Position (deg) | Updated"
            << std::endl;
  std::cout << "------|-------|----------------|----------------|--------"
            << std::endl;

  // Print each joint
  for (int i = 0; i < 6; i++) {
    float pos_rad = arm_positions[i].load();
    float pos_deg = pos_rad * 180.0f / M_PI;
    bool updated = position_updated[i].load();

    std::cout << std::setw(5) << (i + 1) << " | " << std::setw(5) << "M"
              << (i + 1) << " | " << std::fixed << std::setw(14)
              << std::setprecision(3) << pos_rad << " | " << std::fixed
              << std::setw(14) << std::setprecision(1) << pos_deg << " | "
              << (updated ? "✅" : "⏸️") << std::endl;
  }

  std::cout << std::endl;

  // Print arm configuration info
  std::cout << "Arm Configuration:" << std::endl;
  std::cout << "  Joint 1 (M1): DM10010L - ±12.5 rad, 25 rad/s, 200 Nm"
            << std::endl;
  std::cout << "  Joint 2 (M2): DM6248   - ±12.566 rad, 20 rad/s, 120 Nm"
            << std::endl;
  std::cout << "  Joint 3 (M3): DM6248   - ±12.566 rad, 20 rad/s, 120 Nm"
            << std::endl;
  std::cout << "  Joint 4 (M4): DM4340   - ±12.5 rad, 10 rad/s, 28 Nm"
            << std::endl;
  std::cout << "  Joint 5 (M5): DM4340   - ±12.5 rad, 10 rad/s, 28 Nm"
            << std::endl;
  std::cout << "  Joint 6 (M6): DM4310   - ±12.5 rad, 30 rad/s, 10 Nm"
            << std::endl;

  auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now().time_since_epoch())
                       .count();
  std::cout << "\nLast update: " << timestamp << " ms" << std::endl;
}

void enable_arm_motors(usb_class *device) {
  std::cout << "🔧 Enabling all 6 arm motors..." << std::endl;

  // Enable motors 1-6
  std::vector<uint8_t> enable_cmd = {0xFF, 0xFF, 0xFF, 0xFF,
                                     0xFF, 0xFF, 0xFF, 0xFC};

  for (int motor_id = 1; motor_id <= 6; motor_id++) {
    device->fdcanFrameSend(enable_cmd, motor_id);
    std::cout << "   ✅ Motor " << motor_id << " enabled" << std::endl;
    usleep(100000); // 100ms between enables
  }

  std::cout << "✅ All arm motors enabled" << std::endl;
  usleep(500000); // Wait 500ms for motors to initialize
}

void request_status_all(usb_class *device) {
  // Send status request to trigger feedback from all motors
  for (int motor_id = 1; motor_id <= 6; motor_id++) {
    std::cout << " sending command to " << motor_id << std::endl;
    std::vector<uint8_t> status_cmd = {std::uint8_t(motor_id), 0x00, 0xCC,
                                       0x00};
    device->fdcanFrameSend(status_cmd, 0x7FF); // Broadcast status request
  }
}

int main() {
  std::cout << "=== 6-Joint Arm Position Monitor ===" << std::endl;
  std::cout << "Starting real-time position monitoring for IC_ARM" << std::endl;

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
    enable_arm_motors(device);

    // Initial status request to get current positions
    std::cout << "\n📊 Requesting initial status..." << std::endl;
    request_status_all(device);

    // Main monitoring loop
    std::cout << "\n🎯 Starting position monitoring..." << std::endl;
    std::cout << "Updating display every 100ms" << std::endl;

    auto last_update = std::chrono::steady_clock::now();

    while (g_running) {
      auto current_time = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                         current_time - last_update)
                         .count();

      // Update display every 100ms
      if (elapsed >= 100) {
        print_arm_positions();
        last_update = current_time;

        // Periodically request status to ensure continuous updates
        static int request_counter = 0;
        if (++request_counter >= 10) { // Every 1 second
          request_status_all(device);
          request_counter = 0;
        }
      }

      // Sleep for short duration to avoid high CPU usage
      usleep(10000); // 10ms
    }

    // Cleanup
    std::cout << "\n🧹 Cleaning up..." << std::endl;
    device->USB_CMD_STOP_CAP();
    delete device;

    std::cout << "\n🎉 ARM POSITION MONITOR STOPPED" << std::endl;
    std::cout << "✅ Successfully monitored 6-joint arm positions" << std::endl;

    return 0;

  } catch (const std::exception &e) {
    std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
    return -1;
  }
}
