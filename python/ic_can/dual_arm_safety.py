"""
Dual-Arm Safety and Coordination System

This module provides safety monitoring and coordination capabilities for the dual-arm
robotic system, including collision detection, workspace monitoring, and emergency procedures.

Features:
    - Inter-arm collision detection
    - Workspace boundary monitoring
    - Emergency stop handling
    - Coordinated movement planning
    - Safety zone management
"""

import numpy as np
import time
import logging
from typing import Dict, List, Optional, Tuple, Any, Set
from dataclasses import dataclass
from enum import Enum
import math

from .dual_arm_manager import DualArmManager, DualArmState, ArmState

# Setup logging
logger = logging.getLogger(__name__)

class SafetyLevel(Enum):
    """Safety severity levels"""
    NORMAL = "normal"
    WARNING = "warning"
    CRITICAL = "critical"
    EMERGENCY = "emergency"

class SafetyCondition(Enum):
    """Types of safety conditions"""
    INTER_ARM_COLLISION = "inter_arm_collision"
    WORKSPACE_BOUNDARY = "workspace_boundary"
    SELF_COLLISION = "self_collision"
    VELOCITY_LIMIT = "velocity_limit"
    FORCE_LIMIT = "force_limit"
    COMMUNICATION_LOST = "communication_lost"
    MOTOR_ERROR = "motor_error"

@dataclass
class SafetyEvent:
    """Safety event information"""
    condition: SafetyCondition
    level: SafetyLevel
    timestamp: float
    description: str
    arm_id: Optional[str] = None
    joint_indices: Optional[List[int]] = None
    positions: Optional[np.ndarray] = None
    threshold: Optional[float] = None
    actual_value: Optional[float] = None

@dataclass
class SafetyZone:
    """3D safety zone definition"""
    center: np.ndarray
    radius: float
    zone_type: str  # "forbidden", "caution", "allowed"
    name: str

class DualArmSafety:
    """
    Safety monitoring system for dual-arm operations.

    This class provides comprehensive safety monitoring including collision detection,
    workspace boundaries, and emergency procedures for dual-arm robotic systems.
    """

    def __init__(self,
                 dual_arm_manager: DualArmManager,
                 max_inter_arm_distance: float = 0.2,  # 20cm minimum distance
                 max_velocity: float = 2.0,  # rad/s
                 max_force: float = 50.0,  # Nm
                 enable_collision_detection: bool = True,
                 enable_workspace_monitoring: bool = True):
        """
        Initialize safety system.

        Args:
            dual_arm_manager: Reference to dual-arm manager
            max_inter_arm_distance: Minimum allowed distance between end effectors
            max_velocity: Maximum allowed joint velocity
            max_force: Maximum allowed joint torque/force
            enable_collision_detection: Enable collision detection algorithms
            enable_workspace_monitoring: Enable workspace boundary monitoring
        """
        self.dual_arm_manager = dual_arm_manager
        self.max_inter_arm_distance = max_inter_arm_distance
        self.max_velocity = max_velocity
        self.max_force = max_force
        self.enable_collision_detection = enable_collision_detection
        self.enable_workspace_monitoring = enable_workspace_monitoring

        # Safety zones
        self.safety_zones: List[SafetyZone] = []

        # Safety event history
        self.safety_events: List[SafetyEvent] = []
        self.active_safety_conditions: Set[SafetyCondition] = set()

        # Current safety level
        self.current_safety_level = SafetyLevel.NORMAL

        # Emergency state
        self.emergency_stop_active = False
        self.emergency_stop_reason = ""

        # Monitoring statistics
        self.safety_stats = {
            'events_triggered': 0,
            'emergency_stops': 0,
            'last_check_time': 0.0,
            'check_rate': 0.0
        }

        # Workspace boundaries (simplified - in real implementation would be 3D)
        self.workspace_limits = {
            'left_arm': {
                'min_positions': np.array([-3.0, -2.0, -2.0, -3.0, -2.0, -1.5, -3.0, -3.0, -3.0]),
                'max_positions': np.array([3.0, 2.0, 2.0, 3.0, 2.0, 1.5, 3.0, 3.0, 3.0])
            },
            'right_arm': {
                'min_positions': np.array([-3.0, -2.0, -2.0, -3.0, -2.0, -1.5, -3.0, -3.0, -3.0]),
                'max_positions': np.array([3.0, 2.0, 2.0, 3.0, 2.0, 1.5, 3.0, 3.0, 3.0])
            }
        }

        logger.info("DualArmSafety system initialized")

    def add_safety_zone(self, center: np.ndarray, radius: float, zone_type: str, name: str):
        """Add a safety zone to the monitoring system."""
        zone = SafetyZone(center=center, radius=radius, zone_type=zone_type, name=name)
        self.safety_zones.append(zone)
        logger.info(f"Added safety zone '{name}' ({zone_type}) at {center} with radius {radius}")

    def check_safety(self, state: DualArmState) -> List[SafetyEvent]:
        """
        Perform comprehensive safety check on current system state.

        Args:
            state: Current dual-arm state

        Returns:
            List of safety events triggered
        """
        events = []
        current_time = time.time()

        # Update monitoring statistics
        if self.safety_stats['last_check_time'] > 0:
            dt = current_time - self.safety_stats['last_check_time']
            self.safety_stats['check_rate'] = 1.0 / dt
        self.safety_stats['last_check_time'] = current_time

        try:
            # Check inter-arm collision
            if self.enable_collision_detection:
                collision_events = self._check_inter_arm_collision(state)
                events.extend(collision_events)

            # Check workspace boundaries
            if self.enable_workspace_monitoring:
                workspace_events = self._check_workspace_boundaries(state)
                events.extend(workspace_events)

            # Check velocity limits
            velocity_events = self._check_velocity_limits(state)
            events.extend(velocity_events)

            # Check force/torque limits
            force_events = self._check_force_limits(state)
            events.extend(force_events)

            # Check safety zones
            zone_events = self._check_safety_zones(state)
            events.extend(zone_events)

            # Process events
            for event in events:
                self._process_safety_event(event)

            # Update safety level
            self._update_safety_level()

        except Exception as e:
            logger.error(f"Safety check error: {e}")
            events.append(SafetyEvent(
                condition=SafetyCondition.MOTOR_ERROR,
                level=SafetyLevel.CRITICAL,
                timestamp=current_time,
                description=f"Safety system error: {e}"
            ))

        return events

    def _check_inter_arm_collision(self, state: DualArmState) -> List[SafetyEvent]:
        """Check for potential collisions between arms."""
        events = []

        # Simplified distance check using joint positions
        # In real implementation, this would use forward kinematics to get 3D positions
        left_ee_pos = state.left_arm.positions[:6]  # Approximate end-effector position
        right_ee_pos = state.right_arm.positions[:6]

        # Calculate joint-space distance
        joint_distance = np.linalg.norm(left_ee_pos - right_ee_pos)

        if joint_distance < self.max_inter_arm_distance:
            level = SafetyLevel.EMERGENCY if joint_distance < self.max_inter_arm_distance * 0.5 else SafetyLevel.CRITICAL
            events.append(SafetyEvent(
                condition=SafetyCondition.INTER_ARM_COLLISION,
                level=level,
                timestamp=state.timestamp,
                description=f"Inter-arm distance too small: {joint_distance:.3f}m < {self.max_inter_arm_distance:.3f}m",
                actual_value=joint_distance,
                threshold=self.max_inter_arm_distance
            ))

        return events

    def _check_workspace_boundaries(self, state: DualArmState) -> List[SafetyEvent]:
        """Check if arms are within workspace boundaries."""
        events = []

        # Check left arm
        left_violations = self._check_arm_workspace_boundaries(
            state.left_arm, 'left_arm', state.timestamp
        )
        events.extend(left_violations)

        # Check right arm
        right_violations = self._check_arm_workspace_boundaries(
            state.right_arm, 'right_arm', state.timestamp
        )
        events.extend(right_violations)

        return events

    def _check_arm_workspace_boundaries(self, arm_state: ArmState, arm_id: str, timestamp: float) -> List[SafetyEvent]:
        """Check workspace boundaries for a single arm."""
        events = []
        limits = self.workspace_limits.get(arm_id)
        if not limits:
            return events

        min_pos = limits['min_positions']
        max_pos = limits['max_positions']

        for i, (pos, min_val, max_val) in enumerate(zip(arm_state.positions, min_pos, max_pos)):
            if pos < min_val or pos > max_val:
                violation_type = "below minimum" if pos < min_val else "above maximum"
                threshold = min_val if pos < min_val else max_val
                events.append(SafetyEvent(
                    condition=SafetyCondition.WORKSPACE_BOUNDARY,
                    level=SafetyLevel.WARNING,
                    timestamp=timestamp,
                    description=f"Joint {i} {violation_type}: {pos:.3f} (limit: {threshold:.3f})",
                    arm_id=arm_id,
                    joint_indices=[i],
                    positions=arm_state.positions.copy(),
                    actual_value=pos,
                    threshold=threshold
                ))

        return events

    def _check_velocity_limits(self, state: DualArmState) -> List[SafetyEvent]:
        """Check if velocities are within limits."""
        events = []

        # Check left arm
        for i, vel in enumerate(state.left_arm.velocities):
            if abs(vel) > self.max_velocity:
                events.append(SafetyEvent(
                    condition=SafetyCondition.VELOCITY_LIMIT,
                    level=SafetyLevel.WARNING,
                    timestamp=state.timestamp,
                    description=f"Left arm joint {i} velocity exceeded: {vel:.3f} rad/s > {self.max_velocity:.3f} rad/s",
                    arm_id='left_arm',
                    joint_indices=[i],
                    actual_value=abs(vel),
                    threshold=self.max_velocity
                ))

        # Check right arm
        for i, vel in enumerate(state.right_arm.velocities):
            if abs(vel) > self.max_velocity:
                events.append(SafetyEvent(
                    condition=SafetyCondition.VELOCITY_LIMIT,
                    level=SafetyLevel.WARNING,
                    timestamp=state.timestamp,
                    description=f"Right arm joint {i} velocity exceeded: {vel:.3f} rad/s > {self.max_velocity:.3f} rad/s",
                    arm_id='right_arm',
                    joint_indices=[i],
                    actual_value=abs(vel),
                    threshold=self.max_velocity
                ))

        return events

    def _check_force_limits(self, state: DualArmState) -> List[SafetyEvent]:
        """Check if forces/torques are within limits."""
        events = []

        # Check left arm
        for i, torque in enumerate(state.left_arm.torques):
            if abs(torque) > self.max_force:
                events.append(SafetyEvent(
                    condition=SafetyCondition.FORCE_LIMIT,
                    level=SafetyLevel.CRITICAL,
                    timestamp=state.timestamp,
                    description=f"Left arm joint {i} torque exceeded: {torque:.3f} Nm > {self.max_force:.3f} Nm",
                    arm_id='left_arm',
                    joint_indices=[i],
                    actual_value=abs(torque),
                    threshold=self.max_force
                ))

        # Check right arm
        for i, torque in enumerate(state.right_arm.torques):
            if abs(torque) > self.max_force:
                events.append(SafetyEvent(
                    condition=SafetyCondition.FORCE_LIMIT,
                    level=SafetyLevel.CRITICAL,
                    timestamp=state.timestamp,
                    description=f"Right arm joint {i} torque exceeded: {torque:.3f} Nm > {self.max_force:.3f} Nm",
                    arm_id='right_arm',
                    joint_indices=[i],
                    actual_value=abs(torque),
                    threshold=self.max_force
                ))

        return events

    def _check_safety_zones(self, state: DualArmState) -> List[SafetyEvent]:
        """Check if arms violate any safety zones."""
        events = []

        for zone in self.safety_zones:
            # Simplified check using joint positions
            # In real implementation, this would use actual 3D positions

            # Check left arm
            left_ee_pos = state.left_arm.positions[:3]  # Simplified position
            distance_to_zone = np.linalg.norm(left_ee_pos - zone.center[:3])

            if zone.zone_type == "forbidden" and distance_to_zone < zone.radius:
                events.append(SafetyEvent(
                    condition=SafetyCondition.WORKSPACE_BOUNDARY,
                    level=SafetyLevel.EMERGENCY,
                    timestamp=state.timestamp,
                    description=f"Left arm entered forbidden zone '{zone.name}': distance {distance_to_zone:.3f}m",
                    arm_id='left_arm',
                    positions=state.left_arm.positions.copy()
                ))

            elif zone.zone_type == "caution" and distance_to_zone < zone.radius:
                events.append(SafetyEvent(
                    condition=SafetyCondition.WORKSPACE_BOUNDARY,
                    level=SafetyLevel.WARNING,
                    timestamp=state.timestamp,
                    description=f"Left arm entered caution zone '{zone.name}': distance {distance_to_zone:.3f}m",
                    arm_id='left_arm',
                    positions=state.left_arm.positions.copy()
                ))

        return events

    def _process_safety_event(self, event: SafetyEvent):
        """Process a safety event and take appropriate action."""
        # Add to event history
        self.safety_events.append(event)
        self.safety_stats['events_triggered'] += 1

        # Keep only recent events (last 100)
        if len(self.safety_events) > 100:
            self.safety_events = self.safety_events[-100:]

        # Add to active conditions
        self.active_safety_conditions.add(event.condition)

        # Log event
        logger.warning(f"Safety event [{event.level.value.upper()}]: {event.description}")

        # Take action based on severity
        if event.level == SafetyLevel.EMERGENCY:
            self.trigger_emergency_stop(f"Safety violation: {event.description}")

    def _update_safety_level(self):
        """Update current safety level based on active conditions."""
        if any(event.level == SafetyLevel.EMERGENCY for event in self.safety_events[-10:]):
            self.current_safety_level = SafetyLevel.EMERGENCY
        elif any(event.level == SafetyLevel.CRITICAL for event in self.safety_events[-10:]):
            self.current_safety_level = SafetyLevel.CRITICAL
        elif any(event.level == SafetyLevel.WARNING for event in self.safety_events[-10:]):
            self.current_safety_level = SafetyLevel.WARNING
        else:
            self.current_safety_level = SafetyLevel.NORMAL

    def trigger_emergency_stop(self, reason: str = ""):
        """Trigger emergency stop for both arms."""
        if not self.emergency_stop_active:
            self.emergency_stop_active = True
            self.emergency_stop_reason = reason
            self.safety_stats['emergency_stops'] += 1

            logger.error(f"EMERGENCY STOP TRIGGERED: {reason}")

            # Stop both arms
            if self.dual_arm_manager:
                self.dual_arm_manager.emergency_stop()

    def clear_emergency_stop(self):
        """Clear emergency stop condition."""
        if self.emergency_stop_active:
            logger.info("Emergency stop cleared")
            self.emergency_stop_active = False
            self.emergency_stop_reason = ""
            self.active_safety_conditions.clear()

    def is_safe_to_move(self, target_positions_left: np.ndarray,
                       target_positions_right: np.ndarray) -> Tuple[bool, str]:
        """
        Check if it's safe to move to target positions.

        Args:
            target_positions_left: Target positions for left arm
            target_positions_right: Target positions for right arm

        Returns:
            Tuple of (is_safe, reason)
        """
        if self.emergency_stop_active:
            return False, "Emergency stop is active"

        # Check workspace boundaries
        for i, pos in enumerate(target_positions_left):
            min_val = self.workspace_limits['left_arm']['min_positions'][i]
            max_val = self.workspace_limits['left_arm']['max_positions'][i]
            if pos < min_val or pos > max_val:
                return False, f"Left arm joint {i} position {pos:.3f} outside workspace [{min_val:.3f}, {max_val:.3f}]"

        for i, pos in enumerate(target_positions_right):
            min_val = self.workspace_limits['right_arm']['min_positions'][i]
            max_val = self.workspace_limits['right_arm']['max_positions'][i]
            if pos < min_val or pos > max_val:
                return False, f"Right arm joint {i} position {pos:.3f} outside workspace [{min_val:.3f}, {max_val:.3f}]"

        # Check inter-arm distance (simplified)
        distance = np.linalg.norm(target_positions_left[:6] - target_positions_right[:6])
        if distance < self.max_inter_arm_distance:
            return False, f"Target inter-arm distance {distance:.3f}m below minimum {self.max_inter_arm_distance:.3f}m"

        return True, "Safe to move"

    def get_safety_summary(self) -> Dict[str, Any]:
        """Get comprehensive safety system summary."""
        recent_events = self.safety_events[-10:] if self.safety_events else []

        return {
            'current_level': self.current_safety_level.value,
            'emergency_stop_active': self.emergency_stop_active,
            'emergency_stop_reason': self.emergency_stop_reason,
            'active_conditions': [cond.value for cond in self.active_safety_conditions],
            'recent_events': [
                {
                    'condition': event.condition.value,
                    'level': event.level.value,
                    'description': event.description,
                    'timestamp': event.timestamp
                } for event in recent_events
            ],
            'statistics': self.safety_stats.copy(),
            'configuration': {
                'max_inter_arm_distance': self.max_inter_arm_distance,
                'max_velocity': self.max_velocity,
                'max_force': self.max_force,
                'collision_detection_enabled': self.enable_collision_detection,
                'workspace_monitoring_enabled': self.enable_workspace_monitoring,
                'safety_zones_count': len(self.safety_zones)
            }
        }

    def print_safety_status(self):
        """Print current safety system status."""
        summary = self.get_safety_summary()

        print("\n⚠️  Safety System Status:")
        print("=" * 50)
        print(f"Current Level: {summary['current_level'].upper()}")
        print(f"Emergency Stop: {'🚨 ACTIVE' if summary['emergency_stop_active'] else '✅ Normal'}")

        if summary['emergency_stop_active']:
            print(f"Reason: {summary['emergency_stop_reason']}")

        if summary['active_conditions']:
            print(f"Active Conditions: {', '.join(summary['active_conditions'])}")

        print(f"Recent Events: {len(summary['recent_events'])}")
        for event in summary['recent_events'][-3:]:  # Show last 3 events
            print(f"  [{event['level'].upper()}] {event['description']}")

        print(f"\nConfiguration:")
        print(f"  Max Inter-arm Distance: {summary['configuration']['max_inter_arm_distance']}m")
        print(f"  Max Velocity: {summary['configuration']['max_velocity']} rad/s")
        print(f"  Max Force: {summary['configuration']['max_force']} Nm")
        print(f"  Safety Zones: {summary['configuration']['safety_zones_count']}")
        print(f"  Collision Detection: {'✅' if summary['configuration']['collision_detection_enabled'] else '❌'}")
        print(f"  Workspace Monitoring: {'✅' if summary['configuration']['workspace_monitoring_enabled'] else '❌'}")

        print(f"\nStatistics:")
        print(f"  Events Triggered: {summary['statistics']['events_triggered']}")
        print(f"  Emergency Stops: {summary['statistics']['emergency_stops']}")
        print(f"  Check Rate: {summary['statistics']['check_rate']:.1f} Hz")
        print("=" * 50)