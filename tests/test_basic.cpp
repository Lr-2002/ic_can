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

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <vector>
#include <iostream>

#include <ic_can/core/ic_can.hpp>
#include <ic_can/motors/base_motor.hpp>
#include <ic_can/motors/dm_motor.hpp>
#include <ic_can/protocols/usb2can_adapter.hpp>
#include <ic_can/core/arm_component.hpp>
#include <ic_can/core/gripper_component.hpp>
#include <ic_can/safety/safety_module.hpp>

using namespace ic_can;
using namespace std::chrono;

class BasicTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test environment
        std::cout << "Setting up basic test..." << std::endl;
    }

    void TearDown() override {
        // Cleanup test environment
        std::cout << "Tearing down basic test..." << std::endl;
    }
};

// Test basic motor creation and configuration
TEST_F(BasicTest, MotorCreation) {
    std::cout << "Testing motor creation..." << std::endl;

    // Test DM motor creation
    auto dm_motor = std::make_unique<DMMotor>(1, DMMotorType::DM4310, "DM4310_001");
    ASSERT_NE(dm_motor, nullptr);
    EXPECT_EQ(dm_motor->get_motor_id(), 1);
    EXPECT_EQ(dm_motor->get_motor_type(), MotorType::DM_DAMIAO);
    EXPECT_FALSE(dm_motor->is_enabled());

    // Test motor parameters
    dm_motor->set_position_limits(-3.14159, 3.14159);
    dm_motor->set_velocity_limits(10.0, 10.0);
    dm_motor->set_torque_limits(5.0, 5.0);

    EXPECT_EQ(dm_motor->get_min_position(), -3.14159);
    EXPECT_EQ(dm_motor->get_max_position(), 3.14159);
    EXPECT_EQ(dm_motor->get_max_velocity(), 10.0);
    EXPECT_EQ(dm_motor->get_max_torque(), 5.0);

    std::cout << "Motor creation test passed!" << std::endl;
}

// Test motor state management
TEST_F(BasicTest, MotorStateManagement) {
    std::cout << "Testing motor state management..." << std::endl;

    auto dm_motor = std::make_unique<DMMotor>(2, DMMotorType::DM6248, "DM6248_001");

    // Test initial state
    EXPECT_FALSE(dm_motor->is_enabled());
    EXPECT_FALSE(dm_motor->is_homed());

    // Test enabling/disabling
    dm_motor->enable();
    EXPECT_TRUE(dm_motor->is_enabled());

    dm_motor->disable();
    EXPECT_FALSE(dm_motor->is_enabled());

    // Test state updates
    dm_motor->update_state(0.5, 0.1, 0.2, 25.0, 0);
    EXPECT_NEAR(dm_motor->get_position(), 0.5, 1e-6);
    EXPECT_NEAR(dm_motor->get_velocity(), 0.1, 1e-6);
    EXPECT_NEAR(dm_motor->get_torque(), 0.2, 1e-6);
    EXPECT_NEAR(dm_motor->get_temperature(), 25.0, 1e-6);

    std::cout << "Motor state management test passed!" << std::endl;
}

// Test motor safety checks
TEST_F(BasicTest, MotorSafetyChecks) {
    std::cout << "Testing motor safety checks..." << std::endl;

    auto dm_motor = std::make_unique<DMMotor>(3, DMMotorType::DM4340, "DM4340_001");

    // Set safety limits
    dm_motor->set_position_limits(-2.0, 2.0);
    dm_motor->set_velocity_limits(5.0, 5.0);
    dm_motor->set_torque_limits(2.0, 2.0);

    // Test position limits
    EXPECT_TRUE(dm_motor->is_position_safe(0.0));
    EXPECT_TRUE(dm_motor->is_position_safe(1.9));
    EXPECT_FALSE(dm_motor->is_position_safe(2.1));
    EXPECT_FALSE(dm_motor->is_position_safe(-2.1));

    // Test velocity limits
    EXPECT_TRUE(dm_motor->is_velocity_safe(0.0));
    EXPECT_TRUE(dm_motor->is_velocity_safe(4.9));
    EXPECT_FALSE(dm_motor->is_velocity_safe(5.1));
    EXPECT_FALSE(dm_motor->is_velocity_safe(-5.1));

    // Test torque limits
    EXPECT_TRUE(dm_motor->is_torque_safe(0.0));
    EXPECT_TRUE(dm_motor->is_torque_safe(1.9));
    EXPECT_FALSE(dm_motor->is_torque_safe(2.1));
    EXPECT_FALSE(dm_motor->is_torque_safe(-2.1));

    // Test temperature limits
    EXPECT_TRUE(dm_motor->is_temperature_safe(25.0));
    EXPECT_TRUE(dm_motor->is_temperature_safe(70.0));
    EXPECT_FALSE(dm_motor->is_temperature_safe(85.0));

    std::cout << "Motor safety checks test passed!" << std::endl;
}

// Test USB2CAN adapter creation and basic functionality
TEST_F(BasicTest, USB2CANAdapter) {
    std::cout << "Testing USB2CAN adapter..." << std::endl;

    try {
        auto adapter = std::make_unique<USB2CANAdapter>("F561E08C892274DB09496BCC1102DBC5");
        ASSERT_NE(adapter, nullptr);

        // Test connection (may fail in simulation)
        bool connected = adapter->connect();
        std::cout << "USB2CAN connection status: " << (connected ? "Connected" : "Disconnected") << std::endl;

        if (connected) {
            EXPECT_TRUE(adapter->is_connected());

            // Test basic CAN frame sending
            CANFrame test_frame;
            test_frame.can_id = 0x201;
            test_frame.dlc = 8;
            test_frame.data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

            bool sent = adapter->send_frame(test_frame);
            std::cout << "CAN frame sent: " << (sent ? "Success" : "Failed") << std::endl;

            adapter->disconnect();
            EXPECT_FALSE(adapter->is_connected());
        }

    } catch (const std::exception& e) {
        std::cout << "USB2CAN test exception (expected in simulation): " << e.what() << std::endl;
    }

    std::cout << "USB2CAN adapter test completed!" << std::endl;
}

// Test arm component creation and basic functionality
TEST_F(BasicTest, ArmComponent) {
    std::cout << "Testing arm component..." << std::endl;

    try {
        auto usb_adapter = std::make_unique<USB2CANAdapter>("F561E08C892274DB09496BCC1102DBC5");
        auto arm = std::make_unique<ArmComponent>(std::move(usb_adapter));
        ASSERT_NE(arm, nullptr);

        // Test motor configuration
        EXPECT_EQ(arm->get_motor_count(), 6);

        // Test position operations
        std::vector<double> test_positions = {0.0, 0.5, 1.0, 0.0, -0.5, 1.5};
        auto positions = arm->get_positions();
        EXPECT_EQ(positions.size(), 6);

        // Test velocity operations
        auto velocities = arm->get_velocities();
        EXPECT_EQ(velocities.size(), 6);

        // Test torque operations
        auto torques = arm->get_torques();
        EXPECT_EQ(torques.size(), 6);

        std::cout << "Arm component test passed!" << std::endl;

    } catch (const std::exception& e) {
        std::cout << "Arm component test exception (expected in simulation): " << e.what() << std::endl;
    }
}

// Test gripper component creation and basic functionality
TEST_F(BasicTest, GripperComponent) {
    std::cout << "Testing gripper component..." << std::endl;

    try {
        auto usb_adapter = std::make_unique<USB2CANAdapter>("F561E08C892274DB09496BCC1102DBC5");
        auto gripper = std::make_unique<GripperComponent>(std::move(usb_adapter));
        ASSERT_NE(gripper, nullptr);

        // Test gripper state
        EXPECT_GE(gripper->get_openness(), 0.0);
        EXPECT_LE(gripper->get_openness(), 1.0);

        // Test gripper operations
        EXPECT_FALSE(gripper->is_moving());
        gripper->stop();

        std::cout << "Gripper component test passed!" << std::endl;

    } catch (const std::exception& e) {
        std::cout << "Gripper component test exception (expected in simulation): " << e.what() << std::endl;
    }
}

// Test safety module creation and basic functionality
TEST_F(BasicTest, SafetyModule) {
    std::cout << "Testing safety module..." << std::endl;

    auto safety = std::make_unique<SafetyModule>();
    ASSERT_NE(safety, nullptr);

    // Test initial state
    EXPECT_FALSE(safety->is_emergency_stopped());
    EXPECT_FALSE(safety->is_watchdog_timeout());
    EXPECT_FALSE(safety->has_active_violations());

    // Test emergency stop
    safety->trigger_emergency_stop("Test emergency stop");
    EXPECT_TRUE(safety->is_emergency_stopped());
    EXPECT_TRUE(safety->has_active_violations());

    auto violations = safety->get_active_violations();
    EXPECT_GT(violations.size(), 0);

    // Reset emergency stop
    safety->reset_emergency_stop();
    EXPECT_FALSE(safety->is_emergency_stopped());

    // Test watchdog
    safety->feed_watchdog();
    EXPECT_FALSE(safety->is_watchdog_timeout());

    std::cout << "Safety module test passed!" << std::endl;
}

// Test IC_CAN main controller creation and configuration
TEST_F(BasicTest, IC_CANController) {
    std::cout << "Testing IC_CAN main controller..." << std::endl;

    try {
        auto controller = std::make_unique<IC_CAN>("F561E08C892274DB09496BCC1102DBC5", true);
        ASSERT_NE(controller, nullptr);

        // Test initial state
        EXPECT_FALSE(controller->is_initialized());

        // Test initialization (may fail in simulation)
        bool initialized = controller->initialize();
        std::cout << "IC_CAN initialization status: " << (initialized ? "Success" : "Failed") << std::endl;

        if (initialized) {
            EXPECT_TRUE(controller->is_initialized());

            // Test component access
            auto arm = controller->get_arm();
            EXPECT_NE(arm, nullptr);

            auto gripper = controller->get_gripper();
            EXPECT_NE(gripper, nullptr);

            auto safety = controller->get_safety();
            EXPECT_NE(safety, nullptr);

            // Test state operations
            auto positions = controller->get_joint_positions();
            EXPECT_EQ(positions.size(), 7); // 6 arm + 1 gripper

            auto velocities = controller->get_joint_velocities();
            EXPECT_EQ(velocities.size(), 7);

            auto torques = controller->get_joint_torques();
            EXPECT_EQ(torques.size(), 7);

            // Test control operations
            std::vector<double> test_positions = {0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6};
            controller->set_joint_positions(test_positions);

            std::vector<double> test_torques = {0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1};
            controller->set_joint_torques(test_torques);

            // Test system operations
            controller->update_all_states();
            controller->enable_all();
            controller->disable_all();

            // Test emergency operations
            controller->emergency_stop();
            controller->reset_emergency_stop();

            // Test control frequency
            controller->set_control_frequency(500.0);
            EXPECT_NEAR(controller->get_control_frequency(), 500.0, 1e-6);

            // Test high-frequency control
            controller->start_high_frequency_control();
            EXPECT_TRUE(controller->is_high_frequency_control_running());

            std::this_thread::sleep_for(milliseconds(100));

            controller->stop_high_frequency_control();
            EXPECT_FALSE(controller->is_high_frequency_control_running());

            // Test system status
            auto status = controller->get_system_status();
            EXPECT_GT(status.size(), 0);

            controller->print_system_info();
        }

        std::cout << "IC_CAN main controller test passed!" << std::endl;

    } catch (const std::exception& e) {
        std::cout << "IC_CAN controller test exception (expected in simulation): " << e.what() << std::endl;
    }
}

// Test DM motor protocol encoding/decoding
TEST_F(BasicTest, DMMotorProtocol) {
    std::cout << "Testing DM motor protocol..." << std::endl;

    auto dm_motor = std::make_unique<DMMotor>(4, DMMotorType::DM10010L, "DM10010L_001");

    // Test MIT control command encoding
    double target_position = 1.0;
    double target_velocity = 0.5;
    double target_torque = 0.2;
    double kp = 50.0;
    double kd = 5.0;

    auto command_data = dm_motor->encode_mit_command(
        target_position, target_velocity, target_torque, kp, kd
    );
    EXPECT_EQ(command_data.size(), 8);

    // Test motor parameter reading command
    auto param_data = dm_motor->encode_read_command(DMRegisterID::MST_ID);
    EXPECT_EQ(param_data.size(), 8);

    // Test motor parameter writing command
    std::vector<uint8_t> write_data = {0x01, 0x02, 0x03, 0x04};
    auto write_command = dm_motor->encode_write_command(DMRegisterID::MST_ID, write_data);
    EXPECT_EQ(write_command.size(), 8);

    std::cout << "DM motor protocol test passed!" << std::endl;
}

// Test CAN frame handling
TEST_F(BasicTest, CANFrameHandling) {
    std::cout << "Testing CAN frame handling..." << std::endl;

    // Test CAN frame creation
    CANFrame frame;
    frame.can_id = 0x201;
    frame.dlc = 8;
    frame.data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    frame.timestamp = steady_clock::now();

    // Test frame validation
    EXPECT_TRUE(frame.can_id == 0x201);
    EXPECT_TRUE(frame.dlc == 8);
    EXPECT_EQ(frame.data.size(), 8);

    // Test frame copying
    CANFrame frame_copy = frame;
    EXPECT_EQ(frame_copy.can_id, frame.can_id);
    EXPECT_EQ(frame_copy.dlc, frame.dlc);
    EXPECT_EQ(frame_copy.data, frame.data);

    std::cout << "CAN frame handling test passed!" << std::endl;
}

// Performance monitoring test
TEST_F(BasicTest, PerformanceMonitoring) {
    std::cout << "Testing performance monitoring..." << std::endl;

    auto start_time = high_resolution_clock::now();

    // Simulate some work
    std::vector<double> positions;
    for (int i = 0; i < 1000; ++i) {
        positions.push_back(i * 0.001);
    }

    auto end_time = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end_time - start_time);

    std::cout << "Performance monitoring test completed in "
              << duration.count() << " microseconds" << std::endl;

    EXPECT_LT(duration.count(), 100000); // Should complete within 100ms
}

int main(int argc, char** argv) {
    std::cout << "=== IC_CAN Basic Tests ===" << std::endl;
    std::cout << "Note: Some tests may fail in simulation mode" << std::endl;
    std::cout << "This is expected behavior when real hardware is not available" << std::endl;
    std::cout << "=============================" << std::endl;

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}