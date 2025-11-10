"""
Mock IC_CAN Python bindings for testing purposes.

This provides a mock implementation of the IC_CAN class for testing
the dual-arm system without requiring actual hardware or compiled bindings.
"""

import numpy as np
import time
from typing import Optional, List, Dict, Any

class MockIC_CAN:
    """Mock implementation of IC_CAN class for testing."""

    def __init__(self, device_sn: str = "mock_device", debug: bool = False):
        self.device_sn = device_sn
        self.debug = debug
        self.connected = False
        self.initialized = False

        # Mock motor state (9 motors)
        self.positions = np.zeros(9)
        self.velocities = np.zeros(9)
        self.torques = np.zeros(9)

        # Mock configuration
        self.motor_gains = {
            'kp': np.array([400.0, 200.0, 200.0, 100.0, 150.0, 15.0, 80.0, 80.0, 0.0]),
            'kd': np.array([4.0, 2.5, 2.5, 1.5, 1.5, 0.8, 1.0, 1.0, 0.0])
        }

        if self.debug:
            print(f"🔧 Mock IC_CAN created with device SN: {device_sn}")

    def load_default_motor_gains(self):
        """Mock load default motor gains."""
        if self.debug:
            print("🔧 Mock default motor gains loaded")

    def enable_all(self) -> bool:
        """Mock enable all motors."""
        if not self.connected:
            return False

        if self.debug:
            print("🔧 Mock all motors enabled")

        return True

    def disable_all(self) -> bool:
        """Mock disable all motors."""
        if not self.connected:
            return False

        if self.debug:
            print("🔧 Mock all motors disabled")

        return True

    def initialize(self) -> bool:
        """Mock initialization."""
        if self.debug:
            print("🔧 Mock IC_CAN initializing...")

        time.sleep(0.1)  # Simulate initialization time
        self.initialized = True
        self.connected = True

        if self.debug:
            print("✅ Mock IC_CAN initialized successfully")

        return True

    def shutdown(self):
        """Mock shutdown."""
        if self.debug:
            print("🔧 Mock IC_CAN shutting down...")

        self.connected = False
        self.initialized = False

    def enable(self) -> bool:
        """Mock enable motors."""
        if not self.connected:
            return False

        if self.debug:
            print("🔧 Mock motors enabled")

        return True

    def disable(self) -> bool:
        """Mock disable motors."""
        if not self.connected:
            return False

        if self.debug:
            print("🔧 Mock motors disabled")

        return True

    def refresh(self) -> bool:
        """Mock refresh motor states."""
        if not self.connected:
            return False

        # Simulate small random changes in state
        self.positions += np.random.normal(0, 0.001, 9)
        self.velocities = np.random.normal(0, 0.01, 9)
        self.torques = np.random.normal(0, 0.1, 9)

        return True

    def step(self, positions: np.ndarray, velocities: Optional[np.ndarray] = None,
             torques: Optional[np.ndarray] = None) -> bool:
        """Mock step - send commands to motors."""
        if not self.connected:
            return False

        if len(positions) != 9:
            return False

        # Update mock positions
        self.positions = positions.copy()

        if self.debug:
            print(f"🔧 Mock step: positions={positions[:3]}...")

        return True

    def get_joint_positions(self) -> List[float]:
        """Get mock joint positions."""
        return self.positions.tolist()

    def get_joint_velocities(self) -> List[float]:
        """Get mock joint velocities."""
        return self.velocities.tolist()

    def get_joint_torques(self) -> List[float]:
        """Get mock joint torques."""
        return self.torques.tolist()

    def set_control_mode(self, mode: str) -> bool:
        """Mock set control mode."""
        if self.debug:
            print(f"🔧 Mock control mode set to: {mode}")

        return True

    def is_connected(self) -> bool:
        """Check if mock is connected."""
        return self.connected

    def refresh_all(self) -> bool:
        """Mock refresh all motors."""
        if not self.connected:
            return False

        return self.refresh()

    def get_performance_stats(self) -> Dict[str, float]:
        """Get mock performance stats."""
        return {
            'send_frequency': 500.0,
            'receive_frequency': 500.0,
            'total_commands_sent': 1000.0,
            'total_messages_received': 950.0,
            'uptime_seconds': 10.0
        }

# Mock the IC_CAN class name for imports
IC_CAN = MockIC_CAN