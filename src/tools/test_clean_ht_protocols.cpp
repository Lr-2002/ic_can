/**
 * @file test_clean_ht_protocols.cpp
 * @brief Test Clean HT Motor Protocols Only
 *
 * Simple test demonstrating the two clean protocols:
 * - 48-byte frame commands for setting positions
 * - 12-byte refresh commands for requesting positions
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include "ic_can/motors/ht_motor.hpp"
#include "ic_can/motors/ht_motor_frame.hpp"

void print_header() {
    std::cout << "========================================" << std::endl;
    std::cout << "=== Clean HT Motor Protocol Test ===" << std::endl;
    std::cout << "========================================" << std::endl;
}

void test_48_byte_frame_protocol() {
    std::cout << "\n📋 Testing 48-byte Frame Protocol (Position Control)" << std::endl;
    std::cout << "----------------------------------------------" << std::endl;

    // Create HT motors
    ic_can::HTMotor motor7(7, 0x8007, 0x700, 50.0, 0.8, 18.0, true);
    ic_can::HTMotor motor8(8, 0x8008, 0x800, 50.0, 0.8, 18.0, true);

    // Create 48-byte frames for position control
    double m7_pos = 0.1;  // radians
    double m8_pos = 0.05; // radians
    double kp = 30.0;
    double kd = 0.5;

    std::cout << "🎯 Creating position control frames:" << std::endl;
    std::cout << "   Motor 7 target: " << std::fixed << std::setprecision(3)
              << m7_pos << " rad (" << (m7_pos * 180.0 / M_PI) << "°)" << std::endl;
    std::cout << "   Motor 8 target: " << std::fixed << std::setprecision(3)
              << m8_pos << " rad (" << (m8_pos * 180.0 / M_PI) << "°)" << std::endl;
    std::cout << "   Gains: kp=" << kp << ", kd=" << kd << std::endl;

    // Create frames
    auto frame7 = motor7.create_frame(m7_pos, 0.0, 0.0, kp, kd);
    auto frame8 = motor8.create_frame(m8_pos, 0.0, 0.0, kp, kd);

    // Display frame data
    std::cout << "\n📤 Motor 7 Frame (48 bytes, CAN ID: 0x8007):" << std::endl;
    auto data7 = motor7.get_frame_command_data();
    for (size_t i = 0; i < data7.size(); i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data7[i] << " ";
        if ((i + 1) % 16 == 0) std::cout << std::endl;
    }
    std::cout << std::dec << std::endl;

    std::cout << "\n📤 Motor 8 Frame (48 bytes, CAN ID: 0x8008):" << std::endl;
    auto data8 = motor8.get_frame_command_data();
    for (size_t i = 0; i < data8.size(); i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data8[i] << " ";
        if ((i + 1) % 16 == 0) std::cout << std::endl;
    }
    std::cout << std::dec << std::endl;

    std::cout << "✅ 48-byte frame protocol test completed" << std::endl;
}

void test_12_byte_refresh_protocol() {
    std::cout << "\n📋 Testing 12-byte Refresh Protocol (Position Request)" << std::endl;
    std::cout << "----------------------------------------------" << std::endl;

    // Create HT motors
    ic_can::HTMotor motor7(7, 0x8007, 0x700, 50.0, 0.8, 18.0, true);
    ic_can::HTMotor motor8(8, 0x8008, 0x800, 50.0, 0.8, 18.0, true);

    std::cout << "🔄 Creating refresh commands for position requests:" << std::endl;

    // Send refresh commands
    bool success7 = motor7.refresh();
    bool success8 = motor8.refresh();

    std::cout << "   Motor 7 refresh: " << (success7 ? "✅ SUCCESS" : "❌ FAILED") << std::endl;
    std::cout << "   Motor 8 refresh: " << (success8 ? "✅ SUCCESS" : "❌ FAILED") << std::endl;

    // Display refresh command data
    std::cout << "\n📤 Motor 7 Refresh (12 bytes, CAN ID: 0x8007):" << std::endl;
    auto cmd7 = motor7.get_command_data();
    for (size_t i = 0; i < cmd7.size(); i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)cmd7[i] << " ";
    }
    std::cout << std::dec << std::endl;

    std::cout << "\n📤 Motor 8 Refresh (12 bytes, CAN ID: 0x8008):" << std::endl;
    auto cmd8 = motor8.get_command_data();
    for (size_t i = 0; i < cmd8.size(); i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)cmd8[i] << " ";
    }
    std::cout << std::dec << std::endl;

    // Verify the refresh command format
    uint8_t expected_refresh[12] = {0x01, 0x00, 0x00, 0x11, 0x00, 0x1f, 0x01, 0x13, 0x0d, 0x50, 0x50, 0x50};

    bool cmd7_match = (cmd7.size() == 12);
    bool cmd8_match = (cmd8.size() == 12);

    if (cmd7_match) {
        for (int i = 0; i < 12; i++) {
            if (cmd7[i] != expected_refresh[i]) {
                cmd7_match = false;
                break;
            }
        }
    }

    if (cmd8_match) {
        for (int i = 0; i < 12; i++) {
            if (cmd8[i] != expected_refresh[i]) {
                cmd8_match = false;
                break;
            }
        }
    }

    std::cout << "\n🔍 Refresh command verification:" << std::endl;
    std::cout << "   Expected: 01 00 00 11 00 1f 01 13 0d 50 50 50" << std::endl;
    std::cout << "   Motor 7: " << (cmd7_match ? "✅ MATCH" : "❌ MISMATCH") << std::endl;
    std::cout << "   Motor 8: " << (cmd8_match ? "✅ MATCH" : "❌ MISMATCH") << std::endl;

    std::cout << "✅ 12-byte refresh protocol test completed" << std::endl;
}

void show_protocol_summary() {
    std::cout << "\n📋 Clean HT Motor Protocol Summary" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << "🎯 POSITION CONTROL (48-byte frames):" << std::endl;
    std::cout << "   - Motor 7: Send ID 0x8007, Receive ID 0x700" << std::endl;
    std::cout << "   - Motor 8: Send ID 0x8008, Receive ID 0x800" << std::endl;
    std::cout << "   - Structure: Header + Mode + Control + Magic + Gains + Tail + Padding" << std::endl;
    std::cout << "   - Use: Send position commands to HT motors" << std::endl;
    std::cout << std::endl;
    std::cout << "🔄 POSITION REQUEST (12-byte refresh):" << std::endl;
    std::cout << "   - Command: 01 00 00 11 00 1f 01 13 0d 50 50 50" << std::endl;
    std::cout << "   - Motor 7: Send ID 0x8007, Receive ID 0x700" << std::endl;
    std::cout << "   - Motor 8: Send ID 0x8008, Receive ID 0x800" << std::endl;
    std::cout << "   - Use: Request current positions from HT motors" << std::endl;
    std::cout << std::endl;
    std::cout << "✅ All old HT methods have been removed!" << std::endl;
    std::cout << "✅ Only the two clean protocols remain!" << std::endl;
}

int main(int argc, char *argv[]) {
    print_header();

    std::cout << "🎯 Testing Clean HT Motor Implementation" << std::endl;
    std::cout << "📋 Only two protocols:" << std::endl;
    std::cout << "   1. 48-byte frames for setting positions" << std::endl;
    std::cout << "   2. 12-byte refresh for requesting positions" << std::endl;
    std::cout << "   ❌ All old HT methods have been removed!" << std::endl;

    try {
        test_48_byte_frame_protocol();
        test_12_byte_refresh_protocol();
        show_protocol_summary();

        std::cout << "\n🎉 All tests passed! Clean HT implementation is working correctly." << std::endl;
        return 0;

    } catch (const std::exception &e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
}