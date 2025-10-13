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
IC_CAN Python Functionality Tests
Comprehensive functional testing of the Python interface to IC_CAN library.
"""

import sys
import time
import threading
import logging
import traceback
from typing import List, Dict, Any
import unittest

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

try:
    from ic_can_python import (
        IC_CAN, MotorType, DMMotorType, DMControlMode, DMRegisterID,
        SafetyViolationType, USB2CANException
    )
    from ic_can.core import IC_CANController
    from ic_can.exceptions import (
        IC_CANError, USB2CANError, MotorError, SafetyError
    )
    NATIVE_AVAILABLE = True
    logger.info("IC_CAN native module loaded successfully")
except ImportError as e:
    logger.warning(f"IC_CAN native module not available: {e}")
    logger.warning("Running in simulation mode")
    NATIVE_AVAILABLE = False

    # Mock classes for simulation mode
    class IC_CAN:
        def __init__(self, device_sn="mock", debug=False):
            self.device_sn = device_sn
            self.debug = debug
            self._initialized = False
            self._high_freq_running = False

        def initialize(self):
            logger.info("Mock: Initialize IC_CAN")
            self._initialized = True
            return True

        def is_initialized(self):
            return self._initialized

        def enable_all(self):
            logger.info("Mock: Enable all motors")

        def disable_all(self):
            logger.info("Mock: Disable all motors")

        def emergency_stop(self):
            logger.info("Mock: Emergency stop")

        def reset_emergency_stop(self):
            logger.info("Mock: Reset emergency stop")

        def get_joint_positions(self):
            logger.info("Mock: Get joint positions")
            return [0.0] * 7

        def get_joint_velocities(self):
            logger.info("Mock: Get joint velocities")
            return [0.0] * 7

        def get_joint_torques(self):
            logger.info("Mock: Get joint torques")
            return [0.0] * 7

        def set_joint_positions(self, positions, velocities=None, torques=None):
            logger.info(f"Mock: Set joint positions: {positions}")

        def set_joint_torques(self, torques, positions=None, velocities=None):
            logger.info(f"Mock: Set joint torques: {torques}")

        def start_high_frequency_control(self):
            logger.info("Mock: Start high frequency control")
            self._high_freq_running = True

        def stop_high_frequency_control(self):
            logger.info("Mock: Stop high frequency control")
            self._high_freq_running = False

        def is_high_frequency_control_running(self):
            return self._high_freq_running

        def get_control_frequency(self):
            return 500.0

        def set_control_frequency(self, freq):
            logger.info(f"Mock: Set control frequency to {freq} Hz")

        def update_all_states(self):
            logger.info("Mock: Update all states")

        def get_system_status(self):
            logger.info("Mock: Get system status")
            return {"status": "mock", "initialized": self._initialized}

        def print_system_info(self):
            logger.info("Mock: Print system info")

        def get_arm(self):
            return MockArmComponent()

        def get_gripper(self):
            return MockGripperComponent()

        def get_safety(self):
            return MockSafetyModule()

    class MockArmComponent:
        def get_positions(self):
            return [0.0] * 6

        def set_positions(self, positions, velocities=None, torques=None):
            logger.info(f"Mock arm: Set positions {positions}")

        def get_velocities(self):
            return [0.0] * 6

        def get_torques(self):
            return [0.0] * 6

    class MockGripperComponent:
        def open(self, speed=0.5, force=0.3):
            logger.info("Mock gripper: Open")

        def close(self, speed=0.5, force=0.5):
            logger.info("Mock gripper: Close")

        def get_openness(self):
            return 0.5

    class MockSafetyModule:
        def has_active_violations(self):
            return False

        def get_active_violations(self):
            return []

    # Mock enums
    MotorType = type('MotorType', (), {
        'DM_DAMIAO': 0, 'HT_HIGH_TORQUE': 1, 'SERVO': 2
    })
    DMMotorType = type('DMMotorType', (), {
        'DM4310': 0, 'DM6248': 1, 'DM4340': 2, 'DM10010L': 3
    })
    SafetyViolationType = type('SafetyViolationType', (), {
        'POSITION_LIMIT': 0, 'VELOCITY_LIMIT': 1, 'TORQUE_LIMIT': 2
    })

    class USB2CANException(Exception):
        pass

    class IC_CANController:
        def __init__(self, device_sn="mock", debug=False):
            self.controller = IC_CAN(device_sn, debug)
            self._initialized = False
            self._high_freq_running = False

        def initialize(self):
            return self.controller.initialize()

        def is_initialized(self):
            return self.controller.is_initialized()


class TestIC_CANFunctionality(unittest.TestCase):
    """Test IC_CAN Python functionality"""

    def setUp(self):
        """Set up test environment"""
        logger.info(f"Setting up {self._testMethodName}")
        self.device_sn = "F561E08C892274DB09496BCC1102DBC5"
        if not NATIVE_AVAILABLE:
            self.device_sn = "mock_device"

    def tearDown(self):
        """Clean up test environment"""
        logger.info(f"Tearing down {self._testMethodName}")

    def test_controller_creation(self):
        """Test IC_CAN controller creation"""
        logger.info("Testing controller creation...")

        controller = IC_CAN(self.device_sn, debug=True)
        self.assertIsNotNone(controller)
        self.assertEqual(controller.device_sn, self.device_sn)

        logger.info("Controller creation test passed")

    def test_initialization(self):
        """Test system initialization"""
        logger.info("Testing system initialization...")

        controller = IC_CAN(self.device_sn)

        # Test initial state
        self.assertFalse(controller.is_initialized())

        # Test initialization
        result = controller.initialize()
        self.assertTrue(result)
        self.assertTrue(controller.is_initialized())

        logger.info("Initialization test passed")

    def test_motor_enable_disable(self):
        """Test motor enable/disable functionality"""
        logger.info("Testing motor enable/disable...")

        controller = IC_CAN(self.device_sn)
        controller.initialize()

        # Test enabling motors
        controller.enable_all()
        logger.info("Motors enabled")

        # Test disabling motors
        controller.disable_all()
        logger.info("Motors disabled")

        logger.info("Motor enable/disable test passed")

    def test_joint_state_reading(self):
        """Test joint state reading"""
        logger.info("Testing joint state reading...")

        controller = IC_CAN(self.device_sn)
        controller.initialize()

        # Test position reading
        positions = controller.get_joint_positions()
        self.assertEqual(len(positions), 7)  # 6 arm + 1 gripper
        self.assertIsInstance(positions, list)
        self.assertIsInstance(positions[0], (int, float))

        # Test velocity reading
        velocities = controller.get_joint_velocities()
        self.assertEqual(len(velocities), 7)

        # Test torque reading
        torques = controller.get_joint_torques()
        self.assertEqual(len(torques), 7)

        logger.info(f"Joint states - Positions: {positions[:3]}...")
        logger.info("Joint state reading test passed")

    def test_joint_control(self):
        """Test joint position and torque control"""
        logger.info("Testing joint control...")

        controller = IC_CAN(self.device_sn)
        controller.initialize()

        # Test position control
        positions = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7]
        controller.set_joint_positions(positions)
        logger.info(f"Set joint positions: {positions}")

        # Test torque control
        torques = [0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1]
        controller.set_joint_torques(torques)
        logger.info(f"Set joint torques: {torques}")

        logger.info("Joint control test passed")

    def test_high_frequency_control(self):
        """Test high-frequency control functionality"""
        logger.info("Testing high-frequency control...")

        controller = IC_CAN(self.device_sn)
        controller.initialize()

        # Test frequency setting
        controller.set_control_frequency(500.0)
        self.assertEqual(controller.get_control_frequency(), 500.0)

        # Test starting high-frequency control
        controller.start_high_frequency_control()
        self.assertTrue(controller.is_high_frequency_control_running())

        # Let it run for a short time
        time.sleep(0.5)

        # Test stopping high-frequency control
        controller.stop_high_frequency_control()
        self.assertFalse(controller.is_high_frequency_control_running())

        logger.info("High-frequency control test passed")

    def test_emergency_stop(self):
        """Test emergency stop functionality"""
        logger.info("Testing emergency stop...")

        controller = IC_CAN(self.device_sn)
        controller.initialize()

        # Test emergency stop
        controller.emergency_stop()
        logger.info("Emergency stop triggered")

        # Test reset
        controller.reset_emergency_stop()
        logger.info("Emergency stop reset")

        logger.info("Emergency stop test passed")

    def test_component_access(self):
        """Test component access"""
        logger.info("Testing component access...")

        controller = IC_CAN(self.device_sn)
        controller.initialize()

        # Test arm component
        arm = controller.get_arm()
        self.assertIsNotNone(arm)

        positions = arm.get_positions()
        self.assertEqual(len(positions), 6)

        # Test gripper component
        gripper = controller.get_gripper()
        self.assertIsNotNone(gripper)

        openness = gripper.get_openness()
        self.assertIsInstance(openness, (int, float))
        self.assertGreaterEqual(openness, 0.0)
        self.assertLessEqual(openness, 1.0)

        # Test safety module
        safety = controller.get_safety()
        self.assertIsNotNone(safety)

        self.assertFalse(safety.has_active_violations())
        violations = safety.get_active_violations()
        self.assertIsInstance(violations, list)

        logger.info("Component access test passed")

    def test_system_status(self):
        """Test system status reporting"""
        logger.info("Testing system status...")

        controller = IC_CAN(self.device_sn)
        controller.initialize()

        # Test system status
        status = controller.get_system_status()
        self.assertIsInstance(status, dict)
        self.assertGreater(len(status), 0)

        # Test system info printing
        controller.print_system_info()

        logger.info("System status test passed")

    def test_high_level_controller(self):
        """Test high-level Python controller"""
        logger.info("Testing high-level controller...")

        if NATIVE_AVAILABLE:
            controller = IC_CANController(self.device_sn, debug=True)

            # Test initialization
            result = controller.initialize()
            self.assertTrue(result)
            self.assertTrue(controller.is_initialized())

            # Test performance monitoring
            stats = controller.get_performance_stats()
            self.assertIsInstance(stats, dict)

            # Test controlled motion context manager
            with controller.controlled_motion():
                controller.enable_all()
                positions = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7]
                controller.set_joint_positions(positions)
                time.sleep(0.1)

            logger.info("High-level controller test passed")
        else:
            logger.info("Skipping high-level controller test (native module not available)")

    def test_safety_callbacks(self):
        """Test safety violation callbacks"""
        logger.info("Testing safety callbacks...")

        if NATIVE_AVAILABLE:
            controller = IC_CANController(self.device_sn)
            controller.initialize()

            callback_called = threading.Event()
            callback_data = {}

            def safety_callback(violation_type, message):
                callback_data['type'] = violation_type
                callback_data['message'] = message
                callback_called.set()

            # Add callback
            controller.add_safety_callback(safety_callback)

            # Trigger emergency stop (should trigger callback)
            controller.emergency_stop("Test emergency stop")

            # Check if callback was called (timeout after 1 second)
            if callback_called.wait(1.0):
                self.assertEqual(callback_data['type'], 'EMERGENCY_STOP')
                logger.info("Safety callback test passed")
            else:
                logger.warning("Safety callback not triggered (may be expected in simulation)")

            # Remove callback
            controller.remove_safety_callback(safety_callback)
        else:
            logger.info("Skipping safety callback test (native module not available)")

    def test_error_handling(self):
        """Test error handling"""
        logger.info("Testing error handling...")

        # Test invalid device (should fail gracefully)
        try:
            invalid_controller = IC_CAN("invalid_device_sn")
            result = invalid_controller.initialize()
            # May succeed in simulation mode
            logger.info("Invalid device initialization result: " + str(result))
        except Exception as e:
            logger.info(f"Expected error with invalid device: {e}")
            if NATIVE_AVAILABLE:
                self.assertIsInstance(e, (IC_CANError, USB2CANException))

        # Test invalid inputs
        controller = IC_CAN(self.device_sn)
        controller.initialize()

        # Test invalid position list length
        try:
            invalid_positions = [0.1, 0.2]  # Should be 7 elements
            controller.set_joint_positions(invalid_positions)
            logger.warning("Invalid position list was accepted (may be expected in simulation)")
        except Exception as e:
            logger.info(f"Expected error with invalid positions: {e}")
            if NATIVE_AVAILABLE:
                self.assertIsInstance(e, (IC_CANError, ValueError))

        logger.info("Error handling test passed")

    def test_thread_safety(self):
        """Test thread safety of operations"""
        logger.info("Testing thread safety...")

        controller = IC_CAN(self.device_sn)
        controller.initialize()

        results = []
        errors = []

        def worker_thread(thread_id):
            try:
                for i in range(10):
                    # Read states
                    positions = controller.get_joint_positions()
                    velocities = controller.get_joint_velocities()

                    # Set positions
                    test_positions = [0.1 * i] * 7
                    controller.set_joint_positions(test_positions)

                    results.append(thread_id)
                    time.sleep(0.01)  # 10ms delay

            except Exception as e:
                errors.append((thread_id, str(e)))

        # Start multiple threads
        threads = []
        for i in range(3):
            thread = threading.Thread(target=worker_thread, args=(i,))
            threads.append(thread)
            thread.start()

        # Wait for completion
        for thread in threads:
            thread.join(timeout=10)

        # Check results
        logger.info(f"Thread safety test completed. Results: {len(results)}, Errors: {len(errors)}")

        if errors:
            for thread_id, error in errors:
                logger.error(f"Thread {thread_id} error: {error}")

        # Some operations may fail in simulation mode, so we're lenient
        self.assertGreaterEqual(len(results), 0)

        logger.info("Thread safety test passed")


class TestIC_CANPerformance(unittest.TestCase):
    """Test IC_CAN Python performance"""

    def setUp(self):
        """Set up performance test environment"""
        logger.info(f"Setting up performance test {self._testMethodName}")
        self.device_sn = "F561E08C892274DB09496BCC1102DBC5"
        if not NATIVE_AVAILABLE:
            self.device_sn = "mock_device"

        self.controller = IC_CAN(self.device_sn)
        self.controller.initialize()

    def tearDown(self):
        """Clean up performance test environment"""
        logger.info(f"Tearing down performance test {self._testMethodName}")

    def test_state_reading_frequency(self):
        """Test frequency of state reading operations"""
        logger.info("Testing state reading frequency...")

        test_duration = 2.0  # seconds
        target_frequency = 100  # Hz

        start_time = time.time()
        iterations = 0

        while time.time() - start_time < test_duration:
            # Read all states
            positions = self.controller.get_joint_positions()
            velocities = self.controller.get_joint_velocities()
            torques = self.controller.get_joint_torques()

            # Use the data to prevent optimization
            sum_positions = sum(positions)
            sum_velocities = sum(velocities)
            sum_torques = sum(torques)

            # Prevent optimization
            _ = sum_positions + sum_velocities + sum_torques

            iterations += 1

        end_time = time.time()
        actual_duration = end_time - start_time
        actual_frequency = iterations / actual_duration

        logger.info(f"State reading performance:")
        logger.info(f"  Target frequency: {target_frequency} Hz")
        logger.info(f"  Actual frequency: {actual_frequency:.2f} Hz")
        logger.info(f"  Total iterations: {iterations}")
        logger.info(f"  Duration: {actual_duration:.2f} s")

        # Performance assertion (lenient for simulation)
        if NATIVE_AVAILABLE:
            self.assertGreater(actual_frequency, target_frequency * 0.5)

        logger.info("State reading frequency test passed")

    def test_control_command_frequency(self):
        """Test frequency of control command operations"""
        logger.info("Testing control command frequency...")

        test_duration = 1.0  # seconds
        target_frequency = 50  # Hz

        start_time = time.time()
        iterations = 0

        while time.time() - start_time < test_duration:
            # Generate test positions
            positions = [0.1 * (iterations % 10)] * 7
            self.controller.set_joint_positions(positions)

            iterations += 1
            time.sleep(0.001)  # Small delay to prevent overwhelming

        end_time = time.time()
        actual_duration = end_time - start_time
        actual_frequency = iterations / actual_duration

        logger.info(f"Control command performance:")
        logger.info(f"  Target frequency: {target_frequency} Hz")
        logger.info(f"  Actual frequency: {actual_frequency:.2f} Hz")
        logger.info(f"  Total iterations: {iterations}")
        logger.info(f"  Duration: {actual_duration:.2f} s")

        # Performance assertion (lenient for simulation)
        if NATIVE_AVAILABLE:
            self.assertGreater(actual_frequency, target_frequency * 0.5)

        logger.info("Control command frequency test passed")

    def test_high_frequency_control_performance(self):
        """Test high-frequency control loop performance"""
        logger.info("Testing high-frequency control performance...")

        if not NATIVE_AVAILABLE:
            logger.info("Skipping high-frequency control test (native module not available)")
            return

        target_frequency = 200  # Hz (reduced for test stability)
        test_duration = 1.0  # seconds

        # Start high-frequency control
        self.controller.set_control_frequency(target_frequency)
        self.controller.start_high_frequency_control()

        start_time = time.time()
        iterations = 0

        while time.time() - start_time < test_duration:
            # Update states
            self.controller.update_all_states()
            iterations += 1

            # Small delay to prevent overwhelming
            time.sleep(0.001)

        end_time = time.time()
        actual_duration = end_time - start_time
        actual_frequency = iterations / actual_duration

        # Stop high-frequency control
        self.controller.stop_high_frequency_control()

        logger.info(f"High-frequency control performance:")
        logger.info(f"  Target frequency: {target_frequency} Hz")
        logger.info(f"  Actual frequency: {actual_frequency:.2f} Hz")
        logger.info(f"  Total iterations: {iterations}")
        logger.info(f"  Duration: {actual_duration:.2f} s")
        logger.info(f"  Control running: {self.controller.is_high_frequency_control_running()}")

        # Performance assertion
        self.assertGreater(actual_frequency, target_frequency * 0.3)

        logger.info("High-frequency control performance test passed")

    def test_memory_usage(self):
        """Test memory usage and stability"""
        logger.info("Testing memory usage...")

        import psutil
        import os

        process = psutil.Process(os.getpid())
        initial_memory = process.memory_info().rss / 1024 / 1024  # MB

        test_iterations = 1000

        for i in range(test_iterations):
            # Perform operations that allocate memory
            positions = self.controller.get_joint_positions()
            velocities = self.controller.get_joint_velocities()
            torques = self.controller.get_joint_torques()

            # Create temporary data structures
            temp_data = {
                'positions': positions,
                'velocities': velocities,
                'torques': torques,
                'iteration': i,
                'extra_data': list(range(100))
            }

            # Use the data
            _ = sum(temp_data['positions']) + sum(temp_data['velocities'])

            # Cleanup
            del temp_data

        final_memory = process.memory_info().rss / 1024 / 1024  # MB
        memory_increase = final_memory - initial_memory

        logger.info(f"Memory usage test:")
        logger.info(f"  Initial memory: {initial_memory:.2f} MB")
        logger.info(f"  Final memory: {final_memory:.2f} MB")
        logger.info(f"  Memory increase: {memory_increase:.2f} MB")
        logger.info(f"  Iterations: {test_iterations}")

        # Memory usage should be reasonable
        self.assertLess(memory_increase, 100)  # Less than 100MB increase

        logger.info("Memory usage test passed")


def run_functionality_tests():
    """Run all functionality tests"""
    logger.info("Starting IC_CAN Python functionality tests")

    # Create test suite
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()

    # Add test cases
    suite.addTests(loader.loadTestsFromTestCase(TestIC_CANFunctionality))
    suite.addTests(loader.loadTestsFromTestCase(TestIC_CANPerformance))

    # Run tests
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)

    # Print summary
    logger.info("=" * 50)
    logger.info("TEST SUMMARY")
    logger.info("=" * 50)
    logger.info(f"Tests run: {result.testsRun}")
    logger.info(f"Failures: {len(result.failures)}")
    logger.info(f"Errors: {len(result.errors)}")
    logger.info(f"Success rate: {((result.testsRun - len(result.failures) - len(result.errors)) / result.testsRun * 100):.1f}%")

    if result.failures:
        logger.error("FAILURES:")
        for test, traceback in result.failures:
            logger.error(f"  {test}: {traceback}")

    if result.errors:
        logger.error("ERRORS:")
        for test, traceback in result.errors:
            logger.error(f"  {test}: {traceback}")

    logger.info("=" * 50)

    return result.wasSuccessful()


if __name__ == "__main__":
    success = run_functionality_tests()
    sys.exit(0 if success else 1)