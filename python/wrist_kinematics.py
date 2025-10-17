#!/usr/bin/env python3
"""
Wrist Kinematics Module for Cross-Wrist Mechanism

This module provides the kinematics calculations for the cross-wrist with 3-umberal gear group.
The relationship between motor angles and output angles is:
- α = (θ₁ + θ₂) / 2  (pitch motion)
- β = (θ₁ - θ₂) / 2  (roll motion)

And the inverse kinematics:
- θ₁ = α + β
- θ₂ = α - β

Author: Claude Code Assistant
"""

import numpy as np
from typing import Tuple, List, Union
import math


class WristKinematics:
    """
    Wrist kinematics calculator for cross-wrist mechanism.

    This class handles the forward and inverse kinematics for the differential
    wrist mechanism with two input motors and two output degrees of freedom.
    """

    def __init__(self):
        """Initialize wrist kinematics calculator."""
        pass

    @staticmethod
    def forward_kinematics(theta1: float, theta2: float) -> Tuple[float, float]:
        """
        Convert motor angles to output angles.

        Args:
            theta1: Motor 1 angle (θ₁) in radians
            theta2: Motor 2 angle (θ₂) in radians

        Returns:
            Tuple of (alpha, beta) output angles in radians
            - alpha: Pitch angle (α) = (θ₁ + θ₂) / 2
            - beta: Roll angle (β) = (θ₁ - θ₂) / 2
        """
        alpha = (theta1 + theta2) / 2.0  # α = (θ₁ + θ₂) / 2
        beta = (theta1 - theta2) / 2.0   # β = (θ₁ - θ₂) / 2

        return alpha, beta

    @staticmethod
    def inverse_kinematics(alpha: float, beta: float) -> Tuple[float, float]:
        """
        Convert desired output angles to motor angles.

        Args:
            alpha: Desired pitch angle (α) in radians
            beta: Desired roll angle (β) in radians

        Returns:
            Tuple of (theta1, theta2) motor angles in radians
            - theta1: Motor 1 angle (θ₁) = α + β
            - theta2: Motor 2 angle (θ₂) = α - β
        """
        theta1 = alpha + beta  # θ₁ = α + β
        theta2 = alpha - beta  # θ₂ = α - β

        return theta1, theta2

    @staticmethod
    def forward_kinematics_vector(motor_angles: Union[List[float], np.ndarray]) -> np.ndarray:
        """
        Convert motor angles to output angles for vector inputs.

        Args:
            motor_angles: Array-like of motor angles [θ₁, θ₂] in radians

        Returns:
            numpy array of output angles [α, β] in radians
        """
        motor_angles = np.array(motor_angles)
        if motor_angles.size != 2:
            raise ValueError("Motor angles array must have exactly 2 elements [θ₁, θ₂]")

        alpha, beta = WristKinematics.forward_kinematics(motor_angles[0], motor_angles[1])
        return np.array([alpha, beta])

    @staticmethod
    def inverse_kinematics_vector(output_angles: Union[List[float], np.ndarray]) -> np.ndarray:
        """
        Convert desired output angles to motor angles for vector inputs.

        Args:
            output_angles: Array-like of desired output angles [α, β] in radians

        Returns:
            numpy array of motor angles [θ₁, θ₂] in radians
        """
        output_angles = np.array(output_angles)
        if output_angles.size != 2:
            raise ValueError("Output angles array must have exactly 2 elements [α, β]")

        theta1, theta2 = WristKinematics.inverse_kinematics(output_angles[0], output_angles[1])
        return np.array([theta1, theta2])

    @staticmethod
    def deg_to_rad(degrees: float) -> float:
        """Convert degrees to radians."""
        return degrees * np.pi / 180.0

    @staticmethod
    def rad_to_deg(radians: float) -> float:
        """Convert radians to degrees."""
        return radians * 180.0 / np.pi

    @staticmethod
    def normalize_angle(angle: float) -> float:
        """
        Normalize angle to [-π, π].

        Args:
            angle: Input angle in radians

        Returns:
            Normalized angle in radians
        """
        while angle > np.pi:
            angle -= 2 * np.pi
        while angle < -np.pi:
            angle += 2 * np.pi
        return angle

    @staticmethod
    def validate_motor_angles(theta1: float, theta2: float,
                            min_angle: float = -np.pi,
                            max_angle: float = np.pi) -> bool:
        """
        Validate motor angles are within limits.

        Args:
            theta1: Motor 1 angle (θ₁) in radians
            theta2: Motor 2 angle (θ₂) in radians
            min_angle: Minimum allowed angle in radians
            max_angle: Maximum allowed angle in radians

        Returns:
            True if angles are within limits
        """
        return (min_angle <= theta1 <= max_angle and
                min_angle <= theta2 <= max_angle)

    @staticmethod
    def validate_output_angles(alpha: float, beta: float,
                             min_alpha: float = -np.pi/2,
                             max_alpha: float = np.pi/2,
                             min_beta: float = -np.pi/2,
                             max_beta: float = np.pi/2) -> bool:
        """
        Validate output angles are within limits.

        Args:
            alpha: Pitch angle (α) in radians
            beta: Roll angle (β) in radians
            min_alpha: Minimum allowed pitch angle in radians
            max_alpha: Maximum allowed pitch angle in radians
            min_beta: Minimum allowed roll angle in radians
            max_beta: Maximum allowed roll angle in radians

        Returns:
            True if angles are within limits
        """
        return (min_alpha <= alpha <= max_alpha and
                min_beta <= beta <= max_beta)

    @staticmethod
    def print_wrist_state(theta1: float, theta2: float) -> None:
        """
        Print current wrist state in a readable format.

        Args:
            theta1: Motor 1 angle (θ₁) in radians
            theta2: Motor 2 angle (θ₂) in radians
        """
        alpha, beta = WristKinematics.forward_kinematics(theta1, theta2)

        print("Wrist State:")
        print(f"  Motor Angles: θ₁ = {WristKinematics.rad_to_deg(theta1):.2f}°, θ₂ = {WristKinematics.rad_to_deg(theta2):.2f}°")
        print(f"  Output Angles: α = {WristKinematics.rad_to_deg(alpha):.2f}°, β = {WristKinematics.rad_to_deg(beta):.2f}°")
        print(f"  α (pitch): {alpha:.4f} rad ({WristKinematics.rad_to_deg(alpha):.2f}°)")
        print(f"  β (roll): {beta:.4f} rad ({WristKinematics.rad_to_deg(beta):.2f}°)")


class WristController:
    """
    High-level controller for the wrist mechanism.

    This class provides a convenient interface for controlling the wrist
    using desired output angles (alpha, beta) and handles the conversion
    to motor commands.
    """

    def __init__(self):
        """Initialize wrist controller."""
        self.kinematics = WristKinematics()
        self.current_theta1 = 0.0
        self.current_theta2 = 0.0

    def set_motor_angles(self, theta1: float, theta2: float) -> None:
        """
        Set current motor angles.

        Args:
            theta1: Motor 1 angle (θ₁) in radians
            theta2: Motor 2 angle (θ₂) in radians
        """
        self.current_theta1 = theta1
        self.current_theta2 = theta2

    def get_motor_angles(self) -> Tuple[float, float]:
        """
        Get current motor angles.

        Returns:
            Tuple of (theta1, theta2) motor angles in radians
        """
        return self.current_theta1, self.current_theta2

    def get_output_angles(self) -> Tuple[float, float]:
        """
        Get current output angles.

        Returns:
            Tuple of (alpha, beta) output angles in radians
        """
        return self.kinematics.forward_kinematics(self.current_theta1, self.current_theta2)

    def command_output_angles(self, alpha: float, beta: float) -> Tuple[float, float]:
        """
        Command desired output angles and return required motor angles.

        Args:
            alpha: Desired pitch angle (α) in radians
            beta: Desired roll angle (β) in radians

        Returns:
            Tuple of (theta1, theta2) motor angles in radians
        """
        theta1, theta2 = self.kinematics.inverse_kinematics(alpha, beta)

        # Validate motor angles
        if not self.kinematics.validate_motor_angles(theta1, theta2):
            print(f"Warning: Motor angles out of bounds: θ₁={self.kinematics.rad_to_deg(theta1):.2f}°, θ₂={self.kinematics.rad_to_deg(theta2):.2f}°")

        # Validate output angles
        if not self.kinematics.validate_output_angles(alpha, beta):
            print(f"Warning: Output angles out of bounds: α={self.kinematics.rad_to_deg(alpha):.2f}°, β={self.kinematics.rad_to_deg(beta):.2f}°")

        self.current_theta1 = theta1
        self.current_theta2 = theta2

        return theta1, theta2

    def print_state(self) -> None:
        """Print current wrist state."""
        self.kinematics.print_wrist_state(self.current_theta1, self.current_theta2)


def run_tests():
    """Run comprehensive tests for wrist kinematics."""
    print("=" * 60)
    print("WRIST KINEMATICS TESTS")
    print("=" * 60)

    # Test forward kinematics
    print("\n1. Testing Forward Kinematics")
    print("-" * 40)

    test_cases_forward = [
        (0.0, 0.0),
        (np.pi/2, np.pi/2),
        (np.pi/2, -np.pi/2),
        (0.5, 0.3),
        (-0.5, -0.3)
    ]

    for theta1, theta2 in test_cases_forward:
        alpha, beta = WristKinematics.forward_kinematics(theta1, theta2)
        print(f"θ₁={WristKinematics.rad_to_deg(theta1):6.2f}°, θ₂={WristKinematics.rad_to_deg(theta2):6.2f}° → "
              f"α={WristKinematics.rad_to_deg(alpha):6.2f}°, β={WristKinematics.rad_to_deg(beta):6.2f}°")

    # Test inverse kinematics
    print("\n2. Testing Inverse Kinematics")
    print("-" * 40)

    test_cases_inverse = [
        (0.0, 0.0),
        (np.pi/4, np.pi/4),
        (np.pi/4, -np.pi/4),
        (0.3, 0.2),
        (-0.3, -0.2)
    ]

    for alpha, beta in test_cases_inverse:
        theta1, theta2 = WristKinematics.inverse_kinematics(alpha, beta)
        print(f"α={WristKinematics.rad_to_deg(alpha):6.2f}°, β={WristKinematics.rad_to_deg(beta):6.2f}° → "
              f"θ₁={WristKinematics.rad_to_deg(theta1):6.2f}°, θ₂={WristKinematics.rad_to_deg(theta2):6.2f}°")

    # Test round-trip consistency
    print("\n3. Testing Round-trip Consistency")
    print("-" * 40)

    test_cases_roundtrip = [
        (0.0, 0.0),
        (np.pi/4, np.pi/6),
        (-np.pi/3, np.pi/5),
        (0.7, -0.4),
        (-0.2, -0.8)
    ]

    max_error = 0.0
    for theta1, theta2 in test_cases_roundtrip:
        alpha, beta = WristKinematics.forward_kinematics(theta1, theta2)
        theta1_back, theta2_back = WristKinematics.inverse_kinematics(alpha, beta)

        error1 = abs(theta1 - theta1_back)
        error2 = abs(theta2 - theta2_back)
        max_error = max(max_error, error1, error2)

        print(f"θ₁={WristKinematics.rad_to_deg(theta1):6.2f}°, θ₂={WristKinematics.rad_to_deg(theta2):6.2f}° → "
              f"Errors: θ₁={error1:.6f}, θ₂={error2:.6f}")

    print(f"\nMaximum round-trip error: {max_error:.8f} rad")

    # Test WristController
    print("\n4. Testing WristController")
    print("-" * 40)

    controller = WristController()

    # Test some commanded positions
    commands = [
        (0.0, 0.0),
        (np.pi/6, np.pi/8),
        (-np.pi/8, np.pi/6),
        (0.2, -0.15)
    ]

    for alpha_cmd, beta_cmd in commands:
        theta1_cmd, theta2_cmd = controller.command_output_angles(alpha_cmd, beta_cmd)
        alpha_actual, beta_actual = controller.get_output_angles()

        print(f"Command: α={WristKinematics.rad_to_deg(alpha_cmd):6.2f}°, β={WristKinematics.rad_to_deg(beta_cmd):6.2f}°")
        print(f"Motors:   θ₁={WristKinematics.rad_to_deg(theta1_cmd):6.2f}°, θ₂={WristKinematics.rad_to_deg(theta2_cmd):6.2f}°")
        print(f"Actual:   α={WristKinematics.rad_to_deg(alpha_actual):6.2f}°, β={WristKinematics.rad_to_deg(beta_actual):6.2f}°")
        print()

    print("=" * 60)
    print("ALL TESTS COMPLETED")
    print("=" * 60)


if __name__ == "__main__":
    run_tests()