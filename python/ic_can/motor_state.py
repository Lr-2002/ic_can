"""
Motor State Representation for IC_CAN High-Level Python API

This module provides clean data structures for representing motor state and commands
in a way that is optimized for RL/DNN applications.
"""

from dataclasses import dataclass, field
from typing import List, Optional, Union
import numpy as np
import time
from enum import Enum


class ControlMode(Enum):
    """Control mode enumeration for the robotic arm."""
    TEACH = "TEACH"
    EXECUTION = "EXECUTION"


@dataclass
class MotorState:
    """
    Clean state representation for all 9 motors in the IC_ARM system.

    This class provides a unified interface for accessing motor state information
    in a format that is optimized for reinforcement learning and deep neural network
    applications.

    Attributes:
        positions (np.ndarray): Joint angles in radians for all 9 motors
        velocities (np.ndarray): Joint velocities in rad/s for all 9 motors
        torques (np.ndarray): Joint torques in Nm for all 9 motors
        temperatures (np.ndarray): Motor temperatures in Celsius
        enabled (np.ndarray): Boolean array indicating which motors are enabled
        errors (np.ndarray): Boolean array indicating motor error status
        timestamp (float): Unix timestamp when state was captured
        control_mode (ControlMode): Current control mode of the system
        motor_count (int): Number of motors (always 9)
    """
    positions: np.ndarray = field(default_factory=lambda: np.zeros(9))
    velocities: np.ndarray = field(default_factory=lambda: np.zeros(9))
    torques: np.ndarray = field(default_factory=lambda: np.zeros(9))
    temperatures: np.ndarray = field(default_factory=lambda: np.full(9, 25.0))
    enabled: np.ndarray = field(default_factory=lambda: np.zeros(9, dtype=bool))
    errors: np.ndarray = field(default_factory=lambda: np.zeros(9, dtype=bool))
    timestamp: float = field(default_factory=time.time)
    control_mode: ControlMode = ControlMode.TEACH
    motor_count: int = 9

    def __post_init__(self):
        """Ensure all arrays are numpy arrays with correct shape."""
        self.positions = np.asarray(self.positions, dtype=np.float64)
        self.velocities = np.asarray(self.velocities, dtype=np.float64)
        self.torques = np.asarray(self.torques, dtype=np.float64)
        self.temperatures = np.asarray(self.temperatures, dtype=np.float64)
        self.enabled = np.asarray(self.enabled, dtype=bool)
        self.errors = np.asarray(self.errors, dtype=bool)

        # Ensure correct shapes
        if self.positions.shape != (9,):
            self.positions = np.zeros(9)
        if self.velocities.shape != (9,):
            self.velocities = np.zeros(9)
        if self.torques.shape != (9,):
            self.torques = np.zeros(9)
        if self.temperatures.shape != (9,):
            self.temperatures = np.full(9, 25.0)
        if self.enabled.shape != (9,):
            self.enabled = np.zeros(9, dtype=bool)
        if self.errors.shape != (9,):
            self.errors = np.zeros(9, dtype=bool)

    @classmethod
    def from_cpp_state(cls, cpp_state_dict: dict) -> 'MotorState':
        """
        Create MotorState from C++ state dictionary.

        Args:
            cpp_state_dict: Dictionary returned by C++ refresh_all_state method

        Returns:
            MotorState instance
        """
        positions = np.array(cpp_state_dict.get("positions", [0.0] * 9))
        velocities = np.array(cpp_state_dict.get("velocities", [0.0] * 9))
        torques = np.array(cpp_state_dict.get("torques", [0.0] * 9))
        timestamp = cpp_state_dict.get("timestamp", time.time())
        motor_count = cpp_state_dict.get("motor_count", 9)

        return cls(
            positions=positions,
            velocities=velocities,
            torques=torques,
            timestamp=timestamp,
            motor_count=motor_count
        )

    def to_vector(self) -> np.ndarray:
        """
        Convert state to a single flat vector for RL applications.

        Returns:
            Combined state vector: [positions(9), velocities(9), torques(9)]
        """
        return np.concatenate([self.positions, self.velocities, self.torques])

    def is_valid(self) -> bool:
        """Check if state data is consistent and valid."""
        return (len(self.positions) == 9 and
                len(self.velocities) == 9 and
                len(self.torques) == 9 and
                not np.any(np.isnan(self.positions)) and
                not np.any(np.isnan(self.velocities)) and
                not np.any(np.isnan(self.torques)))

    def has_errors(self) -> bool:
        """Check if any motor has errors."""
        return np.any(self.errors)

    def is_safe(self) -> bool:
        """Check if the system is in a safe state."""
        return (self.is_valid() and
                not self.has_errors() and
                self.is_within_limits())

    def is_within_limits(self) -> bool:
        """Check if all joint positions are within reasonable limits."""
        # Basic joint limits: +/- π radians
        return np.all(np.abs(self.positions) <= np.pi)

    # Motor group access methods
    def get_arm_positions(self) -> np.ndarray:
        """Get positions for arm motors (1-6)."""
        return self.positions[:6]

    def get_wrist_positions(self) -> np.ndarray:
        """Get positions for wrist motors (7-8)."""
        return self.positions[6:8]

    def get_gripper_position(self) -> float:
        """Get position for gripper motor (9)."""
        return self.positions[8]

    def get_arm_state(self) -> 'MotorState':
        """Get state for arm motors only."""
        return MotorState(
            positions=self.get_arm_positions(),
            velocities=self.velocities[:6],
            torques=self.torques[:6],
            temperatures=self.temperatures[:6],
            enabled=self.enabled[:6],
            errors=self.errors[:6],
            timestamp=self.timestamp,
            control_mode=self.control_mode,
            motor_count=6
        )

    def copy(self) -> 'MotorState':
        """Create a deep copy of the motor state."""
        return MotorState(
            positions=self.positions.copy(),
            velocities=self.velocities.copy(),
            torques=self.torques.copy(),
            temperatures=self.temperatures.copy(),
            enabled=self.enabled.copy(),
            errors=self.errors.copy(),
            timestamp=self.timestamp,
            control_mode=self.control_mode,
            motor_count=self.motor_count
        )

    def __str__(self) -> str:
        """String representation of motor state."""
        return (f"MotorState(timestamp={self.timestamp:.3f}, "
                f"mode={self.control_mode.value}, "
                f"pos_range=[{np.min(self.positions):.3f}, {np.max(self.positions):.3f}], "
                f"errors={np.sum(self.errors)})")

    def __repr__(self) -> str:
        return self.__str__()


@dataclass
class MotorCommand:
    """
    Command structure for controlling all 9 motors.

    Attributes:
        positions (np.ndarray): Target positions for all motors (radians)
        velocities (Optional[np.ndarray]): Target velocity limits (rad/s)
        torques (Optional[np.ndarray]): Target torque limits (Nm)
    """
    positions: np.ndarray
    velocities: Optional[np.ndarray] = None
    torques: Optional[np.ndarray] = None

    def __post_init__(self):
        """Ensure positions is a numpy array with correct shape."""
        self.positions = np.asarray(self.positions, dtype=np.float64)
        if self.positions.shape != (9,):
            raise ValueError(f"Positions must have shape (9,), got {self.positions.shape}")

        if self.velocities is not None:
            self.velocities = np.asarray(self.velocities, dtype=np.float64)
            if self.velocities.shape != (9,):
                raise ValueError(f"Velocities must have shape (9,), got {self.velocities.shape}")

        if self.torques is not None:
            self.torques = np.asarray(self.torques, dtype=np.float64)
            if self.torques.shape != (9,):
                raise ValueError(f"Torques must have shape (9,), got {self.torques.shape}")

    def is_valid(self) -> bool:
        """Check if command is valid."""
        return (len(self.positions) == 9 and
                not np.any(np.isnan(self.positions)) and
                (self.velocities is None or not np.any(np.isnan(self.velocities))) and
                (self.torques is None or not np.any(np.isnan(self.torques))))

    def clamp_positions(self, limits: tuple = (-np.pi, np.pi)) -> 'MotorCommand':
        """Clamp positions to specified limits."""
        clamped_positions = np.clip(self.positions, limits[0], limits[1])
        return MotorCommand(
            positions=clamped_positions,
            velocities=self.velocities,
            torques=self.torques
        )

    @classmethod
    def zero_position(cls) -> 'MotorCommand':
        """Create a zero position command."""
        return cls(positions=np.zeros(9))

    @classmethod
    def from_arm_wrist_gripper(cls, arm_pos: List[float],
                              wrist_pos: List[float],
                              gripper_pos: float) -> 'MotorCommand':
        """Create command from separate arm, wrist, and gripper positions."""
        if len(arm_pos) != 6:
            raise ValueError("Arm positions must have 6 elements")
        if len(wrist_pos) != 2:
            raise ValueError("Wrist positions must have 2 elements")

        positions = np.zeros(9)
        positions[:6] = arm_pos
        positions[6:8] = wrist_pos
        positions[8] = gripper_pos

        return cls(positions=positions)