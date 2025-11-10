"""
High-Level RobotArm Interface for IC_CAN

This module provides a clean, production-ready Python interface for controlling
the IC_ARM robotic system. Designed specifically for RL and DNN applications
with a simple 4-method API: enable, refresh, step, set_control_mode.

The interface abstracts away low-level motor details while maintaining
real-time performance (500Hz control loop) and comprehensive safety features.
"""

import numpy as np
import time
import logging
from typing import List, Optional, Union
from threading import Lock

from .motor_state import MotorState, MotorCommand, ControlMode
from .ic_can_python import IC_CAN

# Setup logging
logger = logging.getLogger(__name__)


class RobotArm:
    """
    High-level interface for controlling the IC_ARM robotic system.

    This class provides a clean, unified interface for all 9 motors (6 arm + 2 wrist + 1 gripper)
    while abstracting away the complexity of the underlying C++ implementation.

    Core API Methods:
        - enable(): Enable motors for control
        - refresh(): Read current motor state
        - step(): Send position/velocity/torque commands
        - set_control_mode(): Switch between TEACH and EXECUTION modes

    Features:
        - Real-time 500Hz control performance
        - Comprehensive safety monitoring
        - NumPy-compatible state representation
        - Production-ready error handling
        - RL/DNN-optimized interface
    """

    def __init__(self, device_sn: str = "F561E08C892274DB09496BCC1102DBC5",
                 debug: bool = False,
                 safety_limits: bool = True):
        """
        Initialize the RobotArm controller.

        Args:
            device_sn: Device serial number for CAN communication
            debug: Enable debug logging
            safety_limits: Enable automatic position/velocity/torque limiting
        """
        self.device_sn = device_sn
        self.debug = debug
        self.safety_limits = safety_limits
        self._lock = Lock()
        self._initialized = False
        self._motors_enabled = False

        # Safety limits
        self._position_limits = (-np.pi, np.pi)  # +/- π radians
        self._velocity_limits = 2.0  # rad/s
        self._torque_limits = 10.0   # Nm

        # Initialize low-level controller
        try:
            self._controller = IC_CAN(device_sn, debug)
            self._initialized = True
            logger.info(f"RobotArm initialized with device: {device_sn}")
        except Exception as e:
            logger.error(f"Failed to initialize RobotArm: {e}")
            raise

        # State tracking
        self._last_state = None
        self._control_mode = ControlMode.TEACH
        self._performance_stats = {
            'last_refresh_time': 0.0,
            'last_step_time': 0.0,
            'refresh_count': 0,
            'step_count': 0
        }

    def initialize(self) -> bool:
        """
        Initialize the hardware system and establish communication.

        Returns:
            True if initialization successful, False otherwise
        """
        if not self._initialized:
            logger.error("RobotArm not properly initialized")
            return False

        try:
            with self._lock:
                success = self._controller.initialize()
                if success:
                    logger.info("RobotArm hardware initialized successfully")
                    # Load default gains
                    self._controller.load_default_motor_gains()
                else:
                    logger.error("Failed to initialize RobotArm hardware")
                return success
        except Exception as e:
            logger.error(f"Initialization error: {e}")
            return False

    def shutdown(self) -> bool:
        """
        Safely shutdown the robot system.

        Returns:
            True if shutdown successful
        """
        try:
            with self._lock:
                # Stop any active control
                if self._controller.is_hf_control_running():
                    self._controller.stop_high_frequency_control()

                # Disable motors
                self._controller.disable_all()
                self._motors_enabled = False

                # Shutdown system
                success = self._controller.shutdown()
                logger.info("RobotArm shutdown complete")
                return success
        except Exception as e:
            logger.error(f"Shutdown error: {e}")
            return False

    def enable(self, motor_ids: Optional[List[int]] = None) -> bool:
        """
        Enable motors for control.

        Args:
            motor_ids: Optional list of motor IDs (1-9) to enable.
                      If None, enables all motors.

        Returns:
            True if motors enabled successfully
        """
        if not self._initialized:
            logger.error("RobotArm not initialized")
            return False

        try:
            with self._lock:
                if motor_ids is None:
                    success = self._controller.enable_all()
                    self._motors_enabled = success
                    logger.info("All motors enabled")
                else:
                    # Use the new pybind method
                    success = self._controller.enable_motors(motor_ids)
                    self._motors_enabled = success
                    logger.info(f"Motors {motor_ids} enabled")

                return success
        except Exception as e:
            logger.error(f"Enable error: {e}")
            return False

    def disable(self) -> bool:
        """
        Disable all motors.

        Returns:
            True if motors disabled successfully
        """
        try:
            with self._lock:
                success = self._controller.disable_all()
                self._motors_enabled = False
                logger.info("All motors disabled")
                return success
        except Exception as e:
            logger.error(f"Disable error: {e}")
            return False

    def refresh(self) -> MotorState:
        """
        Refresh and return current motor state.

        Returns:
            MotorState object containing current state of all 9 motors
        """
        if not self._initialized:
            raise RuntimeError("RobotArm not initialized")

        start_time = time.time()

        try:
            with self._lock:
                # Use the new unified state refresh method
                cpp_state = self._controller.refresh_all_state()

                # Create MotorState from C++ data
                state = MotorState.from_cpp_state(cpp_state)

                # Add control mode information
                mode_str = self._controller.get_control_mode_python()
                state.control_mode = ControlMode(mode_str)

                # Update performance stats
                self._last_state = state
                self._performance_stats['last_refresh_time'] = time.time() - start_time
                self._performance_stats['refresh_count'] += 1

                if self.debug:
                    logger.debug(f"State refreshed in {self._performance_stats['last_refresh_time']*1000:.2f}ms")

                return state

        except Exception as e:
            logger.error(f"Refresh error: {e}")
            raise

    def step(self, positions: Union[np.ndarray, List[float]],
             velocities: Optional[Union[np.ndarray, List[float]]] = None,
             torques: Optional[Union[np.ndarray, List[float]]] = None) -> bool:
        """
        Execute a single control step with position/velocity/torque commands.

        This is the core control method for the robot. It sends commands to all 9 motors
        simultaneously and provides optional velocity and torque limiting.

        Args:
            positions: Target positions for all 9 motors (radians)
            velocities: Optional velocity limits (rad/s)
            torques: Optional torque limits (Nm)

        Returns:
            True if command sent successfully
        """
        if not self._initialized:
            logger.error("RobotArm not initialized")
            return False

        if not self._motors_enabled:
            logger.warning("Motors not enabled - step command ignored")
            return False

        start_time = time.time()

        try:
            # Convert inputs to numpy arrays
            positions = np.asarray(positions, dtype=np.float64)
            if positions.shape != (9,):
                raise ValueError(f"Positions must have shape (9,), got {positions.shape}")

            # Apply safety limits if enabled
            if self.safety_limits:
                positions = np.clip(positions, self._position_limits[0], self._position_limits[1])

            # Create command
            command = MotorCommand(positions=positions)

            # Handle optional parameters
            py_velocities = None
            py_torques = None

            if velocities is not None:
                velocities = np.asarray(velocities, dtype=np.float64)
                if velocities.shape != (9,):
                    raise ValueError(f"Velocities must have shape (9,), got {velocities.shape}")
                if self.safety_limits:
                    velocities = np.clip(velocities, -self._velocity_limits, self._velocity_limits)
                py_velocities = velocities.tolist()

            if torques is not None:
                torques = np.asarray(torques, dtype=np.float64)
                if torques.shape != (9,):
                    raise ValueError(f"Torques must have shape (9,), got {torques.shape}")
                if self.safety_limits:
                    torques = np.clip(torques, -self._torque_limits, self._torque_limits)
                py_torques = torques.tolist()

            # Send command using new pybind method
            with self._lock:
                success = self._controller.step_control(positions.tolist(),
                                                       py_velocities,
                                                       py_torques)

                # Update performance stats
                self._performance_stats['last_step_time'] = time.time() - start_time
                self._performance_stats['step_count'] += 1

                if self.debug:
                    logger.debug(f"Step command executed in {self._performance_stats['last_step_time']*1000:.2f}ms")

                return success

        except Exception as e:
            logger.error(f"Step error: {e}")
            return False

    def set_control_mode(self, mode: Union[str, ControlMode]) -> bool:
        """
        Switch between TEACH and EXECUTION control modes.

        Args:
            mode: Control mode - "TEACH" or "EXECUTION" (or ControlMode enum)

        Returns:
            True if mode switched successfully
        """
        if not self._initialized:
            logger.error("RobotArm not initialized")
            return False

        try:
            # Convert to string
            if isinstance(mode, ControlMode):
                mode_str = mode.value
            else:
                mode_str = str(mode).upper()

            with self._lock:
                success = self._controller.set_control_mode_python(mode_str)
                if success:
                    self._control_mode = ControlMode(mode_str)
                    logger.info(f"Control mode changed to {mode_str}")
                else:
                    logger.error(f"Failed to change control mode to {mode_str}")

                return success

        except Exception as e:
            logger.error(f"Control mode change error: {e}")
            return False

    def get_control_mode(self) -> ControlMode:
        """Get current control mode."""
        return self._control_mode

    def emergency_stop(self) -> bool:
        """
        Immediately stop all robot motion.

        Returns:
            True if emergency stop executed successfully
        """
        logger.warning("EMERGENCY STOP ACTIVATED")
        try:
            # Stop control loop if running
            if self._controller.is_hf_control_running():
                self._controller.stop_high_frequency_control()

            # Disable all motors
            success = self._controller.disable_all()
            self._motors_enabled = False

            logger.info("Emergency stop completed")
            return success
        except Exception as e:
            logger.error(f"Emergency stop error: {e}")
            return False

    def is_safe(self) -> bool:
        """Check if robot is in a safe state."""
        if not self._initialized or not self._motors_enabled:
            return False

        try:
            state = self.refresh()
            return state.is_safe()
        except:
            return False

    def get_performance_stats(self) -> dict:
        """Get performance statistics."""
        return self._performance_stats.copy()

    def __enter__(self):
        """Context manager entry."""
        if self.initialize():
            return self
        else:
            raise RuntimeError("Failed to initialize RobotArm")

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit."""
        self.shutdown()

    def __repr__(self) -> str:
        """String representation."""
        return (f"RobotArm(device={self.device_sn}, "
                f"initialized={self._initialized}, "
                f"enabled={self._motors_enabled}, "
                f"mode={self._control_mode.value})")


# Convenience function for quick initialization
def create_robot_arm(device_sn: str = "F561E08C892274DB09496BCC1102DBC5",
                     debug: bool = False) -> RobotArm:
    """
    Create and initialize a RobotArm instance.

    Args:
        device_sn: Device serial number
        debug: Enable debug logging

    Returns:
        Initialized RobotArm instance
    """
    robot = RobotArm(device_sn=device_sn, debug=debug)
    if not robot.initialize():
        raise RuntimeError("Failed to initialize RobotArm")
    return robot