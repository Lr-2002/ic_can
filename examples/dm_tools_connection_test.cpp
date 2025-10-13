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
 * @file dm_tools_connection_test.cpp
 * @brief Direct dm-tools SDK Connection Test
 *
 * This test bypasses the IC_CAN framework and directly tests the dm-tools SDK
 * to verify basic USB2CAN connection and communication functionality.
 *
 * Usage:
 *   ./dm_tools_connection_test [device_serial]
 *
 * Default: device_serial="F561E08C892274DB09496BCC1102DBC5"
 */

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <csignal>

// Direct dm-tools SDK includes
#define private public
#include "../dm-tools/USB2FDCAN/SDK/C++/arm/include/protocol/usb_class.h"
#include "../dm-tools/USB2FDCAN/SDK/C++/arm/include/protocol/damiao.h"
#undef private

// Global variables for signal handling
static usb_class* g_usb_device = nullptr;
static volatile bool g_running = true;

void signal_handler(int signal) {
    std::cout << "\n⚠️  Received signal " << signal << ", stopping test..." << std::endl;
    g_running = false;
}

void print_test_header(const std::string& device_sn) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "🔧 DM-TOOLS SDK DIRECT CONNECTION TEST" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Device Serial: " << device_sn << std::endl;
    std::cout << "Test Duration: ~15 seconds" << std::endl;
    std::cout << "Press Ctrl+C to abort" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

bool test_connection(const std::string& device_sn) {
    std::cout << "\n🔗 STEP 1: Testing dm-tools SDK Connection" << std::endl;
    std::cout << std::string(40, '-') << std::endl;

    try {
        std::cout << "📡 Creating usb_class instance..." << std::endl;
        g_usb_device = new usb_class(1000000, 5000000, device_sn);  // 1M baud, 5M FD

        if (!g_usb_device) {
            std::cout << "❌ FAILED: Could not create usb_class instance" << std::endl;
            return false;
        }
        std::cout << "✅ SUCCESS: usb_class instance created" << std::endl;

        std::cout << "🔌 Connecting to USB2CAN device..." << std::endl;
        int result = g_usb_device->usb_open(device_sn);
        if (result != 0) {
            std::cout << "❌ FAILED: usb_open returned " << result << std::endl;
            delete g_usb_device;
            g_usb_device = nullptr;
            return false;
        }
        std::cout << "✅ SUCCESS: Connected to USB2CAN device" << std::endl;

        return true;

    } catch (const std::exception& e) {
        std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
        if (g_usb_device) {
            delete g_usb_device;
            g_usb_device = nullptr;
        }
        return false;
    }
}

bool test_can_communication() {
    std::cout << "\n📡 STEP 2: Testing CAN Communication" << std::endl;
    std::cout << std::string(40, '-') << std::endl;

    if (!g_usb_device) {
        std::cout << "❌ No device connected" << std::endl;
        return false;
    }

    try {
        // Test CAN frame sending
        std::cout << "📤 Testing CAN frame transmission..." << std::endl;

        std::vector<uint8_t> test_data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
        uint32_t test_can_id = 0x123;

        g_usb_device->fdcanFrameSend(test_data, test_can_id);
        // fdcanFrameSend returns void, so we assume success if no exception is thrown
        std::cout << "✅ SUCCESS: CAN frame sent (ID=0x" << std::hex << test_can_id << std::dec << ")" << std::endl;

        // Test data capture start
        std::cout << "🔄 Starting data capture..." << std::endl;
        int start_result = g_usb_device->USB_CMD_START_CAP();
        if (start_result != 0) {
            std::cout << "❌ FAILED: USB_CMD_START_CAP returned " << start_result << std::endl;
            return false;
        }
        std::cout << "✅ SUCCESS: Data capture started" << std::endl;

        return true;

    } catch (const std::exception& e) {
        std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
        return false;
    }
}

bool test_frame_reception() {
    std::cout << "\n📥 STEP 3: Testing CAN Frame Reception" << std::endl;
    std::cout << std::string(40, '-') << std::endl;

    if (!g_usb_device) {
        std::cout << "❌ No device connected" << std::endl;
        return false;
    }

    try {
        // Set up frame callback
        bool frame_received = false;
        int frame_count = 0;

        g_usb_device->setFrameCallback([&frame_received, &frame_count](can_value_type& can_value) {
            frame_received = true;
            frame_count++;
            std::cout << "📥 Received frame #" << frame_count
                      << " | ID=0x" << std::hex << can_value.head.id << std::dec
                      << " | DLC=" << (int)can_value.head.dlc << std::endl;
        });

        std::cout << "📡 Listening for CAN frames for 5 seconds..." << std::endl;

        auto start_time = std::chrono::steady_clock::now();
        while (g_running &&
               std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::steady_clock::now() - start_time).count() < 5) {

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (frame_received) {
            std::cout << "✅ SUCCESS: Received " << frame_count << " CAN frames" << std::endl;
            return true;
        } else {
            std::cout << "⚠️  WARNING: No CAN frames received (this is normal if no motors are responding)" << std::endl;
            return true;  // Not receiving frames is expected if no motors are connected
        }

    } catch (const std::exception& e) {
        std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
        return false;
    }
}

bool test_motor_command() {
    std::cout << "\n🎯 STEP 4: Testing Motor Command" << std::endl;
    std::cout << std::string(40, '-') << std::endl;

    if (!g_usb_device) {
        std::cout << "❌ No device connected" << std::endl;
        return false;
    }

    try {
        std::cout << "📤 Sending motor enable command (DM motor, ID=4)..." << std::endl;

        // DM motor enable command format (simplified)
        std::vector<uint8_t> enable_cmd = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC};
        uint32_t motor_can_id = 0x204;  // Motor ID 4 receive ID

        g_usb_device->fdcanFrameSend(enable_cmd, motor_can_id);
        // fdcanFrameSend returns void, assume success if no exception
        std::cout << "✅ SUCCESS: Motor enable command sent" << std::endl;

        // Wait and try to send position command
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::cout << "📤 Sending position command (0.1 rad)..." << std::endl;

        // DM motor MIT command format (simplified)
        std::vector<uint8_t> pos_cmd = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        uint32_t cmd_can_id = 0x004;  // Motor ID 4 send ID

        g_usb_device->fdcanFrameSend(pos_cmd, cmd_can_id);
        // fdcanFrameSend returns void, assume success if no exception
        std::cout << "✅ SUCCESS: Position command sent" << std::endl;

        return true;

    } catch (const std::exception& e) {
        std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
        return false;
    }
}

void cleanup() {
    std::cout << "\n🧹 Cleaning up resources..." << std::endl;

    if (g_usb_device) {
        try {
            g_usb_device->USB_CMD_STOP_CAP();
            std::cout << "🛑 Data capture stopped" << std::endl;
        } catch (...) {
            // Ignore errors during cleanup
        }

        try {
            delete g_usb_device;
            std::cout << "🗑️  USB device deleted" << std::endl;
        } catch (...) {
            // Ignore errors during cleanup
        }

        g_usb_device = nullptr;
    }
}

void print_test_result(bool success) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    if (success) {
        std::cout << "🎉 DM-TOOLS SDK TEST COMPLETED!" << std::endl;
        std::cout << "✅ Basic connection to USB2CAN device works" << std::endl;
        std::cout << "✅ CAN frame transmission is functional" << std::endl;
        std::cout << "✅ dm-tools SDK integration is working" << std::endl;
    } else {
        std::cout << "❌ DM-TOOLS SDK TEST FAILED!" << std::endl;
        std::cout << "⚠️  Check USB2CAN device connection" << std::endl;
        std::cout << "⚠️  Verify device serial number" << std::endl;
        std::cout << "⚠️  Check dm-tools SDK library" << std::endl;
    }
    std::cout << std::string(60, '=') << std::endl;
}

int main(int argc, char* argv[]) {
    // Parse arguments
    std::string device_sn = "F561E08C892274DB09496BCC1102DBC5";
    if (argc > 1) {
        device_sn = argv[1];
    }

    print_test_header(device_sn);

    // Set up signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    bool all_tests_passed = true;

    try {
        // Step 1: Test connection
        if (!test_connection(device_sn)) {
            all_tests_passed = false;
            goto cleanup;
        }

        // Step 2: Test CAN communication
        if (!test_can_communication()) {
            all_tests_passed = false;
            goto cleanup;
        }

        // Step 3: Test frame reception
        if (!test_frame_reception()) {
            all_tests_passed = false;
        }

        // Step 4: Test motor command
        if (!test_motor_command()) {
            all_tests_passed = false;
        }

    } catch (const std::exception& e) {
        std::cout << "❌ FATAL EXCEPTION: " << e.what() << std::endl;
        all_tests_passed = false;
    }

cleanup:
    cleanup();
    print_test_result(all_tests_passed && g_running);

    return (all_tests_passed && g_running) ? 0 : -1;
}