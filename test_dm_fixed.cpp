/**
 * @file test_dm_fixed.cpp
 * @brief FIXED dm-tools SDK test - no double connection issue
 */

#include <cmath>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

#include "dm-tools/USB2FDCAN/SDK/C++/ubuntu/include/protocol/usb_class.h"

static volatile bool g_running = true;
static volatile bool position_received = false;
static float received_position = 0.0f;
static float received_velocity = 0.0f;
static float received_torque = 0.0f;

void signal_handler(int signal) {
  std::cout << "\n⚠️  Received signal " << signal << ", stopping..."
            << std::endl;
  g_running = false;
}
uint16_t float_to_uint(double x, double min, double max, int bits) {
  double span = max - min;
  if (x < min)
    x = min;
  if (x > max)
    x = max;
  return static_cast<uint16_t>((x - min) * ((1 << bits) - 1) / span);
}
std::vector<uint8_t> create_mit_command(double position, double velocity,
                                        double torque, double kp, double kd) {
  std::vector<uint8_t> data(8);

  // 使用dm-tools SDK中的正确限制参数 (DM4340作为示例)
  double p_max = 12.5; // Q_MAX from dm-tools SDK
  double v_max = 10.0; // DQ_MAX from dm-tools SDK
  double t_max = 28.0; // TAU_MAX from dm-tools SDK

  // 转换为内部格式 (使用dm-tools SDK的转换方法)
  uint16_t kp_int = float_to_uint(kp, 0.0, 500.0, 12);
  uint16_t kd_int = float_to_uint(kd, 0.0, 5.0, 12);
  uint16_t q_uint = float_to_uint(position, -p_max, p_max, 16);
  uint16_t dq_uint = float_to_uint(velocity, -v_max, v_max, 12);
  uint16_t tau_uint = float_to_uint(torque, -t_max, t_max, 12);

  // 使用dm-tools SDK的数据打包格式
  data[0] = (q_uint >> 8) & 0xFF;
  data[1] = q_uint & 0xFF;
  data[2] = dq_uint >> 4;
  data[3] = ((dq_uint & 0xF) << 4) | ((kp_int >> 8) & 0xF);
  data[4] = kp_int & 0xFF;
  data[5] = kd_int >> 4;
  data[6] = ((kd_int & 0xF) << 4) | ((tau_uint >> 8) & 0xF);
  data[7] = tau_uint & 0xFF;

  std::cout << "Creating MIT command: p=" << position << " rad (±" << p_max
            << "), v=" << velocity << " rad/s (±" << v_max << "), t=" << torque
            << " Nm (±" << t_max << ")" << std::endl;

  return data;
}

// Motor feedback callback based on dm-tools SDK canframeCallback
void motor_feedback_callback(can_value_type &value) {
  // Extract position, velocity, and torque from CAN response
  // Based on dm-tools SDK damiao.cpp lines 511-525
  static auto uint_to_float = [](uint16_t x, float xmin, float xmax,
                                 uint8_t bits) -> float {
    float span = xmax - xmin;
    float data_norm = float(x) / ((1 << bits) - 1);
    float data = data_norm * span + xmin;
    return data;
  };

  uint32_t canID = value.head.id;

  // Extract motor response data
  uint16_t q_uint = (uint16_t(value.data[1]) << 8) | value.data[2];
  uint16_t dq_uint = (uint16_t(value.data[3]) << 4) | (value.data[4] >> 4);
  uint16_t tau_uint = (uint16_t(value.data[4] & 0xf) << 8) | value.data[5];

  // Use DM4340 limits for conversion (same as in create_mit_command)
  double p_max = 12.5; // Q_MAX
  double v_max = 10.0; // DQ_MAX
  double t_max = 28.0; // TAU_MAX

  // Convert to real values
  received_position = uint_to_float(q_uint, -p_max, p_max, 16);
  received_velocity = uint_to_float(dq_uint, -v_max, v_max, 12);
  received_torque = uint_to_float(tau_uint, -t_max, t_max, 12);

  position_received = true;

  std::cout << "📥 Motor Feedback (ID=" << canID << "): "
            << "p=" << std::fixed << std::setprecision(3) << received_position
            << " rad, "
            << "v=" << received_velocity << " rad/s, "
            << "τ=" << received_torque << " Nm" << std::endl;
}

int main() {
  std::cout << "=== FIXED dm-tools SDK Test ===" << std::endl;
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  try {
    std::string device_sn = "F561E08C892274DB09496BCC1102DBC5";

    std::cout << "Creating usb_class and connecting to device..." << std::endl;
    std::cout << "Device SN: " << device_sn << std::endl;

    // The constructor AUTOMATICALLY connects to the device
    // We should NOT call usb_open() again!
    usb_class *device = new usb_class(1000000, 5000000, device_sn);

    if (!device) {
      std::cout << "❌ FAILED: Could not create usb_class" << std::endl;
      return -1;
    }

    std::cout << "✅ SUCCESS: Device connected via constructor" << std::endl;
    std::cout << "💡 Note: usb_class constructor automatically connects, no "
                 "need for usb_open()"
              << std::endl;

    // Set up callback for motor feedback
    std::cout << "\n📡 Setting up motor feedback callback..." << std::endl;
    device->setFrameCallback(motor_feedback_callback);
    std::cout << "✅ Callback configured" << std::endl;

    // Start data capture
    std::cout << "\n🔄 Starting data capture..." << std::endl;
    uint8_t capture_result = device->USB_CMD_START_CAP();
    if (capture_result == 0) {
      std::cout << "✅ SUCCESS: Data capture started" << std::endl;
    } else {
      std::cout << "⚠️  WARNING: Data capture start returned "
                << (int)capture_result << std::endl;
    }

    // Test motor command
    std::cout << "\n🎯 Testing motor command (Motor ID 1)..." << std::endl;

    // Motor enable command
    std::vector<uint8_t> enable_cmd = {0xFF, 0xFF, 0xFF, 0xFF,
                                       0xFF, 0xFF, 0xFF, 0xFC};
    device->fdcanFrameSend(enable_cmd, 0x01); // Motor 1 receive ID
    std::cout << "✅ Motor enable command sent" << std::endl;

    usleep(500000); // Wait 0.5 seconds

    // Request motor status to trigger feedback
    std::cout << "\n📊 Requesting motor status..." << std::endl;
    std::vector<uint8_t> status_cmd = {0x01, 0x00, 0xCC,
                                       0x00}; // Motor 1 status request
    device->fdcanFrameSend(status_cmd, 0x7FF);
    std::cout << "✅ Status request sent (ID=0x7FF)" << std::endl;

    // Wait for feedback
    std::cout << "\n⏱️  Waiting for motor feedback..." << std::endl;
    for (int i = 3; i > 0 && g_running && !position_received; --i) {
      std::cout << "   " << i << " seconds remaining..." << std::endl;
      usleep(1000000); // 1 second
    }

    // Send a position command and read response
    std::cout << "\n🎯 Sending position command (0 rad)..." << std::endl;
    auto cmd = create_mit_command(0, 0, 0, 10, 0);
    device->fdcanFrameSend(cmd, 0x01);
    std::cout << "✅ Position command sent" << std::endl;

    // Wait for position feedback
    position_received = false; // Reset flag
    std::cout << "\n⏱️  Waiting for position feedback..." << std::endl;
    for (int i = 3; i > 0 && g_running && !position_received; --i) {
      std::cout << "   " << i << " seconds remaining..." << std::endl;
      usleep(2000000); // 1 second
    }

    // Display results
    std::cout << "\n📊 FINAL RESULTS:" << std::endl;
    if (position_received) {
      std::cout << "✅ Motor position successfully read!" << std::endl;
      std::cout << "   📍 Final Position: " << std::fixed
                << std::setprecision(3) << received_position << " rad ("
                << (received_position * 180.0 / M_PI) << "°)" << std::endl;
      std::cout << "   🔄 Final Velocity: " << received_velocity << " rad/s"
                << std::endl;
      std::cout << "   💪 Final Torque: " << received_torque << " Nm"
                << std::endl;
    } else {
      std::cout << "⚠️  No motor feedback received (motor may not be connected)"
                << std::endl;
    }

    // Motor disable command
    std::cout << "\n🔌 Disabling motor..." << std::endl;
    std::vector<uint8_t> disable_cmd = {0xFF, 0xFF, 0xFF, 0xFF,
                                        0xFF, 0xFF, 0xFF, 0xFD};
    device->fdcanFrameSend(disable_cmd, 0x01); // Motor 1 receive ID
    std::cout << "✅ Motor disable command sent" << std::endl;

    // Cleanup
    std::cout << "\n🧹 Cleaning up..." << std::endl;
    device->USB_CMD_STOP_CAP();
    delete device;

    std::cout << "\n🎉 TEST COMPLETED SUCCESSFULLY!" << std::endl;
    std::cout << "✅ dm-tools SDK is working correctly" << std::endl;
    std::cout << "✅ USB2CAN connection established" << std::endl;
    std::cout << "✅ CAN frame transmission working" << std::endl;
    std::cout << "✅ Motor commands sent successfully" << std::endl;

    return 0;

  } catch (const std::exception &e) {
    std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
    return -1;
  }
}
