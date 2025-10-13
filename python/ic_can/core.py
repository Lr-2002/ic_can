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
IC_CAN Core Controller
High-level Python interface for IC_CAN robotic arm control.

This module provides a user-friendly Python wrapper around the C++ IC_CAN library,
offering simplified APIs for common robotic control tasks.
"""

import time
import logging
import threading
from typing import List, Optional, Tuple, Dict, Any, Callable
from contextlib import contextmanager

try:
    from .ic_can_python import IC_CAN, MotorType, DMMotorType, DMControlMode, SafetyViolationType
    from .exceptions import IC_CANError, SafetyError
except ImportError:
    # Fallback for development mode
    IC_CAN = None
    MotorType = None
    DMMotorType = None
    DMControlMode = None
    SafetyViolationType = None
    IC_CANError = Exception
    SafetyError = Exception

# Configure logging
logger = logging.getLogger(__name__)


class IC_CANController:
    """
    High-level Python controller for IC_CAN robotic arms.

    This class provides a simplified interface for controlling robotic arms
    with the IC_CAN library, including automatic initialization, safety monitoring,
    and high-frequency control capabilities.
    """

    def __init__(self, device_sn: str = "F561E08C892274DB09496BCC1102DBC5", debug: bool = False):
        """
        Initialize IC_CAN controller.

        Args:
            device_sn: USB2CAN device serial number
            debug: Enable debug logging
        """
        self.device_sn = device_sn
        self.debug = debug

        # Configure logging
        if debug:
            logging.basicConfig(level=logging.DEBUG)
            logger.setLevel(logging.DEBUG)

        # Initialize C++ controller
        if IC_CAN is None:
            raise IC_CANError("IC_CAN C++ module not available. Please check installation.")

        self.controller = IC_CAN(device_sn, debug)
        self._initialized = False
        self._high_freq_running = False
        self._safety_callbacks = []

        # Performance monitoring
        self._performance_stats = {
            'control_loop_count': 0,
            'last_update_time': 0,
            'average_frequency': 0.0,
            'min_frequency': float('inf'),
            'max_frequency': 0.0
        }

        logger.info(f"IC_CAN controller initialized with device: {device_sn}")

    def initialize(self) -> bool:
        """
        Initialize the controller and connect to hardware.

        Returns:
            True if initialization successful, False otherwise
        """
        try:
            result = self.controller.initialize()
            self._initialized = result

            if result:
                logger.info("IC_CAN controller successfully initialized")
                # Start monitoring thread
                self._start_monitoring()
            else:
                logger.error("Failed to initialize IC_CAN controller")

            return result
        except Exception as e:
            logger.error(f"Initialization error: {e}")
            raise IC_CANError(f"Failed to initialize: {e}")

    def is_initialized(self) -> bool:
        """Check if controller is initialized."""
        return self._initialized and self.controller.is_initialized()

    def enable_all(self) -> None:
        """Enable all motors."""
        if not self.is_initialized():
            raise IC_CANError("Controller not initialized")

        try:
            self.controller.enable_all()
            logger.info("All motors enabled")
        except Exception as e:
            logger.error(f"Failed to enable motors: {e}")
            raise IC_CANError(f"Failed to enable motors: {e}")

    def disable_all(self) -> None:
        """Disable all motors."""
        try:
            self.controller.disable_all()
            logger.info("All motors disabled")
        except Exception as e:
            logger.error(f"Failed to disable motors: {e}")
            raise IC_CANError(f"Failed to disable motors: {e}")

    def emergency_stop(self, reason: str = "Manual emergency stop") -> None:
        """
        Trigger emergency stop.

        Args:
            reason: Reason for emergency stop
        """
        try:
            self.controller.emergency_stop()
            logger.warning(f"Emergency stop triggered: {reason}")
            self._notify_safety_violation("EMERGENCY_STOP", reason)
        except Exception as e:
            logger.error(f"Emergency stop failed: {e}")
            raise IC_CANError(f"Emergency stop failed: {e}")

    def reset_emergency_stop(self) -> None:
        """Reset emergency stop state."""
        try:
            self.controller.reset_emergency_stop()
            logger.info("Emergency stop reset")
        except Exception as e:
            logger.error(f"Failed to reset emergency stop: {e}")
            raise IC_CANError(f"Failed to reset emergency stop: {e}")

    def get_joint_positions(self) -> List[float]:
        """
        Get current joint positions in radians.

        Returns:
            List of joint positions (6 arm joints + gripper)
        """
        if not self.is_initialized():
            raise IC_CANError("Controller not initialized")

        try:
            positions = self.controller.get_joint_positions()
            self._update_performance_stats()
            return positions
        except Exception as e:
            logger.error(f"Failed to get joint positions: {e}")
            raise IC_CANError(f"Failed to get joint positions: {e}")

    def get_joint_velocities(self) -> List[float]:
        """
        Get current joint velocities in rad/s.

        Returns:
            List of joint velocities
        """
        if not self.is_initialized():
            raise IC_CANError("Controller not initialized")

        try:
            velocities = self.controller.get_joint_velocities()
            return velocities
        except Exception as e:
            logger.error(f"Failed to get joint velocities: {e}")
            raise IC_CANError(f"Failed to get joint velocities: {e}")

    def get_joint_torques(self) -> List[float]:
        """
        Get current joint torques in N·m.

        Returns:
            List of joint torques
        """
        if not self.is_initialized():
            raise IC_CANError("Controller not initialized")

        try:
            torques = self.controller.get_joint_torques()
            return torques
        except Exception as e:
            logger.error(f"Failed to get joint torques: {e}")
            raise IC_CANError(f"Failed to get joint torques: {e}")

    def set_joint_positions(self, positions: List[float],
                          velocities: Optional[List[float]] = None,
                          torques: Optional[List[float]] = None) -> None:
        """
        Set joint positions.

        Args:
            positions: Target positions in radians
            velocities: Optional velocity limits
            torques: Optional torque limits
        """
        if not self.is_initialized():
            raise IC_CANError("Controller not initialized")

        if len(positions) != 7:  # 6 arm joints + gripper
            raise IC_CANError(f"Expected 7 positions, got {len(positions)}")

        try:
            self.controller.set_joint_positions(
                positions,
                velocities or [],
                torques or []
            )
            logger.debug(f"Set joint positions: {positions}")
        except Exception as e:
            logger.error(f"Failed to set joint positions: {e}")
            raise IC_CANError(f"Failed to set joint positions: {e}")

    def set_joint_torques(self, torques: List[float],
                         positions: Optional[List[float]] = None,
                         velocities: Optional[List[float]] = None) -> None:
        """
        Set joint torques.

        Args:
            torques: Target torques in N·m
            positions: Optional position limits
            velocities: Optional velocity limits
        """
        if not self.is_initialized():
            raise IC_CANError("Controller not initialized")

        try:
            self.controller.set_joint_torques(
                torques,
                positions or [],
                velocities or []
            )
            logger.debug(f"Set joint torques: {torques}")
        except Exception as e:
            logger.error(f"Failed to set joint torques: {e}")
            raise IC_CANError(f"Failed to set joint torques: {e}")

    def start_high_frequency_control(self, frequency_hz: float = 500.0) -> None:
        """
        Start high-frequency control thread.

        Args:
            frequency_hz: Control frequency in Hz
        """
        if not self.is_initialized():
            raise IC_CANError("Controller not initialized")

        if self._high_freq_running:
            logger.warning("High-frequency control already running")
            return

        try:
            self.controller.set_control_frequency(frequency_hz)
            self.controller.start_high_frequency_control()
            self._high_freq_running = True
            logger.info(f"Started high-frequency control at {frequency_hz} Hz")
        except Exception as e:
            logger.error(f"Failed to start high-frequency control: {e}")
            raise IC_CANError(f"Failed to start high-frequency control: {e}")

    def stop_high_frequency_control(self) -> None:
        """Stop high-frequency control thread."""
        if not self._high_freq_running:
            logger.warning("High-frequency control not running")
            return

        try:
            self.controller.stop_high_frequency_control()
            self._high_freq_running = False
            logger.info("Stopped high-frequency control")
        except Exception as e:
            logger.error(f"Failed to stop high-frequency control: {e}")
            raise IC_CANError(f"Failed to stop high-frequency control: {e}")

    def is_high_frequency_control_running(self) -> bool:
        """Check if high-frequency control is running."""
        return self._high_freq_running and self.controller.is_high_frequency_control_running()

    def get_control_frequency(self) -> float:
        """Get current control frequency in Hz."""
        try:
            return self.controller.get_control_frequency()
        except Exception as e:
            logger.error(f"Failed to get control frequency: {e}")
            return 0.0

    def update_all_states(self) -> None:
        """Update all motor states."""
        if not self.is_initialized():
            raise IC_CANError("Controller not initialized")

        try:
            self.controller.update_all_states()
            self._update_performance_stats()
        except Exception as e:
            logger.error(f"Failed to update states: {e}")
            raise IC_CANError(f"Failed to update states: {e}")

    def get_system_status(self) -> Dict[str, Any]:
        """
        Get comprehensive system status.

        Returns:
            Dictionary containing system status information
        """
        if not self.is_initialized():
            raise IC_CANError("Controller not initialized")

        try:
            status = self.controller.get_system_status()

            # Add Python-side status
            status.update({
                'python_initialized': self._initialized,
                'high_freq_control_running': self._high_freq_running,
                'performance_stats': self._performance_stats.copy(),
                'safety_callbacks_count': len(self._safety_callbacks)
            })

            return status
        except Exception as e:
            logger.error(f"Failed to get system status: {e}")
            raise IC_CANError(f"Failed to get system status: {e}")

    def print_system_info(self) -> None:
        """Print detailed system information."""
        if not self.is_initialized():
            raise IC_CANError("Controller not initialized")

        try:
            self.controller.print_system_info()

            # Add Python-side info
            print("\n=== Python Controller Status ===")
            print(f"Initialized: {self._initialized}")
            print(f"High-freq control: {self._high_freq_running}")
            print(f"Performance stats: {self._performance_stats}")
            print(f"Safety callbacks: {len(self._safety_callbacks)}")
            print("===============================\n")
        except Exception as e:
            logger.error(f"Failed to print system info: {e}")

    def add_safety_callback(self, callback: Callable[[str, str], None]) -> None:
        """
        Add safety violation callback.

        Args:
            callback: Function to call on safety violations (violation_type, message)
        """
        self._safety_callbacks.append(callback)
        logger.debug(f"Added safety callback: {callback.__name__}")

    def remove_safety_callback(self, callback: Callable[[str, str], None]) -> None:
        """
        Remove safety violation callback.

        Args:
            callback: Function to remove
        """
        if callback in self._safety_callbacks:
            self._safety_callbacks.remove(callback)
            logger.debug(f"Removed safety callback: {callback.__name__}")

    def get_performance_stats(self) -> Dict[str, Any]:
        """Get performance statistics."""
        return self._performance_stats.copy()

    def reset_performance_stats(self) -> None:
        """Reset performance statistics."""
        self._performance_stats = {
            'control_loop_count': 0,
            'last_update_time': 0,
            'average_frequency': 0.0,
            'min_frequency': float('inf'),
            'max_frequency': 0.0
        }
        logger.info("Performance statistics reset")

    @contextmanager
    def controlled_motion(self, emergency_stop_on_exit: bool = True):
        """
        Context manager for controlled motion operations.

        Args:
            emergency_stop_on_exit: Whether to trigger emergency stop on exception
        """
        try:
            yield self
        except Exception as e:
            logger.error(f"Controlled motion failed: {e}")
            if emergency_stop_on_exit:
                self.emergency_stop(f"Controlled motion exception: {e}")
            raise
        finally:
            # Ensure safe state
            if self._high_freq_running:
                self.stop_high_frequency_control()

    def _start_monitoring(self) -> None:
        """Start background monitoring thread."""
        def monitor():
            while self._initialized:
                try:
                    # Check for safety violations
                    safety = self.controller.get_safety()
                    if safety.has_active_violations():
                        violations = safety.get_active_violations()
                        for violation in violations:
                            self._notify_safety_violation(
                                SafetyViolationType.to_string(violation),
                                "Safety violation detected"
                            )

                    time.sleep(0.1)  # 10Hz monitoring
                except Exception as e:
                    logger.error(f"Monitoring error: {e}")
                    time.sleep(1.0)

        monitor_thread = threading.Thread(target=monitor, daemon=True)
        monitor_thread.start()
        logger.debug("Started safety monitoring thread")

    def _notify_safety_violation(self, violation_type: str, message: str) -> None:
        """Notify all safety callbacks."""
        for callback in self._safety_callbacks:
            try:
                callback(violation_type, message)
            except Exception as e:
                logger.error(f"Safety callback error: {e}")

    def _update_performance_stats(self) -> None:
        """Update performance statistics."""
        current_time = time.time()
        if self._performance_stats['last_update_time'] > 0:
            dt = current_time - self._performance_stats['last_update_time']
            frequency = 1.0 / dt

            self._performance_stats['control_loop_count'] += 1
            self._performance_stats['last_update_time'] = current_time

            # Update frequency stats
            if self._performance_stats['average_frequency'] == 0:
                self._performance_stats['average_frequency'] = frequency
            else:
                # Simple moving average
                alpha = 0.1
                self._performance_stats['average_frequency'] = (
                    alpha * frequency +
                    (1 - alpha) * self._performance_stats['average_frequency']
                )

            self._performance_stats['min_frequency'] = min(
                self._performance_stats['min_frequency'], frequency
            )
            self._performance_stats['max_frequency'] = max(
                self._performance_stats['max_frequency'], frequency
            )
        else:
            self._performance_stats['last_update_time'] = current_time

    def __del__(self):
        """Cleanup on deletion."""
        try:
            if hasattr(self, '_high_freq_running') and self._high_freq_running:
                self.stop_high_frequency_control()
            if hasattr(self, '_initialized') and self._initialized:
                self.disable_all()
        except Exception as e:
            logger.error(f"Cleanup error: {e}")