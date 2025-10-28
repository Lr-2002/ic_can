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
 * @file wrist_gripper_simple_demo.cpp
 * @brief Simple demo showing separated wrist and gripper component usage
 *
 * This is a simplified demonstration of the WristComponent and GripperComponent
 * separation without requiring the full IC_CAN framework.
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <memory>
#include <cmath>

// Mock classes to demonstrate the component separation concept
class MockMotor {
public:
    MockMotor(int id, const std::string& name) : id_(id), name_(name), position_(0.0) {}

    int get_id() const { return id_; }
    std::string get_name() const { return name_; }

    bool set_position_target(double position, double velocity) {
        position_ = position;
        std::cout << "  🎯 " << name_ << " (ID " << id_ << "): "
                  << "Moving to position " << std::fixed << std::setprecision(3)
                  << position << " rad at " << velocity << " rad/s" << std::endl;
        return true;
    }

    double get_position() const { return position_; }

    bool enable() {
        std::cout << "  🔌 " << name_ << " (ID " << id_ << "): Enabled" << std::endl;
        return true;
    }

    bool disable() {
        std::cout << "  ⭕ " << name_ << " (ID " << id_ << "): Disabled" << std::endl;
        return true;
    }

private:
    int id_;
    std::string name_;
    double position_;
};

// Mock Wrist Component
class WristComponent {
public:
    WristComponent() {
        // Initialize mock wrist motors
        motors_[7] = std::make_shared<MockMotor>(7, "HT4438-WristPitch");
        motors_[8] = std::make_shared<MockMotor>(8, "HT4438-WristRoll");
        std::cout << "✅ WristComponent initialized with HT motors m7-m8" << std::endl;
    }

    bool enable_all() {
        std::cout << "\n🔌 Enabling wrist motors..." << std::endl;
        motors_[7]->enable();
        motors_[8]->enable();
        return true;
    }

    bool disable_all() {
        std::cout << "\n⭕ Disabling wrist motors..." << std::endl;
        motors_[7]->disable();
        motors_[8]->disable();
        return true;
    }

    bool set_pitch_angle(double angle, double velocity = 1.0) {
        std::cout << "\n📍 Setting wrist pitch angle to "
                  << std::fixed << std::setprecision(2) << angle * 180.0 / M_PI
                  << " degrees (" << angle << " rad)" << std::endl;
        return motors_[7]->set_position_target(angle, velocity);
    }

    bool set_roll_angle(double angle, double velocity = 1.0) {
        std::cout << "\n🔄 Setting wrist roll angle to "
                  << std::fixed << std::setprecision(2) << angle * 180.0 / M_PI
                  << " degrees (" << angle << " rad)" << std::endl;
        return motors_[8]->set_position_target(angle, velocity);
    }

    bool set_wrist_pose(double pitch, double roll, double velocity = 1.0) {
        std::cout << "\n🕺 Setting wrist pose - Pitch: "
                  << std::fixed << std::setprecision(2) << pitch * 180.0 / M_PI
                  << "°, Roll: " << roll * 180.0 / M_PI << "°" << std::endl;
        motors_[7]->set_position_target(pitch, velocity);
        motors_[8]->set_position_target(roll, velocity);
        return true;
    }

    double get_pitch_angle() const {
        return motors_.at(7)->get_position();
    }

    double get_roll_angle() const {
        return motors_.at(8)->get_position();
    }

    std::vector<double> get_positions() const {
        return {get_pitch_angle(), get_roll_angle()};
    }

    void print_wrist_state() const {
        auto pos = get_positions();
        std::cout << "\n📊 Wrist Component State:" << std::endl;
        std::cout << "  Pitch Angle: " << std::fixed << std::setprecision(2)
                  << pos[0] * 180.0 / M_PI << "° (" << pos[0] << " rad)" << std::endl;
        std::cout << "  Roll Angle: " << std::fixed << std::setprecision(2)
                  << pos[1] * 180.0 / M_PI << "° (" << pos[1] << " rad)" << std::endl;
    }

private:
    std::map<int, std::shared_ptr<MockMotor>> motors_;
};

// Mock Gripper Component
class GripperComponent {
public:
    GripperComponent() {
        // Initialize mock gripper motor
        motors_[9] = std::make_shared<MockMotor>(9, "Servo-Gripper");
        std::cout << "✅ GripperComponent initialized with servo motor m9" << std::endl;
    }

    bool enable_all() {
        std::cout << "\n🔌 Enabling gripper motor..." << std::endl;
        motors_[9]->enable();
        return true;
    }

    bool disable_all() {
        std::cout << "\n⭕ Disabling gripper motor..." << std::endl;
        motors_[9]->disable();
        return true;
    }

    bool open(double speed = 0.5, double force = 0.3) {
        double position = 0.5 + 0.5 * speed; // Map speed to position (simplified)
        std::cout << "\n✋ Opening gripper (speed=" << speed << ", force=" << force << ")" << std::endl;
        return motors_[9]->set_position_target(position, speed);
    }

    bool close(double speed = 0.5, double force = 0.5) {
        double position = 0.5 - 0.5 * speed; // Map speed to position (simplified)
        std::cout << "\n✊ Closing gripper (speed=" << speed << ", force=" << force << ")" << std::endl;
        return motors_[9]->set_position_target(position, speed);
    }

    bool set_openness(double openness, double speed = 0.5, double force = 0.3) {
        double position = openness;
        std::cout << "\n🎯 Setting gripper openness to "
                  << std::fixed << std::setprecision(1) << openness * 100
                  << "% (speed=" << speed << ", force=" << force << ")" << std::endl;
        return motors_[9]->set_position_target(position, speed);
    }

    double get_openness() const {
        double pos = motors_.at(9)->get_position();
        // Clamp between 0 and 1
        return std::max(0.0, std::min(1.0, pos));
    }

    void print_gripper_state() const {
        double openness = get_openness();
        std::cout << "\n📊 Gripper Component State:" << std::endl;
        std::cout << "  Openness: " << std::fixed << std::setprecision(1)
                  << openness * 100 << "%" << std::endl;
    }

private:
    std::map<int, std::shared_ptr<MockMotor>> motors_;
};

int main() {
    std::cout << "=== IC_CAN Wrist and Gripper Component Separation Demo ===" << std::endl;
    std::cout << "This demo demonstrates the separated wrist and gripper components:" << std::endl;
    std::cout << "  - WristComponent: Controls HT motors m7-m8 (pitch and roll)" << std::endl;
    std::cout << "  - GripperComponent: Controls servo motor m9 (gripper actuation)" << std::endl;
    std::cout << std::endl;

    try {
        // Initialize components
        std::cout << "🚀 Initializing components..." << std::endl;
        WristComponent wrist;
        GripperComponent gripper;
        std::cout << "✅ Components initialized successfully!" << std::endl;
        std::cout << std::endl;

        // Enable all motors
        wrist.enable_all();
        gripper.enable_all();

        std::cout << "\n=== WRIST CONTROL DEMO ===";
        std::cout << "\nTesting wrist pitch and roll movements...\n" << std::endl;

        // Test wrist pitch
        std::cout << "📍 Testing wrist pitch (motor m7):" << std::endl;
        double pitch_angles[] = {0.0, 0.5, -0.5, 0.0}; // radians

        for (double pitch : pitch_angles) {
            wrist.set_pitch_angle(pitch, 1.0);
            double current_pitch = wrist.get_pitch_angle();
            std::cout << "  → Current pitch: " << std::fixed << std::setprecision(2)
                      << current_pitch * 180.0 / M_PI << "°\n" << std::endl;
        }

        // Test wrist roll
        std::cout << "🔄 Testing wrist roll (motor m8):" << std::endl;
        double roll_angles[] = {0.0, 0.8, -0.8, 0.0}; // radians

        for (double roll : roll_angles) {
            wrist.set_roll_angle(roll, 1.0);
            double current_roll = wrist.get_roll_angle();
            std::cout << "  → Current roll: " << std::fixed << std::setprecision(2)
                      << current_roll * 180.0 / M_PI << "°\n" << std::endl;
        }

        // Test combined wrist motion
        std::cout << "🕺 Testing combined wrist motion:" << std::endl;
        wrist.set_wrist_pose(0.3, 0.4, 1.5);
        wrist.print_wrist_state();

        // Return to zero
        wrist.set_wrist_pose(0.0, 0.0, 1.0);

        std::cout << "\n=== GRIPPER CONTROL DEMO ===";
        std::cout << "\nTesting gripper open/close movements...\n" << std::endl;

        // Test gripper opening
        std::cout << "✋ Opening gripper..." << std::endl;
        gripper.open(0.6, 0.3);
        std::cout << "  → Current openness: " << std::fixed << std::setprecision(1)
                  << gripper.get_openness() * 100 << "%\n" << std::endl;

        // Test gripper closing
        std::cout << "✊ Closing gripper..." << std::endl;
        gripper.close(0.4, 0.5);
        std::cout << "  → Current openness: " << std::fixed << std::setprecision(1)
                  << gripper.get_openness() * 100 << "%\n" << std::endl;

        // Test specific positions
        std::cout << "🎯 Testing specific gripper positions:" << std::endl;
        double positions[] = {0.25, 0.5, 0.75, 0.5, 0.0};

        for (double pos : positions) {
            gripper.set_openness(pos, 0.5, 0.3);
            std::cout << "  → Actual openness: " << std::fixed << std::setprecision(1)
                      << gripper.get_openness() * 100 << "%\n" << std::endl;
        }

        std::cout << "\n=== COORDINATED MOTION DEMO ===";
        std::cout << "\nTesting simultaneous wrist and gripper movement...\n" << std::endl;

        // Open gripper and position wrist
        std::cout << "✋ Opening gripper and positioning wrist..." << std::endl;
        gripper.open(0.8, 0.2);
        wrist.set_wrist_pose(0.6, 0.2, 1.2);

        // Close gripper
        std::cout << "\n✊ Closing gripper to grasp..." << std::endl;
        gripper.close(0.5, 0.6);

        // Move wrist with object
        std::cout << "\n🏋️ Moving wrist with grasped object..." << std::endl;
        wrist.set_wrist_pose(-0.3, 1.0, 0.8);

        // Open gripper to release
        std::cout << "\n✋ Opening gripper to release..." << std::endl;
        gripper.open(0.7, 0.2);

        // Return to home
        std::cout << "\n🏠 Returning to home position..." << std::endl;
        wrist.set_wrist_pose(0.0, 0.0, 1.0);
        gripper.set_openness(0.5, 0.5, 0.3);

        // Print final status
        std::cout << "\n=== FINAL STATUS ===" << std::endl;
        wrist.print_wrist_state();
        std::cout << std::endl;
        gripper.print_gripper_state();

        // Component information
        std::cout << "\n=== COMPONENT SEPARATION SUMMARY ===" << std::endl;
        std::cout << "✅ WristComponent manages:" << std::endl;
        std::cout << "  - Motor m7: HT4438 (Wrist Pitch)" << std::endl;
        std::cout << "  - Motor m8: HT4438 (Wrist Roll)" << std::endl;
        std::cout << "\n✅ GripperComponent manages:" << std::endl;
        std::cout << "  - Motor m9: Servo (Gripper Actuation)" << std::endl;
        std::cout << "\n🎯 Benefits of separation:" << std::endl;
        std::cout << "  - Clear separation of concerns" << std::endl;
        std::cout << "  - Independent control of wrist orientation vs gripper actuation" << std::endl;
        std::cout << "  - More intuitive API for complex manipulation tasks" << std::endl;
        std::cout << "  - Easier to maintain and extend" << std::endl;

        // Disable all motors
        std::cout << "\n🔌 Disabling all motors..." << std::endl;
        wrist.disable_all();
        gripper.disable_all();

        std::cout << "\n✅ Demo completed successfully!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}