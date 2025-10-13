/**
 * @file test_dm_sdk_basic.cpp
 * @brief Minimal dm-tools SDK test outside IC_CAN framework
 *
 * Compile manually:
 * g++ -std=c++17 -I./dm-tools/USB2FDCAN/SDK/C++/ubuntu/include \
 *     -L./dm-tools/USB2FDCAN/SDK/C++/ubuntu/lib \
 *     -lu2canfd -lusb-1.0 -pthread test_dm_sdk_basic.cpp -o test_dm_sdk_basic
 */

#include <iostream>
#include <vector>
#include <string>

// Try different SDK paths based on platform
#ifdef __APPLE__
    #include "dm-tools/USB2FDCAN/SDK/C++/arm/include/protocol/usb_class.h"
#else
    #include "dm-tools/USB2FDCAN/SDK/C++/ubuntu/include/protocol/usb_class.h"
#endif

int main() {
    std::cout << "=== dm-tools SDK Basic Test ===" << std::endl;

    try {
        // Test basic USB class creation
        std::string device_sn = "F561E08C892274DB09496BCC1102DBC5";

        std::cout << "Creating usb_class instance..." << std::endl;
        usb_class* device = new usb_class(1000000, 5000000, device_sn);

        if (!device) {
            std::cout << "❌ FAILED: Could not create usb_class" << std::endl;
            return -1;
        }

        std::cout << "✅ SUCCESS: usb_class created" << std::endl;

        // Test connection
        std::cout << "Attempting to connect to device..." << std::endl;
        int result = device->usb_open(device_sn);

        if (result == 0) {
            std::cout << "✅ SUCCESS: Connected to device" << std::endl;

            // Test CAN frame send
            std::cout << "Testing CAN frame transmission..." << std::endl;
            std::vector<uint8_t> test_data = {0x01, 0x02, 0x03, 0x04};
            device->fdcanFrameSend(test_data, 0x123);
            std::cout << "✅ SUCCESS: CAN frame sent" << std::endl;

            // Cleanup
            delete device;
            std::cout << "✅ TEST PASSED: dm-tools SDK is working!" << std::endl;
            return 0;

        } else {
            std::cout << "❌ FAILED: Could not connect to device (error: " << result << ")" << std::endl;
            std::cout << "   Check if device is connected and serial number is correct" << std::endl;
            delete device;
            return -1;
        }

    } catch (const std::exception& e) {
        std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
        return -1;
    }
}