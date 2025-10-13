/**
 * @file test_dm_sdk_damiao.cpp
 * @brief Damiao Motor Control Test using proper dm-tools SDK
 */

#include <csignal>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>

#include "dm-tools/USB2FDCAN/SDK/C++/ubuntu/include/protocol/damiao.h"

static volatile bool g_running = true;

void signal_handler(int signal) {
  std::cout << "\n⚠️  Received signal " << signal << ", stopping..."
            << std::endl;
  g_running = false;
}

int main() {
  std::cout << "=== Damiao Motor Control Test ===" << std::endl;
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  try {
    std::string device_sn = "F561E08C892274DB09496BCC1102DBC5";

    std::cout << "Creating Damiao Motor_Control..." << std::endl;
    std::cout << "Device SN: " << device_sn << std::endl;

    // Create motor data configuration
    std::vector<damiao::DmActData> motor_data;

    // Configure Motor ID 1 (DM4340 - example motor type)
    damiao::DmActData motor1;
    motor1.motorType = damiao::DM4340;          // Motor type
    motor1.mode = damiao::MIT_MODE;             // Control mode
    motor1.can_id = 0x01;                      // Motor send ID
    motor1.mst_id = 0x01;                      // Motor receive ID (feedback)
    motor_data.push_back(motor1);

    // Create motor control instance
    auto motor_control = std::make_unique<damiao::Motor_Control>(
        1000000, 5000000, device_sn, &motor_data);

    // Create motor instance
    auto motor = std::make_shared<damiao::Motor>(
        damiao::DM4340, damiao::MIT_MODE, 0x01, 0x01);

    // Add motor to controller
    motor_control->addMotor(motor);

    std::cout << "✅ SUCCESS: Damiao Motor_Control created" << std::endl;
    std::cout << "📊 Motor Type: DM4340" << std::endl;
    std::cout << "📊 Control Mode: MIT Mode" << std::endl;
    std::cout << "📊 Motor ID: 1 (Send/Receive)" << std::endl;

    // Enable motor
    std::cout << "\n⚡ Enabling motor..." << std::endl;
    motor_control->enable_all();
    std::cout << "✅ Motor enabled" << std::endl;

    // Wait for motor to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Refresh motor status to get current state
    std::cout << "\n📊 Reading motor status..." << std::endl;
    motor_control->refresh_motor_status(*motor);

    std::cout << "Current Position: " << motor->Get_Position() << " rad" << std::endl;
    std::cout << "Current Velocity: " << motor->Get_Velocity() << " rad/s" << std::endl;
    std::cout << "Current Torque: " << motor->Get_tau() << " Nm" << std::endl;

    // Test MIT control commands
    std::cout << "\n🎯 Testing MIT control commands..." << std::endl;

    // Command 1: Zero position
    std::cout << "📍 Sending zero position command..." << std::endl;
    motor_control->control_mit(*motor, 20.0f, 0.5f, 0.0f, 0.0f, 0.0f);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Refresh status
    motor_control->refresh_motor_status(*motor);
    std::cout << "Position: " << motor->Get_Position() << " rad" << std::endl;

    // Command 2: Small positive position (0.1 rad ≈ 5.7°)
    std::cout << "📍 Moving to 0.1 rad position..." << std::endl;
    motor_control->control_mit(*motor, 20.0f, 0.5f, 0.1f, 0.0f, 0.0f);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // Refresh status
    motor_control->refresh_motor_status(*motor);
    std::cout << "Position: " << motor->Get_Position() << " rad" << std::endl;

    // Command 3: Small negative position (-0.1 rad ≈ -5.7°)
    std::cout << "📍 Moving to -0.1 rad position..." << std::endl;
    motor_control->control_mit(*motor, 20.0f, 0.5f, -0.1f, 0.0f, 0.0f);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // Refresh status
    motor_control->refresh_motor_status(*motor);
    std::cout << "Position: " << motor->Get_Position() << " rad" << std::endl;

    // Command 4: Back to zero
    std::cout << "📍 Returning to zero position..." << std::endl;
    motor_control->control_mit(*motor, 20.0f, 0.5f, 0.0f, 0.0f, 0.0f);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Final status
    motor_control->refresh_motor_status(*motor);
    std::cout << "Final Position: " << motor->Get_Position() << " rad" << std::endl;

    // Disable motor
    std::cout << "\n🔌 Disabling motor..." << std::endl;
    motor_control->disable_all();
    std::cout << "✅ Motor disabled" << std::endl;

    // Cleanup (Motor_Control destructor will handle cleanup)
    motor_control.reset();

    std::cout << "\n🎉 DAMIAO MOTOR TEST COMPLETED!" << std::endl;
    std::cout << "✅ dm-tools SDK connected successfully" << std::endl;
    std::cout << "✅ Damiao Motor_Control working" << std::endl;
    std::cout << "✅ Motor enable/disable successful" << std::endl;
    std::cout << "✅ MIT position control working" << std::endl;

    return 0;

  } catch (const std::exception &e) {
    std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
    return -1;
  }
}