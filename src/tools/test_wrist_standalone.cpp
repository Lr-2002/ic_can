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
 * @file test_wrist_standalone.cpp
 * @brief Standalone wrist monitoring test using dm-tools directly
 *
 * This test bypasses the IC_CAN class and uses dm-tools SDK directly
 * to test CAN communication with wrist motors (7 & 8).
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <csignal>
#include <atomic>
#include <vector>
#include <cstring>
#include <cmath>

// Use dm-tools SDK directly
#include "protocol/usb_class.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Global flag for graceful shutdown
std::atomic<bool> keep_running(true);

void signal_handler(int signal) {
    std::cout << "\n🛑 Received signal " << signal
              << ", shutting down gracefully..." << std::endl;
    keep_running = false;
}

// CAN data structures
struct WristMotorData {
    double position = 0.0;    // radians
    double velocity = 0.0;    // rad/s
    double torque = 0.0;      // Nm
    bool valid = false;
    std::chrono::steady_clock::time_point last_update;
};

// Global motor data storage
WristMotorData motor7_data, motor8_data;

// CAN frame processing callback
void handle_can_frame(can_value_type &frame) {
    uint32_t can_id = frame.head.id;

    // Process HT motor 7 feedback (CAN ID: 0x700)
    if (can_id == 0x700) {
        if (frame.head.dlc >= 7) {
            // Extract HT motor data
            int16_t pos_int = static_cast<int16_t>(frame.data[1] | (frame.data[2] << 8));
            int16_t vel_int = static_cast<int16_t>(frame.data[3] | (frame.data[4] << 8));
            int16_t torque_int = static_cast<int16_t>(frame.data[5] | (frame.data[6] << 8));

            // Convert to physical units (HT protocol)
            const double TURN_TO_RAD = 2.0 * M_PI;
            double position = pos_int * 0.0001 * TURN_TO_RAD;
            double velocity = vel_int * 0.00025 * TURN_TO_RAD;
            const double torque_k = 0.004855;
            const double torque_d = -0.083;
            double torque = torque_int * torque_k + torque_d;

            motor7_data.position = position;
            motor7_data.velocity = velocity;
            motor7_data.torque = torque;
            motor7_data.valid = true;
            motor7_data.last_update = std::chrono::steady_clock::now();

            std::cout << "📥 Motor 7 data: Pos=" << std::fixed << std::setprecision(3)
                      << position << " rad (" << (position * 180.0 / M_PI) << "°), "
                      << "Vel=" << velocity << " rad/s, Tau=" << torque << " Nm" << std::endl;
        }
    }

    // Process HT motor 8 feedback (CAN ID: 0x800)
    else if (can_id == 0x800) {
        if (frame.head.dlc >= 7) {
            // Extract HT motor data
            int16_t pos_int = static_cast<int16_t>(frame.data[1] | (frame.data[2] << 8));
            int16_t vel_int = static_cast<int16_t>(frame.data[3] | (frame.data[4] << 8));
            int16_t torque_int = static_cast<int16_t>(frame.data[5] | (frame.data[6] << 8));

            // Convert to physical units (HT protocol)
            const double TURN_TO_RAD = 2.0 * M_PI;
            double position = pos_int * 0.0001 * TURN_TO_RAD;
            double velocity = vel_int * 0.00025 * TURN_TO_RAD;
            const double torque_k = 0.004855;
            const double torque_d = -0.083;
            double torque = torque_int * torque_k + torque_d;

            motor8_data.position = position;
            motor8_data.velocity = velocity;
            motor8_data.torque = torque;
            motor8_data.valid = true;
            motor8_data.last_update = std::chrono::steady_clock::now();

            std::cout << "📥 Motor 8 data: Pos=" << std::fixed << std::setprecision(3)
                      << position << " rad (" << (position * 180.0 / M_PI) << "°), "
                      << "Vel=" << velocity << " rad/s, Tau=" << torque << " Nm" << std::endl;
        }
    }
}

// Send HT read state command
bool send_ht_read_state(usb_class* device, int motor_id) {
    // HT read state command format
    std::vector<uint8_t> cmd = {0x17, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    uint32_t can_id = (motor_id == 7) ? 0x8007 : 0x8008;

    std::cout << "📤 Sending HT read state to motor " << motor_id
              << " (CAN ID: 0x" << std::hex << can_id << std::dec << ")" << std::endl;

    device->fdcanFrameSend(cmd, can_id);
    return true;
}

// Enable HT motor
bool enable_ht_motor(usb_class* device, int motor_id) {
    std::vector<uint8_t> enable_cmd = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC};
    uint32_t can_id = (motor_id == 7) ? 0x8007 : 0x8008;

    std::cout << "⚡ Enabling HT motor " << motor_id << std::endl;
    device->fdcanFrameSend(enable_cmd, can_id);
    return true;
}

// Disable HT motor
bool disable_ht_motor(usb_class* device, int motor_id) {
    std::vector<uint8_t> disable_cmd = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD};
    uint32_t can_id = (motor_id == 7) ? 0x8007 : 0x8008;

    std::cout << "🔌 Disabling HT motor " << motor_id << std::endl;
    device->fdcanFrameSend(disable_cmd, can_id);
    return true;
}

int main(int argc, char* argv[]) {
    std::cout << "🦾 Standalone Wrist Monitoring Test" << std::endl;
    std::cout << "====================================" << std::endl;

    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Parse command line arguments
    std::string device_sn = "FT4X2Y2A";
    bool debug_enabled = false;
    double poll_frequency = 20.0; // Hz

    if (argc > 1) {
        device_sn = argv[1];
    }
    if (argc > 2 && std::string(argv[2]) == "--debug") {
        debug_enabled = true;
    }
    if (argc > 3) {
        poll_frequency = std::stod(argv[3]);
    }

    std::cout << "Device SN: " << device_sn << std::endl;
    std::cout << "Debug: " << (debug_enabled ? "Enabled" : "Disabled") << std::endl;
    std::cout << "Poll Frequency: " << poll_frequency << " Hz" << std::endl;
    std::cout << std::endl;

    try {
        // Initialize USB2CAN device
        std::cout << "🔧 Initializing USB2CAN device..." << std::endl;
        usb_class device(1000000, 5000000, device_sn);

        // Set up CAN frame callback
        device.setFrameCallback(handle_can_frame);
        std::cout << "✅ CAN callback configured" << std::endl;

        // Start data capture
        std::cout << "🔄 Starting data capture..." << std::endl;
        uint8_t result = device.USB_CMD_START_CAP();
        if (result == 0) {
            std::cout << "✅ Data capture started" << std::endl;
        } else {
            std::cout << "⚠️  Data capture start returned: " << (int)result << std::endl;
        }

        // Enable HT motors
        std::cout << "\n--- Enabling HT Motors ---" << std::endl;
        enable_ht_motor(&device, 7);
        enable_ht_motor(&device, 8);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Start monitoring loop
        std::cout << "\n📊 Starting wrist monitoring loop (Press Ctrl+C to stop)..." << std::endl;
        std::cout << "=================================================" << std::endl;

        auto poll_interval = std::chrono::duration<double>(1.0 / poll_frequency);
        auto last_poll = std::chrono::steady_clock::now();
        auto last_status = last_poll;
        int poll_count = 0;

        while (keep_running) {
            auto now = std::chrono::steady_clock::now();

            // Poll at specified frequency
            if (now - last_poll >= poll_interval) {
                // Send read state commands
                send_ht_read_state(&device, 7);
                send_ht_read_state(&device, 8);

                poll_count++;
                last_poll = now;
            }

            // Print status every second
            if (now - last_status >= std::chrono::seconds(1)) {
                std::cout << "\n--- Status Update #" << (poll_count) << " ---" << std::endl;

                // Check data freshness
                auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - motor7_data.last_update).count();

                std::cout << "Motor 7: " << (motor7_data.valid ? "✅ Connected" : "❌ No data")
                          << ", Pos=" << std::fixed << std::setprecision(2)
                          << (motor7_data.position * 180.0 / M_PI) << "°"
                          << ", Age=" << now_ms << "ms" << std::endl;

                now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - motor8_data.last_update).count();

                std::cout << "Motor 8: " << (motor8_data.valid ? "✅ Connected" : "❌ No data")
                          << ", Pos=" << std::fixed << std::setprecision(2)
                          << (motor8_data.position * 180.0 / M_PI) << "°"
                          << ", Age=" << now_ms << "ms" << std::endl;

                std::cout << "Total polls sent: " << poll_count << std::endl;
                std::cout << "Poll frequency: " << poll_frequency << " Hz" << std::endl;
                std::cout << "=================================================" << std::endl;

                last_status = now;
            }

            // Small sleep to prevent CPU overload
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        // Shutdown sequence
        std::cout << "\n🛑 Shutting down..." << std::endl;

        // Disable motors
        std::cout << "🔌 Disabling HT motors..." << std::endl;
        disable_ht_motor(&device, 7);
        disable_ht_motor(&device, 8);

        // Stop data capture
        std::cout << "🔄 Stopping data capture..." << std::endl;
        device.USB_CMD_STOP_CAP();

        std::cout << "✅ Standalone test completed" << std::endl;

        // Final summary
        std::cout << "\n📊 Test Summary:" << std::endl;
        std::cout << "   Motor 7: " << (motor7_data.valid ? "✅ DATA RECEIVED" : "❌ NO DATA") << std::endl;
        std::cout << "   Motor 8: " << (motor8_data.valid ? "✅ DATA RECEIVED" : "❌ NO DATA") << std::endl;
        std::cout << "   Total polls: " << poll_count << std::endl;

    } catch (const std::exception& e) {
        std::cout << "❌ Exception caught: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}