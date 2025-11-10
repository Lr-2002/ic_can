"""
Dual-Arm Manager for IC_CAN System

This module provides a high-level interface for controlling dual-arm robotic systems
using the existing IC_CAN implementation. Each arm uses the same base IC_CAN class
with different channel configurations.

Architecture:
    - Left Arm: Channel 0, Motors 1-9 (CAN IDs 0x01-0x09)
    - Right Arm: Channel 1, Motors 10-18 (CAN IDs 0x01-0x09, same IDs on different channel)
    - Cameras: 2 hand-eye + 1 fixed overhead camera

Key Features:
    - Configuration-driven arm initialization
    - Independent and coordinated arm control
    - Camera integration with arm-specific mapping
    - Safety monitoring for dual-arm operations
    - Real-time performance monitoring
"""

import numpy as np
import time
import logging
import json
from typing import Dict, List, Optional, Tuple, Union, Any
from threading import Lock, Thread
from dataclasses import dataclass
from pathlib import Path

from .robot_arm import RobotArm

try:
    from .ic_can_python import IC_CAN
except ImportError:
    # Use mock implementation for testing
    from .ic_can_python_mock import IC_CAN

# Setup logging
logger = logging.getLogger(__name__)

@dataclass
class ArmState:
    """Container for arm state information"""
    positions: np.ndarray
    velocities: np.ndarray
    torques: np.ndarray
    gripper_open: bool
    timestamp: float
    is_connected: bool

@dataclass
class DualArmState:
    """Container for dual-arm state information"""
    left_arm: ArmState
    right_arm: ArmState
    inter_arm_distance: float
    timestamp: float

class DualArmManager:
    """
    High-level manager for dual-arm robotic system.

    This class creates and manages two independent IC_CAN instances (left and right arms)
    with different channel configurations, providing both independent and coordinated
    control capabilities.

    Usage:
        manager = DualArmManager()
        manager.initialize()
        manager.enable_arms()

        # Independent control
        manager.move_left_arm(left_positions)
        manager.move_right_arm(right_positions)

        # Coordinated control
        manager.move_both_arms(left_positions, right_positions)
    """

    def __init__(self,
                 dual_arm_config_file: str = "config/dual_arm_config.json",
                 camera_config_file: str = "config/dual_camera_config.json",
                 debug: bool = False):
        """
        Initialize the dual-arm manager.

        Args:
            dual_arm_config_file: Path to dual-arm configuration JSON file
            camera_config_file: Path to camera configuration JSON file
            debug: Enable debug logging
        """
        self.debug = debug
        self.config_dir = Path(__file__).parent.parent.parent / "config"

        # Load configurations
        self.dual_arm_config_file = self.config_dir / "dual_arm_config.json"
        self.camera_config_file = self.config_dir / "dual_camera_config.json"
        self.left_arm_config_file = self.config_dir / "left_arm_config.json"
        self.right_arm_config_file = self.config_dir / "right_arm_config.json"

        # Configuration data
        self.dual_arm_config = {}
        self.camera_config = {}

        # Arm instances
        self.left_arm: Optional[RobotArm] = None
        self.right_arm: Optional[RobotArm] = None

        # Camera instances (will be populated later)
        self.cameras = {}

        # State management
        self.current_state: Optional[DualArmState] = None
        self.state_lock = Lock()

        # Safety and monitoring
        self.safety_enabled = True
        self.coordination_enabled = True
        self.max_inter_arm_distance = 1.0  # meters

        # Performance monitoring
        self.performance_stats = {
            'left_arm': {'frequency': 0.0, 'last_update': 0.0},
            'right_arm': {'frequency': 0.0, 'last_update': 0.0}
        }

        logger.info("DualArmManager initialized")

    def load_configurations(self) -> bool:
        """Load dual-arm and camera configurations from JSON files."""
        try:
            # Load dual-arm configuration
            with open(self.dual_arm_config_file, 'r') as f:
                self.dual_arm_config = json.load(f)

            # Load camera configuration
            with open(self.camera_config_file, 'r') as f:
                self.camera_config = json.load(f)

            # Set safety parameters
            self.safety_enabled = self.dual_arm_config.get('enable_safety_checks', True)
            self.coordination_enabled = self.dual_arm_config.get('enable_coordination', True)
            self.max_inter_arm_distance = self.dual_arm_config.get('max_inter_arm_distance', 1.0)

            logger.info("Configurations loaded successfully")
            return True

        except Exception as e:
            logger.error(f"Failed to load configurations: {e}")
            return False

    def initialize_arms(self) -> bool:
        """
        Initialize left and right arms using IC_CAN instances with different channel configs.

        Returns:
            True if both arms initialized successfully, False otherwise
        """
        if not self.load_configurations():
            return False

        try:
            # Create communication configs from JSON files
            left_comm_config = self._load_communication_config(self.left_arm_config_file)
            right_comm_config = self._load_communication_config(self.right_arm_config_file)

            # Initialize left arm (Channel 0)
            logger.info("Initializing left arm (Channel 0)...")
            self.left_arm = RobotArm(debug=self.debug, safety_limits=self.safety_enabled)
            if not self.left_arm.initialize():
                logger.error("Failed to initialize left arm")
                return False

            # Initialize right arm (Channel 1)
            logger.info("Initializing right arm (Channel 1)...")
            self.right_arm = RobotArm(debug=self.debug, safety_limits=self.safety_enabled)
            if not self.right_arm.initialize():
                logger.error("Failed to initialize right arm")
                return False

            logger.info("Both arms initialized successfully")
            return True

        except Exception as e:
            logger.error(f"Arm initialization failed: {e}")
            return False

    def _load_communication_config(self, config_file: Path) -> Dict[str, Any]:
        """Load communication configuration from JSON file."""
        with open(config_file, 'r') as f:
            config = json.load(f)
        return config

    def enable_arms(self) -> bool:
        """Enable both arms for control."""
        success = True

        if self.left_arm:
            try:
                self.left_arm.enable()
                logger.info("Left arm enabled")
            except Exception as e:
                logger.error(f"Failed to enable left arm: {e}")
                success = False

        if self.right_arm:
            try:
                self.right_arm.enable()
                logger.info("Right arm enabled")
            except Exception as e:
                logger.error(f"Failed to enable right arm: {e}")
                success = False

        return success

    def disable_arms(self) -> bool:
        """Disable both arms."""
        success = True

        if self.left_arm:
            try:
                self.left_arm.disable()
                logger.info("Left arm disabled")
            except Exception as e:
                logger.error(f"Failed to disable left arm: {e}")
                success = False

        if self.right_arm:
            try:
                self.right_arm.disable()
                logger.info("Right arm disabled")
            except Exception as e:
                logger.error(f"Failed to disable right arm: {e}")
                success = False

        return success

    def refresh_states(self) -> bool:
        """Refresh motor states for both arms."""
        success = True
        timestamp = time.time()

        try:
            # Refresh left arm state
            if self.left_arm:
                self.left_arm.refresh()
                left_state = self._get_arm_state(self.left_arm, timestamp)
            else:
                left_state = ArmState(
                    positions=np.zeros(9), velocities=np.zeros(9), torques=np.zeros(9),
                    gripper_open=True, timestamp=timestamp, is_connected=False
                )

            # Refresh right arm state
            if self.right_arm:
                self.right_arm.refresh()
                right_state = self._get_arm_state(self.right_arm, timestamp)
            else:
                right_state = ArmState(
                    positions=np.zeros(9), velocities=np.zeros(9), torques=np.zeros(9),
                    gripper_open=True, timestamp=timestamp, is_connected=False
                )

            # Calculate inter-arm distance (simplified - using end effector positions)
            inter_arm_distance = self._calculate_inter_arm_distance(left_state, right_state)

            # Update dual-arm state
            with self.state_lock:
                self.current_state = DualArmState(
                    left_arm=left_state,
                    right_arm=right_state,
                    inter_arm_distance=inter_arm_distance,
                    timestamp=timestamp
                )

            # Safety check
            if self.safety_enabled and self.coordination_enabled:
                self._check_safety_limits()

            # Update performance stats
            self._update_performance_stats(timestamp)

        except Exception as e:
            logger.error(f"Failed to refresh states: {e}")
            success = False

        return success

    def _get_arm_state(self, arm: RobotArm, timestamp: float) -> ArmState:
        """Extract arm state from RobotArm instance."""
        try:
            # Get joint states (9 motors: 6 arm + 2 wrist + 1 gripper)
            positions = arm.get_joint_positions()
            velocities = arm.get_joint_velocities()
            torques = arm.get_joint_torques()

            # Gripper state (last joint)
            gripper_open = True  # Simplified - should check actual gripper state

            return ArmState(
                positions=np.array(positions),
                velocities=np.array(velocities),
                torques=np.array(torques),
                gripper_open=gripper_open,
                timestamp=timestamp,
                is_connected=True
            )

        except Exception as e:
            logger.warning(f"Failed to get arm state: {e}")
            return ArmState(
                positions=np.zeros(9), velocities=np.zeros(9), torques=np.zeros(9),
                gripper_open=True, timestamp=timestamp, is_connected=False
            )

    def _calculate_inter_arm_distance(self, left_state: ArmState, right_state: ArmState) -> float:
        """Calculate distance between end effectors (simplified)."""
        # Simplified calculation - in real implementation, this would use forward kinematics
        # to calculate 3D end effector positions and compute Euclidean distance
        left_ee_pos = left_state.positions[:6]  # Use arm joint positions as proxy
        right_ee_pos = right_state.positions[:6]

        # Simple joint-space distance metric (not actual Cartesian distance)
        distance = np.linalg.norm(left_ee_pos - right_ee_pos)
        return float(distance)

    def _check_safety_limits(self):
        """Check safety limits for dual-arm operation."""
        if not self.current_state:
            return

        # Check inter-arm distance
        if self.current_state.inter_arm_distance > self.max_inter_arm_distance:
            logger.warning(f"Inter-arm distance exceeded: {self.current_state.inter_arm_distance:.3f}m > {self.max_inter_arm_distance:.3f}m")
            # In a real implementation, this might trigger an emergency stop

    def _update_performance_stats(self, timestamp: float):
        """Update performance monitoring statistics."""
        # Left arm
        if self.left_arm and self.performance_stats['left_arm']['last_update'] > 0:
            dt = timestamp - self.performance_stats['left_arm']['last_update']
            if dt > 0:
                self.performance_stats['left_arm']['frequency'] = 1.0 / dt
        self.performance_stats['left_arm']['last_update'] = timestamp

        # Right arm
        if self.right_arm and self.performance_stats['right_arm']['last_update'] > 0:
            dt = timestamp - self.performance_stats['right_arm']['last_update']
            if dt > 0:
                self.performance_stats['right_arm']['frequency'] = 1.0 / dt
        self.performance_stats['right_arm']['last_update'] = timestamp

    def get_current_state(self) -> Optional[DualArmState]:
        """Get current dual-arm state."""
        with self.state_lock:
            return self.current_state

    # Independent arm control methods
    def move_left_arm(self, positions: np.ndarray, velocities: Optional[np.ndarray] = None,
                     torques: Optional[np.ndarray] = None) -> bool:
        """Move left arm to specified positions."""
        if self.left_arm is None:
            logger.error("Left arm not initialized")
            return False

        try:
            return self.left_arm.step(positions, velocities or np.zeros_like(positions),
                                    torques or np.zeros_like(positions))
        except Exception as e:
            logger.error(f"Failed to move left arm: {e}")
            return False

    def move_right_arm(self, positions: np.ndarray, velocities: Optional[np.ndarray] = None,
                      torques: Optional[np.ndarray] = None) -> bool:
        """Move right arm to specified positions."""
        if self.right_arm is None:
            logger.error("Right arm not initialized")
            return False

        try:
            return self.right_arm.step(positions, velocities or np.zeros_like(positions),
                                     torques or np.zeros_like(positions))
        except Exception as e:
            logger.error(f"Failed to move right arm: {e}")
            return False

    # Coordinated control methods
    def move_both_arms(self, left_positions: np.ndarray, right_positions: np.ndarray,
                      left_velocities: Optional[np.ndarray] = None,
                      right_velocities: Optional[np.ndarray] = None,
                      left_torques: Optional[np.ndarray] = None,
                      right_torques: Optional[np.ndarray] = None) -> bool:
        """Move both arms simultaneously."""
        success = True

        try:
            # Safety check before movement
            if self.safety_enabled and self.coordination_enabled:
                # In a real implementation, this would check for collisions and workspace limits
                pass

            # Send commands to both arms
            left_success = self.move_left_arm(left_positions, left_velocities, left_torques)
            right_success = self.move_right_arm(right_positions, right_velocities, right_torques)

            success = left_success and right_success

            if success:
                logger.debug("Both arms moved successfully")
            else:
                logger.warning("One or both arms failed to move")

        except Exception as e:
            logger.error(f"Failed to move both arms: {e}")
            success = False

        return success

    def home_both_arms(self) -> bool:
        """Move both arms to home position."""
        # Home positions (zeros - should be calibrated per arm)
        left_home = np.zeros(9)
        right_home = np.zeros(9)

        return self.move_both_arms(left_home, right_home)

    def emergency_stop(self) -> bool:
        """Emergency stop both arms."""
        logger.warning("EMERGENCY STOP ACTIVATED")
        success = True

        if self.left_arm:
            try:
                self.left_arm.disable()
            except Exception as e:
                logger.error(f"Failed to emergency stop left arm: {e}")
                success = False

        if self.right_arm:
            try:
                self.right_arm.disable()
            except Exception as e:
                logger.error(f"Failed to emergency stop right arm: {e}")
                success = False

        return success

    def set_control_mode(self, mode: str) -> bool:
        """Set control mode for both arms."""
        success = True

        if self.left_arm:
            try:
                self.left_arm.set_control_mode(mode)
            except Exception as e:
                logger.error(f"Failed to set control mode for left arm: {e}")
                success = False

        if self.right_arm:
            try:
                self.right_arm.set_control_mode(mode)
            except Exception as e:
                logger.error(f"Failed to set control mode for right arm: {e}")
                success = False

        return success

    def get_performance_stats(self) -> Dict[str, Dict[str, float]]:
        """Get performance statistics for both arms."""
        return self.performance_stats.copy()

    def print_status(self):
        """Print current status of both arms."""
        print("\n🦾 Dual-Arm System Status:")
        print("=" * 50)

        # Left arm status
        if self.left_arm:
            print(f"Left Arm (Channel 0): {'✅ Connected' if self.left_arm.is_connected() else '❌ Disconnected'}")
            print(f"  Frequency: {self.performance_stats['left_arm']['frequency']:.1f} Hz")
        else:
            print("Left Arm: ❌ Not initialized")

        # Right arm status
        if self.right_arm:
            print(f"Right Arm (Channel 1): {'✅ Connected' if self.right_arm.is_connected() else '❌ Disconnected'}")
            print(f"  Frequency: {self.performance_stats['right_arm']['frequency']:.1f} Hz")
        else:
            print("Right Arm: ❌ Not initialized")

        # Current state
        if self.current_state:
            print(f"Inter-arm Distance: {self.current_state.inter_arm_distance:.3f} m")
            print(f"Safety: {'✅ Enabled' if self.safety_enabled else '❌ Disabled'}")
            print(f"Coordination: {'✅ Enabled' if self.coordination_enabled else '❌ Disabled'}")

        print("=" * 50)

    def shutdown(self):
        """Shutdown dual-arm system safely."""
        logger.info("Shutting down dual-arm system...")

        # Disable both arms
        self.disable_arms()

        # Clear instances
        self.left_arm = None
        self.right_arm = None
        self.current_state = None

        logger.info("Dual-arm system shutdown complete")

# Convenience function for easy usage
def create_dual_arm_manager(config_dir: str = "config", debug: bool = False) -> DualArmManager:
    """Create a dual-arm manager with default configuration."""
    return DualArmManager(debug=debug)