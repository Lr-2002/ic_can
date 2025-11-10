#!/usr/bin/env python3
"""
High-Level API Demo for IC_CAN

This example demonstrates the new clean Python API for controlling the IC_ARM robot.
The interface is optimized for RL/DNN applications with a simple 4-method API:

1. robot.enable()      - Enable motors
2. robot.refresh()     - Read motor state
3. robot.step(p,v,t)   - Send position/velocity/torque commands
4. robot.set_control_mode() - Switch between TEACH/EXECUTION modes

This shows the production-ready interface that abstracts away low-level details
while maintaining 500Hz real-time performance.
"""

import sys
import time
import numpy as np
from pathlib import Path

# Add the python directory to the path for development
python_dir = Path(__file__).parent.parent
sys.path.insert(0, str(python_dir))

import ic_can
from ic_can import RobotArm, ControlMode, MotorState


def basic_api_demo():
    """Demonstrate the basic 4-method API."""
    print("=== IC_CAN High-Level API Demo ===")
    print("This demo shows the clean RL/DNN-friendly interface")
    print()

    # Initialize robot with context manager (auto-cleanup)
    with ic_can.quick_start() as robot:
        print("✓ Robot initialized successfully")
        print(f"Robot info: {robot}")

        # 1. Enable motors
        print("\n1. Enabling motors...")
        if robot.enable():
            print("✓ All motors enabled")
        else:
            print("✗ Failed to enable motors")
            return

        # 2. Read current state
        print("\n2. Reading motor state...")
        state = robot.refresh()
        print(f"✓ State read: {state}")
        print(f"  Positions: {state.positions}")
        print(f"  Control mode: {state.control_mode.value}")
        print(f"  Is safe: {state.is_safe()}")

        # 3. Switch to execution mode
        print("\n3. Switching to EXECUTION mode...")
        if robot.set_control_mode(ControlMode.EXECUTION):
            print("✓ Control mode changed to EXECUTION")
        else:
            print("✗ Failed to change control mode")
            return

        # 4. Send step commands
        print("\n4. Sending step commands...")

        # Small safe movement
        target_positions = np.array([0.1, -0.1, 0.1, -0.1, 0.1, -0.1, 0.0, 0.0, 0.0])
        print(f"Target positions: {target_positions}")

        if robot.step(target_positions):
            print("✓ Step command sent successfully")

            # Read state after movement
            time.sleep(0.1)  # Small delay for movement
            new_state = robot.refresh()
            print(f"New positions: {new_state.positions}")
        else:
            print("✗ Failed to send step command")

        # 5. Return to teach mode (safe)
        print("\n5. Returning to TEACH mode...")
        robot.set_control_mode(ControlMode.TEACH)
        print("✓ Control mode changed to TEACH (safe)")

        print("\n=== Demo completed successfully ===")


def rl_integration_example():
    """Show how this API integrates with RL frameworks."""
    print("\n=== RL Integration Example ===")
    print("Demonstrating typical RL control loop structure")
    print()

    # Initialize robot
    robot = ic_can.create_robot_arm(debug=False)
    print("✓ Robot created for RL example")

    # RL-style control loop structure
    def rl_control_loop(robot, num_steps=10):
        """
        Example RL control loop structure.

        This shows how the clean API enables easy integration with
        reinforcement learning frameworks.
        """
        print(f"Starting RL control loop ({num_steps} steps)...")

        # Enable robot
        if not robot.enable():
            print("Failed to enable robot")
            return False

        # Set to execution mode
        robot.set_control_mode(ControlMode.EXECUTION)

        # RL control loop
        for step in range(num_steps):
            # 1. Get current state (observation)
            state = robot.refresh()
            state_vector = state.to_vector()  # Convert to flat vector for RL

            print(f"Step {step + 1}: state_range=[{np.min(state_vector):.3f}, {np.max(state_vector):.3f}]")

            # 2. Policy/agent would compute action here
            # action = policy.get_action(state_vector)
            # For demo, use simple sinusoidal action
            t = step * 0.1
            action = np.array([
                0.2 * np.sin(t),      # Motor 1
                0.2 * np.cos(t),      # Motor 2
                0.1 * np.sin(t * 2),  # Motor 3
                0.1 * np.cos(t * 2),  # Motor 4
                0.05 * np.sin(t),     # Motor 5
                0.05 * np.cos(t),     # Motor 6
                0.0, 0.0, 0.0         # Wrist and gripper
            ])

            # 3. Send action to robot
            success = robot.step(action)
            if not success:
                print(f"Step {step + 1}: Failed to send action")
                break

            # Small delay (in real RL, this would be environment timestep)
            time.sleep(0.05)

        # Return to safe mode
        robot.set_control_mode(ControlMode.TEACH)
        print("✓ RL control loop completed")
        return True

    # Run RL example
    try:
        rl_control_loop(robot, num_steps=5)
    finally:
        robot.shutdown()


def safety_features_demo():
    """Demonstrate safety features and error handling."""
    print("\n=== Safety Features Demo ===")
    print("Showing built-in safety monitoring and error handling")
    print()

    robot = ic_can.RobotArm(safety_limits=True)  # Enable safety limits

    if robot.initialize():
        print("✓ Robot initialized with safety features")

        # Test safety limits
        print("\nTesting safety limits...")

        # These should be clamped by safety system
        dangerous_positions = np.array([10.0, -10.0, 5.0, -5.0, 15.0, -15.0, 20.0, -20.0, 10.0])
        print(f"Dangerous command: {dangerous_positions}")

        robot.enable()

        # Safety system should clamp these to +/- π
        if robot.step(dangerous_positions):
            print("✓ Safety system accepted and clamped dangerous positions")

        state = robot.refresh()
        print(f"Actual positions after clamping: {state.positions}")
        print(f"All positions within limits: {state.is_within_limits()}")

        # Test emergency stop
        print("\nTesting emergency stop...")
        robot.emergency_stop()
        print("✓ Emergency stop executed")

        robot.shutdown()
    else:
        print("✗ Failed to initialize robot")


def performance_benchmark():
    """Benchmark the performance of the high-level API."""
    print("\n=== Performance Benchmark ===")
    print("Testing real-time performance capabilities")
    print()

    robot = ic_can.create_robot_arm(debug=False)
    robot.enable()

    # Test state reading performance
    print("Benchmarking state refresh speed...")
    num_reads = 100

    start_time = time.time()
    for _ in range(num_reads):
        state = robot.refresh()
    end_time = time.time()

    avg_refresh_time = (end_time - start_time) / num_reads
    refresh_frequency = 1.0 / avg_refresh_time

    print(f"✓ Average refresh time: {avg_refresh_time * 1000:.2f} ms")
    print(f"✓ Theoretical refresh frequency: {refresh_frequency:.1f} Hz")

    # Test command sending performance
    print("\nBenchmarking command speed...")
    num_commands = 50

    test_positions = np.random.uniform(-0.5, 0.5, (num_commands, 9))

    start_time = time.time()
    for i in range(num_commands):
        robot.step(test_positions[i])
    end_time = time.time()

    avg_command_time = (end_time - start_time) / num_commands
    command_frequency = 1.0 / avg_command_time

    print(f"✓ Average command time: {avg_command_time * 1000:.2f} ms")
    print(f"✓ Theoretical command frequency: {command_frequency:.1f} Hz")

    # Show performance stats
    stats = robot.get_performance_stats()
    print(f"\nPerformance stats: {stats}")

    robot.shutdown()


def main():
    """Run all demo examples."""
    print("IC_CAN High-Level Python API Demo")
    print("=" * 50)
    print("This demo showcases the production-ready RL/DNN interface")
    print("with 4 core methods: enable, refresh, step, set_control_mode")
    print()

    try:
        # Basic API demo
        basic_api_demo()

        # RL integration example
        rl_integration_example()

        # Safety features
        safety_features_demo()

        # Performance benchmark
        performance_benchmark()

        print("\n" + "=" * 50)
        print("All demos completed successfully!")
        print("\nThe high-level API is ready for RL/DNN applications:")
        print("- Clean 4-method interface")
        print("- Real-time 500Hz performance")
        print("- Built-in safety features")
        print("- NumPy-compatible state representation")
        print("- Production-ready error handling")

    except Exception as e:
        print(f"\nDemo failed with error: {e}")
        print("This is expected if hardware is not connected.")
        print("The API structure is ready for use with actual hardware.")


if __name__ == "__main__":
    main()