#!/usr/bin/env python3
# Copyright 2025 IC_CAN Team
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
IC_CAN Python Example

This example demonstrates basic usage of the IC_CAN library from Python.
"""

import sys
import time
from pathlib import Path

# Add the build directory to Python path for importing the module
sys.path.insert(0, str(Path(__file__).parent.parent / "build" / "python"))

try:
    import ic_can
except ImportError:
    print("Error: ic_can module not found. Please build the Python bindings first.")
    print("Run: cmake --build build --target ic_can_python")
    sys.exit(1)


def main():
    """Main example function"""
    print("=== IC_CAN Python Example ===")

    try:
        # Create IC_CAN controller with USB2CAN device
        controller = ic_can.IC_CAN("F561E08C892274DB09496BCC1102DBC5", enable_debug=True)

        # Initialize the system
        print("Initializing IC_CAN system...")
        if not controller.initialize():
            print("Failed to initialize IC_CAN system")
            return -1
        print("IC_CAN system initialized successfully")

        # Print system information
        controller.print_system_info()

        # Enable all motors
        print("Enabling all motors...")
        if not controller.enable_all():
            print("Failed to enable motors")
            return -1
        print("All motors enabled")

        # Wait for motors to be ready
        time.sleep(1.0)

        # Get current joint positions
        positions = controller.get_joint_positions()
        print(f"Current joint positions: {[f'j{i+1}={pos:.3f}' for i, pos in enumerate(positions)]}")

        # Set joint positions (small movement)
        target_positions = [0.1, -0.1, 0.1, -0.1, 0.1, -0.1, 0.0, 0.0, 0.0]
        print("Setting target positions...")
        if not controller.set_joint_positions(target_positions):
            print("Failed to set joint positions")

        # Wait for movement
        time.sleep(2.0)

        # Update and read positions again
        controller.update_all_states()
        positions = controller.get_joint_positions()
        print(f"New joint positions: {[f'j{i+1}={pos:.3f}' for i, pos in enumerate(positions)]}")

        # Start high-frequency control (500Hz)
        print("Starting high-frequency control...")
        if not controller.start_high_frequency_control():
            print("Failed to start high-frequency control")
        else:
            print("High-frequency control started")

            # Run control for a few seconds
            time.sleep(3.0)

            # Stop high-frequency control
            controller.stop_high_frequency_control()
            print("High-frequency control stopped")

        # Print safety status
        safety = controller.get_safety()
        safety.print_safety_status_report()

        # Disable all motors
        print("Disabling all motors...")
        controller.disable_all()
        print("All motors disabled")

        print("Example completed successfully!")

    except Exception as e:
        print(f"Error: {e}")
        return -1

    return 0


def test_arm_component():
    """Test arm component functionality"""
    print("\n=== Arm Component Test ===")

    try:
        controller = ic_can.IC_CAN("F561E08C892274DB09496BCC1102DBC5")

        if not controller.initialize():
            print("Failed to initialize")
            return

        # Get arm component reference
        arm = controller.get_arm()

        # Test MIT control
        print("Testing MIT control...")
        positions = [0.0, 0.5, -0.5, 0.2, -0.2, 0.0]
        velocities = [0.0, 0.1, 0.1, 0.1, 0.1, 0.0]
        kp_values = [20.0, 30.0, 30.0, 25.0, 25.0, 20.0]
        kd_values = [0.5, 0.8, 0.8, 0.6, 0.6, 0.5]

        if arm.mit_control(positions, velocities, kp_values, kd_values):
            print("MIT control command sent successfully")

        # Get arm state
        arm_state = arm.get_complete_state()
        print(f"Arm motors enabled: {arm_state['enabled']}")
        print(f"Arm positions: {[f'{p:.3f}' for p in arm_state['positions']]}")

    except Exception as e:
        print(f"Arm test error: {e}")


def test_gripper_component():
    """Test gripper component functionality"""
    print("\n=== Gripper Component Test ===")

    try:
        controller = ic_can.IC_CAN("F561E08C892274DB09496BCC1102DBC5")

        if not controller.initialize():
            print("Failed to initialize")
            return

        # Get gripper component reference
        gripper = controller.get_gripper()

        # Test gripper control
        print("Testing gripper control...")

        # Open gripper
        if gripper.open():
            print("Gripper opened")

        time.sleep(1.0)

        # Close gripper with force
        if gripper.close(10.0):
            print("Gripper closed with force")

        # Get gripper state
        gripper_state = gripper.get_complete_state()
        print(f"Gripper motors enabled: {gripper_state['enabled']}")
        print(f"Gripper positions: {[f'{p:.3f}' for p in gripper_state['positions']]}")

    except Exception as e:
        print(f"Gripper test error: {e}")


if __name__ == "__main__":
    exit_code = main()

    # Run additional component tests if main test succeeds
    if exit_code == 0:
        test_arm_component()
        test_gripper_component()

    sys.exit(exit_code)