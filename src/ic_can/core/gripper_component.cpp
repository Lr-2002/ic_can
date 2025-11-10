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

#include "ic_can/core/gripper_component.hpp"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <thread>

// For USB serial communication
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace ic_can {

// USB Servo Protocol constants
static constexpr uint8_t SERVO_HEADER[] = {0xFF, 0xFF, 0x01};
static constexpr uint8_t SERVO_ENABLE_CMD[] = {0x04, 0x03, 0x28, 0x01};
static constexpr uint8_t SERVO_DISABLE_CMD[] = {0x04, 0x03, 0x28, 0x00};
static constexpr uint8_t SERVO_READ_POS_CMD[] = {0x04, 0x02, 0x38, 0x02};
static constexpr uint8_t SERVO_POS_CMD_HEADER[] = {0x09, 0x03, 0x2A};

// Position range constants
static constexpr uint16_t POSITION_MIN = 1000;
static constexpr uint16_t POSITION_MAX = 2100;
static constexpr uint16_t POSITION_MID =
    (POSITION_MIN + POSITION_MAX) / 2; // 1550

// Speed range constants
static constexpr uint8_t SPEED_MIN = 0;
static constexpr uint8_t SPEED_MAX = 100;
static constexpr uint8_t SPEED_DEFAULT = 50;

GripperComponent::GripperComponent()
    : MotorProtocolBase("GripperUSB", 0x19, 0x19) {
  debug_print("GripperComponent initialized with USB servo protocol");

  // Initialize USB communication for servo
  usb_fd_ = -1;
  usb_port_ = "/dev/ttyUSB0";

  // Set default state
  current_openness_.store(0.5);
  is_moving_.store(false);
  is_grasping_.store(false);

  std::cout << "   USB Protocol: FEETECH Servo" << std::endl;
  std::cout << "   Position Range: " << POSITION_MIN << "-" << POSITION_MAX
            << std::endl;
  std::cout << "   Speed Range: " << SPEED_MIN << "-" << SPEED_MAX << std::endl;
}

// ========== CANProtocolInterface Implementation ==========

bool GripperComponent::process_can_frame(const CANFrame &frame) {
  // For USB servo, we don't process CAN frames in the traditional way
  // The servo responses are handled via USB communication
  debug_print("GripperComponent: USB servo does not process CAN frames");
  return false;
}

// USB Communication Helper Methods
bool GripperComponent::usb_connect() {
  if (usb_fd_ >= 0) {
    ::close(usb_fd_);
  }

  usb_fd_ = ::open(usb_port_.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
  if (usb_fd_ < 0) {
    debug_print("Failed to open USB port: " + usb_port_);
    return false;
  }

  // Configure serial port
  struct termios options;
  tcgetattr(usb_fd_, &options);

  // Set baud rate to 115200
  cfsetispeed(&options, B115200);
  cfsetospeed(&options, B115200);

  // 8N1 configuration
  options.c_cflag &= ~PARENB; // No parity
  options.c_cflag &= ~CSTOPB; // 1 stop bit
  options.c_cflag &= ~CSIZE;  // Clear data bits
  options.c_cflag |= CS8;     // 8 data bits

  // Enable receiver, ignore control lines
  options.c_cflag |= CREAD | CLOCAL;

  // Raw input mode
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

  // Raw output mode
  options.c_oflag &= ~OPOST;

  // Disable software flow control
  options.c_iflag &= ~(IXON | IXOFF | IXANY);

  // Set timeout
  options.c_cc[VMIN] = 0;
  options.c_cc[VTIME] = 10; // 1 second timeout

  tcsetattr(usb_fd_, TCSANOW, &options);

  debug_print("USB servo connected on port: " + usb_port_);
  return true;
}

bool GripperComponent::usb_disconnect() {
  if (usb_fd_ >= 0) {
    ::close(usb_fd_);
    usb_fd_ = -1;
    debug_print("USB servo disconnected");
  }
  return true;
}

bool GripperComponent::usb_send_command(const std::vector<uint8_t> &command) {
  if (usb_fd_ < 0) {
    debug_print("USB not connected");
    return false;
  }

  // Calculate checksum using correct formula: (SUM(data) & 0xFF) ^ 0xFF
  uint32_t sum = 0;
  for (size_t i = 2; i < command.size(); ++i) {
    sum += command[i];
  }
  uint8_t checksum = (sum & 0xFF) ^ 0xFF;

  // Create full command with checksum
  std::vector<uint8_t> full_command = command;
  full_command.push_back(checksum);

  // Send command
  ssize_t bytes_written =
      write(usb_fd_, full_command.data(), full_command.size());
  if (bytes_written != static_cast<ssize_t>(full_command.size())) {
    debug_print("Failed to send USB command");
    return false;
  }

  // Log command
  std::stringstream ss;
  ss << "USB SENT: ";
  for (uint8_t byte : full_command) {
    ss << std::hex << std::setw(2) << std::setfill('0')
       << static_cast<int>(byte) << " ";
  }
  debug_print(ss.str());

  return true;
}

bool GripperComponent::usb_read_response(std::vector<uint8_t> &response,
                                         size_t expected_size) {
  if (usb_fd_ < 0) {
    return false;
  }

  response.clear();
  response.resize(expected_size);

  ssize_t bytes_read = read(usb_fd_, response.data(), expected_size);
  if (bytes_read <= 0) {
    debug_print("No USB response received");
    return false;
  }

  response.resize(bytes_read);

  // Log response
  std::stringstream ss;
  ss << "USB RECV: ";
  for (uint8_t byte : response) {
    ss << std::hex << std::setw(2) << std::setfill('0')
       << static_cast<int>(byte) << " ";
  }
  debug_print(ss.str());

  return true;
}

// Servo Protocol Methods
bool GripperComponent::servo_enable_torque() {
  std::vector<uint8_t> command = {0xFF, 0xFF, 0x01, 0x04, 0x03, 0x28, 0x01};
  return usb_send_command(command);
}

bool GripperComponent::servo_disable_torque() {
  std::vector<uint8_t> command = {0xFF, 0xFF, 0x01, 0x04, 0x03, 0x28, 0x00};
  return usb_send_command(command);
}

bool GripperComponent::servo_position_control(uint16_t position,
                                              uint16_t velocity) {
  // Validate position range
  position = std::clamp(position, POSITION_MIN, POSITION_MAX);
  velocity = std::clamp(velocity, static_cast<uint16_t>(SPEED_MIN),
                        static_cast<uint16_t>(SPEED_MAX));

  std::vector<uint8_t> command = {
      0xFF,
      0xFF,
      0x01,
      0x09,
      0x03,
      0x2A,
      static_cast<uint8_t>(position & 0xFF),        // Position low byte
      static_cast<uint8_t>((position >> 8) & 0xFF), // Position high byte
      0x00,
      0x00,                                        // Time (reserved)
      static_cast<uint8_t>(velocity & 0xFF),       // Velocity low byte
      static_cast<uint8_t>((velocity >> 8) & 0xFF) // Velocity high byte
  };

  return usb_send_command(command);
}

uint16_t GripperComponent::servo_read_position() {
  std::vector<uint8_t> command = {0xFF, 0xFF, 0x01, 0x04, 0x02, 0x38, 0x02};

  if (!usb_send_command(command)) {
    return 0;
  }

  // Wait for response
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::vector<uint8_t> response;

  if (!usb_read_response(response, 8)) {
    return 0;
  }
  std::cout << " read feed back from usb ";
  // Parse position from response - try different byte combinations
  if (response.size() >= 8 && response[0] == 0xFF && response[1] == 0xFF &&
      response[2] == 0x01) {
    // Try bytes 4-5 first (fe 06 = 0x06fe = 1790)
    uint16_t position1 = static_cast<uint16_t>(response[4] | (response[5] << 8));
    // Try bytes 5-6 (06 f6 = 0xf606 = invalid)
    uint16_t position2 = static_cast<uint16_t>(response[5] | (response[6] << 8));
    // Try bytes 6-7 (f6 from previous responses, but current has no 7th byte)

    uint16_t position = position1; // Use position1 as default

    // Check if position is in reasonable range
    if (position >= 1000 && position <= 2100) {
      return position;
    } else if (position2 >= 1000 && position2 <= 2100) {
      return position2;
    } else {
      // Return the position even if out of range for debugging
      return position;
    }
  }

  return 0;
}

// ========== Gripper Control Interface Implementation ==========

bool GripperComponent::open(double speed, double force) {
  debug_print("Opening gripper - speed: " + std::to_string(speed) +
              ", force: " + std::to_string(force));

  if (!usb_connect()) {
    debug_print("Failed to connect USB servo");
    return false;
  }

  // Enable servo torque
  if (!servo_enable_torque()) {
    debug_print("Failed to enable servo torque");
    return false;
  }

  // Convert speed (0.0-1.0) to servo velocity (0-100)
  uint8_t velocity = static_cast<uint8_t>(speed * SPEED_MAX);
  velocity = std::clamp(velocity, SPEED_MIN, SPEED_MAX);

  // Open gripper (move to max position)
  uint16_t target_position = POSITION_MAX;
  if (!servo_position_control(target_position, velocity)) {
    debug_print("Failed to send position command");
    return false;
  }

  // Update state
  current_openness_.store(1.0);
  is_moving_.store(true);

  debug_print("Gripper open command sent");

  // Read feedback after movement
  std::this_thread::sleep_for(std::chrono::milliseconds(800));
  update_gripper_state();

  return true;
}

bool GripperComponent::close(double speed, double force) {
  debug_print("Closing gripper - speed: " + std::to_string(speed) +
              ", force: " + std::to_string(force));

  if (usb_fd_ < 0) {
    if (!usb_connect()) {
      debug_print("Failed to connect USB servo");
      return false;
    }
  }

  // Enable servo torque
  if (!servo_enable_torque()) {
    debug_print("Failed to enable servo torque");
    return false;
  }

  // Convert speed (0.0-1.0) to servo velocity (0-100)
  uint8_t velocity = static_cast<uint8_t>(speed * SPEED_MAX);
  velocity = std::clamp(velocity, SPEED_MIN, SPEED_MAX);

  // Close gripper (move to min position)
  uint16_t target_position = POSITION_MIN;
  if (!servo_position_control(target_position, velocity)) {
    debug_print("Failed to send position command");
    return false;
  }

  // Update state
  current_openness_.store(0.0);
  is_moving_.store(true);
  is_grasping_.store(true);

  debug_print("Gripper close command sent");

  // Read feedback after movement
  std::this_thread::sleep_for(std::chrono::milliseconds(800));
  update_gripper_state();

  return true;
}

bool GripperComponent::stop() {
  debug_print("Stopping gripper");

  // Disable servo torque
  if (usb_fd_ >= 0) {
    servo_disable_torque();
  }

  // Update state
  is_moving_.store(false);
  is_grasping_.store(false);

  debug_print("Gripper stopped");
  return true;
}

bool GripperComponent::set_openness(double openness, double speed,
                                    double force) {
  debug_print("Setting openness: " + std::to_string(openness));

  openness = std::clamp(openness, 0.0, 1.0);

  if (usb_fd_ < 0) {
    if (!usb_connect()) {
      debug_print("Failed to connect USB servo");
      return false;
    }
  }

  // Enable servo torque
  if (!servo_enable_torque()) {
    debug_print("Failed to enable servo torque");
    return false;
  }

  // Convert openness to position
  uint16_t position = static_cast<uint16_t>(
      POSITION_MIN + openness * (POSITION_MAX - POSITION_MIN));
  position = std::clamp(position, POSITION_MIN, POSITION_MAX);

  // Convert speed (0.0-1.0) to servo velocity (0-100)
  uint16_t velocity = static_cast<uint16_t>(speed * SPEED_MAX);
  // Send position command
  if (!servo_position_control(position, velocity)) {
    debug_print("Failed to send position command");
    return false;
  }

  // Update state
  current_openness_.store(openness);
  is_moving_.store(true);

  debug_print("Openness command sent - position: " + std::to_string(position) +
              ", velocity: " + std::to_string(velocity));

  // Read feedback after movement
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  update_gripper_state();

  return true;
}

double GripperComponent::get_openness() const {
  return current_openness_.load();
}

bool GripperComponent::set_servo_angle(double angle, double speed) {
  debug_print("Setting servo angle: " + std::to_string(angle) + " degrees");

  // Clamp angle to servo range (0-180 degrees)
  angle = std::clamp(angle, 0.0, 180.0);

  // Convert angle to openness
  double openness = angle / 180.0;
  return set_openness(openness, speed);
}

double GripperComponent::get_servo_angle() const {
  // Convert openness to angle
  return current_openness_.load() * 180.0;
}

bool GripperComponent::set_servo_pulse_width(uint16_t pulse_width_us) {
  debug_print("Setting servo pulse width: " + std::to_string(pulse_width_us) +
              " us");

  // Convert pulse width to position (1000-2000us maps to 1000-2100 position
  // range)
  double normalized = (pulse_width_us - 1000.0) / 1000.0; // 0.0-1.0
  normalized = std::clamp(normalized, 0.0, 1.0);

  uint16_t position = static_cast<uint16_t>(
      POSITION_MIN + normalized * (POSITION_MAX - POSITION_MIN));

  if (usb_fd_ < 0) {
    if (!usb_connect()) {
      debug_print("Failed to connect USB servo");
      return false;
    }
  }

  return servo_position_control(position, SPEED_DEFAULT);
}

std::string GripperComponent::get_state_summary() const {
  std::stringstream ss;
  ss << "Gripper State:\n";
  ss << "  Openness: " << std::fixed << std::setprecision(3)
     << current_openness_.load() << "\n";
  ss << "  Angle: " << std::fixed << std::setprecision(1) << get_servo_angle()
     << " degrees\n";
  ss << "  Moving: " << (is_moving_.load() ? "Yes" : "No") << "\n";
  ss << "  Grasping: " << (is_grasping_.load() ? "Yes" : "No") << "\n";
  ss << "  USB Connected: " << (usb_fd_ >= 0 ? "Yes" : "No") << "\n";
  return ss.str();
}

// ========== Helper Methods Implementation ==========

bool GripperComponent::is_valid_motor_id(int motor_id) const {
  // For USB servo, we use motor_id 9 but handle it via USB
  return motor_id == 9;
}

double GripperComponent::clamp_angle(double angle, double min_angle,
                                     double max_angle) const {
  return std::clamp(angle, min_angle, max_angle);
}

double GripperComponent::clamp_velocity(double velocity) const {
  return std::clamp(velocity, 0.0, 1.0);
}

// ========== Private Methods Implementation ==========

double GripperComponent::openness_to_angle(double openness) {
  return openness * 180.0;
}

double GripperComponent::angle_to_openness(double angle) {
  return angle / 180.0;
}

double GripperComponent::angle_to_pulse_width(double angle) {
  return SERVO_MIN_PULSE_US + (angle / SERVO_ANGLE_RANGE) *
                                  (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US);
}

double GripperComponent::pulse_width_to_angle(double pulse_width) {
  return ((pulse_width - SERVO_MIN_PULSE_US) /
          (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US)) *
         SERVO_ANGLE_RANGE;
}

void GripperComponent::update_gripper_state() {
  // Read current position from servo if USB is connected
  if (usb_fd_ >= 0) {
    uint16_t position = servo_read_position();
    if (position > 0) {
      // Convert position to openness
      double normalized = static_cast<double>(position - POSITION_MIN) /
                          (POSITION_MAX - POSITION_MIN);
      normalized = std::clamp(normalized, 0.0, 1.0);
      current_openness_.store(normalized);
    }
  }

  // Update moving status (simplified - in real implementation would track
  // target vs current)
  is_moving_.store(false);
}

void GripperComponent::debug_print(const std::string &message) {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;

  std::cout << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S");
  std::cout << "." << std::setfill('0') << std::setw(3) << ms.count() << "] ";
  std::cout << "Gripper: " << message << std::endl;
}

// ========== Additional Helper Methods ==========

bool GripperComponent::is_fully_open() const {
  return current_openness_.load() >= 0.95;
}

bool GripperComponent::is_fully_closed() const {
  return current_openness_.load() <= 0.05;
}

bool GripperComponent::is_moving() const { return is_moving_.load(); }

bool GripperComponent::set_openness_limits(double min_openness,
                                           double max_openness) {
  if (min_openness >= 0.0 && max_openness <= 1.0 &&
      min_openness < max_openness) {
    min_openness_ = min_openness;
    max_openness_ = max_openness;
    debug_print("Openness limits updated: " + std::to_string(min_openness) +
                " - " + std::to_string(max_openness));
    return true;
  }
  return false;
}

uint16_t GripperComponent::read_servo_position() {
  // Ensure USB connection is established for reading
  if (usb_fd_ < 0) {
    if (!usb_connect()) {
      debug_print("Failed to connect USB for position reading");
      return 0;
    }
  }

  // Read actual position from servo
  uint16_t position = servo_read_position();
  return position;
}

} // namespace ic_can
