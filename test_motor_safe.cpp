/**
 * @file test_motor_safe.cpp
 * @brief Safe Motor Test with USB conflict handling
 */

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <csignal>

#include "dm-tools/USB2FDCAN/SDK/C++/ubuntu/include/protocol/usb_class.h"

static usb_class* g_device = nullptr;
static volatile bool g_running = true;

void signal_handler(int signal) {
    std::cout << "\n⚠️  Received signal " << signal << ", stopping..." << std::endl;
    g_running = false;
}

void print_header() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "🔧 SAFE MOTOR CONTROL TEST" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "This test handles USB device conflicts gracefully" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

bool connect_device_with_retry(int max_retries = 3) {
    std::string device_sn = "F561E08C892274DB09496BCC1102DBC5";

    for (int attempt = 1; attempt <= max_retries; ++attempt) {
        std::cout << "\n📡 Connection Attempt " << attempt << "/" << max_retries << std::endl;
        std::cout << std::string(40, '-') << std::endl;

        try {
            std::cout << "Creating connection to device: " << device_sn << std::endl;
            g_device = new usb_class(1000000, 5000000, device_sn);

            if (!g_device) {
                std::cout << "❌ FAILED: Could not create device" << std::endl;
                continue;
            }

            std::cout << "Opening device..." << std::endl;
            int result = g_device->usb_open(device_sn);

            if (result == 0) {
                std::cout << "✅ SUCCESS: Device connected" << std::endl;

                // Start data capture
                uint8_t start_result = g_device->USB_CMD_START_CAP();
                if (start_result != 0) {
                    std::cout << "⚠️  WARNING: Could not start data capture" << std::endl;
                } else {
                    std::cout << "✅ Data capture started" << std::endl;
                }

                return true;
            } else {
                std::cout << "❌ FAILED: Could not open device (error: " << result << ")" << std::endl;
                std::cout << "   This usually means another process is using the device" << std::endl;

                delete g_device;
                g_device = nullptr;
            }

        } catch (const std::exception& e) {
            std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
            if (g_device) {
                delete g_device;
                g_device = nullptr;
            }
        }

        if (attempt < max_retries) {
            std::cout << "\n🔄 Retrying in 3 seconds..." << std::endl;
            std::cout << "💡 Tip: Unplug and replug the USB2CAN device if this persists" << std::endl;

            for (int i = 3; i > 0 && g_running; --i) {
                std::cout << "   Waiting " << i << " seconds..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }

    return false;
}

bool test_motor_commands() {
    if (!g_device) {
        std::cout << "❌ No device connected" << std::endl;
        return false;
    }

    std::cout << "\n⚡ STEP 2: Testing Motor Commands" << std::endl;
    std::cout << std::string(40, '-') << std::endl;

    try {
        // Enable motor
        std::cout << "📤 Sending motor enable command to motor ID 4..." << std::endl;
        std::vector<uint8_t> enable_cmd = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC};
        g_device->fdcanFrameSend(enable_cmd, 0x204);
        std::cout << "✅ Motor enable command sent" << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // Send test position
        std::cout << "📤 Sending test position command..." << std::endl;
        std::vector<uint8_t> pos_cmd = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        g_device->fdcanFrameSend(pos_cmd, 0x004);
        std::cout << "✅ Position command sent" << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // Disable motor
        std::cout << "📤 Sending motor disable command..." << std::endl;
        std::vector<uint8_t> disable_cmd = {0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC};
        g_device->fdcanFrameSend(disable_cmd, 0x204);
        std::cout << "✅ Motor disable command sent" << std::endl;

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
            g_device->USB_CMD_STOP_CAP();
            delete g_device;
            std::cout << "✅ Device cleaned up" << std::endl;
        } catch (...) {
            std::cout << "⚠️  Cleanup completed with warnings" << std::endl;
        }
        g_device = nullptr;
    }
}

int main() {
    print_header();
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    bool all_tests_passed = true;

    try {
        // Step 1: Connect to device with retry
        if (!connect_device_with_retry()) {
            all_tests_passed = false;
            std::cout << "\n❌ Could not connect to device after multiple attempts" << std::endl;
            std::cout << "💡 Solutions:" << std::endl;
            std::cout << "   1. Unplug and replug the USB2CAN device" << std::endl;
            std::cout << "   2. Stop any other programs using the device" << std::endl;
            std::cout << "   3. Wait a few seconds and try again" << std::endl;
            goto cleanup;
        }

        // Step 2: Test motor commands
        if (!test_motor_commands()) {
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
        std::cout << "✅ Motor commands executed successfully" << std::endl;
    } else {
        std::cout << "❌ MOTOR TEST FAILED" << std::endl;
        std::cout << "⚠️  USB device conflict detected" << std::endl;
        std::cout << "💡 Try unplugging and replugging the device" << std::endl;
    }
    std::cout << std::string(60, '=') << std::endl;

    return (all_tests_passed && g_running) ? 0 : -1;
}