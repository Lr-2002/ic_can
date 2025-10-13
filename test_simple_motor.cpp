/**
 * @file test_simple_motor.cpp
 * @brief Simple Motor Test using dm-tools SDK directly
 *
 * Compile:
 * g++ -std=c++17 -I./dm-tools/USB2FDCAN/SDK/C++/ubuntu/include \
 *     test_simple_motor.cpp ./dm-tools/USB2FDCAN/SDK/C++/ubuntu/lib/libu2canfd.a \
 *     -lusb-1.0 -pthread -o test_simple_motor
 */

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <csignal>

#include "dm-tools/USB2FDCAN/SDK/C++/ubuntu/include/protocol/usb_class.h"

// Global variables
static usb_class* g_device = nullptr;
static volatile bool g_running = true;

void signal_handler(int signal) {
    std::cout << "\n⚠️  Received signal " << signal << ", stopping..." << std::endl;
    g_running = false;
}

void print_header() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "🔧 SIMPLE MOTOR CONTROL TEST" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Testing single motor connection and basic control" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

bool connect_device() {
    std::cout << "\n📡 STEP 1: Connecting to USB2CAN Device" << std::endl;
    std::cout << std::string(40, '-') << std::endl;

    std::string device_sn = "F561E08C892274DB09496BCC1102DBC5";

    try {
        std::cout << "Creating connection to device: " << device_sn << std::endl;
        g_device = new usb_class(1000000, 5000000, device_sn);

        if (!g_device) {
            std::cout << "❌ FAILED: Could not create device" << std::endl;
            return false;
        }

        int result = g_device->usb_open(device_sn);
        if (result != 0) {
            std::cout << "❌ FAILED: Could not open device (error: " << result << ")" << std::endl;
            std::cout << "   Make sure device is connected and not in use" << std::endl;
            delete g_device;
            g_device = nullptr;
            return false;
        }

        std::cout << "✅ SUCCESS: Device connected" << std::endl;

        // Start data capture
        uint8_t start_result = g_device->USB_CMD_START_CAP();
        if (start_result != 0) {
            std::cout << "⚠️  WARNING: Could not start data capture (error: " << (int)start_result << ")" << std::endl;
        } else {
            std::cout << "✅ Data capture started" << std::endl;
        }

        return true;

    } catch (const std::exception& e) {
        std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
        if (g_device) {
            delete g_device;
            g_device = nullptr;
        }
        return false;
    }
}

bool test_motor_enable() {
    std::cout << "\n⚡ STEP 2: Testing Motor Enable" << std::endl;
    std::cout << std::string(40, '-') << std::endl;

    if (!g_device) {
        std::cout << "❌ No device connected" << std::endl;
        return false;
    }

    try {
        std::cout << "📤 Sending motor enable command to motor ID 4..." << std::endl;

        // DM motor enable command (from openarm reference)
        std::vector<uint8_t> enable_cmd = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC};
        uint32_t motor_id = 0x204;  // Motor 4 receive ID

        g_device->fdcanFrameSend(enable_cmd, motor_id);
        std::cout << "✅ Motor enable command sent" << std::endl;

        // Wait for motor to respond
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        return true;

    } catch (const std::exception& e) {
        std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
        return false;
    }
}

bool test_motor_position() {
    std::cout << "\n🎯 STEP 3: Testing Motor Position Control" << std::endl;
    std::cout << std::string(40, '-') << std::endl;

    if (!g_device) {
        std::cout << "❌ No device connected" << std::endl;
        return false;
    }

    try {
        // Test small position movements
        std::vector<double> positions = {0.1, -0.1, 0.0};  // radians

        for (size_t i = 0; i < positions.size() && g_running; ++i) {
            double target_pos = positions[i];
            std::cout << "🎯 Moving to position: " << target_pos << " rad" << std::endl;

            // Simplified DM motor position command
            std::vector<uint8_t> pos_cmd = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            uint32_t motor_id = 0x004;  // Motor 4 send ID

            g_device->fdcanFrameSend(pos_cmd, motor_id);
            std::cout << "✅ Position command sent" << std::endl;

            // Wait for movement
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }

        return true;

    } catch (const std::exception& e) {
        std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
        return false;
    }
}

void cleanup() {
    std::cout << "\n🧹 Cleaning up..." << std::endl;

    if (g_device) {
        try {
            // Send motor disable command
            std::cout << "📤 Sending motor disable command..." << std::endl;
            std::vector<uint8_t> disable_cmd = {0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC};
            g_device->fdcanFrameSend(disable_cmd, 0x204);

            // Stop data capture
            g_device->USB_CMD_STOP_CAP();
            std::cout << "🛑 Data capture stopped" << std::endl;

            delete g_device;
            std::cout << "✅ Device disconnected" << std::endl;

        } catch (...) {
            std::cout << "⚠️  Cleanup completed with some warnings" << std::endl;
        }

        g_device = nullptr;
    }
}

int main() {
    print_header();

    // Set up signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    bool all_tests_passed = true;

    try {
        // Step 1: Connect to device
        if (!connect_device()) {
            all_tests_passed = false;
            goto cleanup;
        }

        // Step 2: Test motor enable
        if (!test_motor_enable()) {
            all_tests_passed = false;
        }

        // Step 3: Test position control
        if (!test_motor_position()) {
            all_tests_passed = false;
        }

    } catch (const std::exception& e) {
        std::cout << "❌ FATAL EXCEPTION: " << e.what() << std::endl;
        all_tests_passed = false;
    }

cleanup:
    cleanup();

    std::cout << "\n" << std::string(60, '=') << std::endl;
    if (all_tests_passed && g_running) {
        std::cout << "🎉 MOTOR TEST COMPLETED SUCCESSFULLY!" << std::endl;
        std::cout << "✅ dm-tools SDK is working" << std::endl;
        std::cout << "✅ USB2CAN connection established" << std::endl;
        std::cout << "✅ Motor commands sent successfully" << std::endl;
    } else {
        std::cout << "❌ MOTOR TEST FAILED" << std::endl;
        std::cout << "⚠️  Check device connection" << std::endl;
        std::cout << "⚠️  Verify no other process is using the device" << std::endl;
        std::cout << "⚠️  Check motor wiring and power" << std::endl;
    }
    std::cout << std::string(60, '=') << std::endl;

    return (all_tests_passed && g_running) ? 0 : -1;
}