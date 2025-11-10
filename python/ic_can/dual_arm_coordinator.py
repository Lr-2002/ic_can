"""
Dual-Arm Coordination System

This module provides coordinated control capabilities for dual-arm operations,
including synchronized movement, bimanual task planning, and coordination algorithms.

Features:
    - Synchronized arm movement
    - Bimanual task planning
    - Coordinated trajectories
    - Task-level coordination
    - Relative motion control
"""

import numpy as np
import time
import logging
from typing import Dict, List, Optional, Tuple, Any, Callable
from dataclasses import dataclass
from enum import Enum
import threading
from queue import Queue

from .dual_arm_manager import DualArmManager, DualArmState
from .dual_arm_safety import DualArmSafety

# Setup logging
logger = logging.getLogger(__name__)

class CoordinationMode(Enum):
    """Coordination modes for dual-arm operations"""
    INDEPENDENT = "independent"           # Arms move independently
    SYNCHRONIZED = "synchronized"        # Arms move synchronously
    RELATIVE = "relative"               # Relative motion control
    MASTER_SLAVE = "master_slave"        # One arm follows the other
    COOPERATIVE = "cooperative"          # Cooperative manipulation

class TaskType(Enum):
    """Types of bimanual tasks"""
    REACH_GRASP = "reach_grasp"          # Reach and grasp with both arms
    TRANSPORT = "transport"              # Transport object together
    ASSEMBLY = "assembly"                # Assembly operations
    HANDOVER = "handover"                # Object handover between arms
    DUAL_TOOL = "dual_tool"              # Using tools with both arms

@dataclass
class CoordinationGoal:
    """Goal specification for coordinated movement"""
    mode: CoordinationMode
    left_target: np.ndarray
    right_target: np.ndarray
    duration: float
    relative_constraint: Optional[np.ndarray] = None  # For relative mode
    synchronization_tolerance: float = 0.1  # seconds

@dataclass
class CoordinationState:
    """Current state of coordination system"""
    current_mode: CoordinationMode
    progress: float  # 0.0 to 1.0
    estimated_completion: float
    is_active: bool
    current_goal: Optional[CoordinationGoal] = None

class DualArmCoordinator:
    """
    Coordination system for dual-arm operations.

    This class provides high-level coordination capabilities including synchronized
    movement, bimanual task planning, and coordinated control strategies.
    """

    def __init__(self,
                 dual_arm_manager: DualArmManager,
                 safety_system: Optional[DualArmSafety] = None):
        """
        Initialize dual-arm coordinator.

        Args:
            dual_arm_manager: Reference to dual-arm manager
            safety_system: Optional safety system reference
        """
        self.dual_arm_manager = dual_arm_manager
        self.safety_system = safety_system

        # Coordination state
        self.coordination_state = CoordinationState(
            current_mode=CoordinationMode.INDEPENDENT,
            progress=0.0,
            estimated_completion=0.0,
            is_active=False
        )

        # Goal queue and execution
        self.goal_queue: Queue[CoordinationGoal] = Queue()
        self.coordination_thread: Optional[threading.Thread] = None
        self.stop_coordination = threading.Event()

        # Current trajectory data
        self.current_trajectory: Optional[Dict[str, np.ndarray]] = None
        self.trajectory_start_time: float = 0.0
        self.trajectory_duration: float = 0.0

        # Coordination parameters
        self.coordination_params = {
            'synchronization_tolerance': 0.1,  # seconds
            'max_velocity_scaling': 0.8,       # Scale factor for coordinated motion
            'inter_arm_coupling': 0.5,         # Coupling strength between arms
            'task_space_tolerance': 0.05,      # Task space tolerance (meters)
        }

        # Performance statistics
        self.coordination_stats = {
            'tasks_completed': 0,
            'tasks_failed': 0,
            'average_execution_time': 0.0,
            'synchronization_errors': 0,
            'coordination_rate': 0.0
        }

        logger.info("DualArmCoordinator initialized")

    def start_coordination(self):
        """Start the coordination thread."""
        if self.coordination_thread and self.coordination_thread.is_alive():
            logger.warning("Coordination thread already running")
            return

        self.stop_coordination.clear()
        self.coordination_thread = threading.Thread(target=self._coordination_loop)
        self.coordination_thread.daemon = True
        self.coordination_thread.start()
        logger.info("Coordination thread started")

    def stop_coordination_thread(self):
        """Stop the coordination thread."""
        if self.coordination_thread and self.coordination_thread.is_alive():
            self.stop_coordination.set()
            self.coordination_thread.join(timeout=1.0)
            logger.info("Coordination thread stopped")

    def _coordination_loop(self):
        """Main coordination loop running in separate thread."""
        last_update_time = time.time()

        while not self.stop_coordination.is_set():
            try:
                current_time = time.time()
                dt = current_time - last_update_time
                last_update_time = current_time

                # Update coordination statistics
                self.coordination_stats['coordination_rate'] = 1.0 / dt

                # Execute current goal or wait for new goal
                if not self.coordination_state.is_active and not self.goal_queue.empty():
                    goal = self.goal_queue.get_nowait()
                    self._start_coordination_goal(goal)

                elif self.coordination_state.is_active:
                    self._execute_coordination_goal(current_time)

                # Brief sleep to prevent excessive CPU usage
                time.sleep(0.001)

            except Exception as e:
                logger.error(f"Coordination loop error: {e}")
                time.sleep(0.01)

    def add_coordination_goal(self, goal: CoordinationGoal) -> bool:
        """
        Add a coordination goal to the execution queue.

        Args:
            goal: Coordination goal to execute

        Returns:
            True if goal was added successfully
        """
        try:
            # Validate goal
            if not self._validate_goal(goal):
                return False

            # Add to queue
            self.goal_queue.put(goal)
            logger.info(f"Added coordination goal: {goal.mode.value}")
            return True

        except Exception as e:
            logger.error(f"Failed to add coordination goal: {e}")
            return False

    def _validate_goal(self, goal: CoordinationGoal) -> bool:
        """Validate a coordination goal."""
        # Check target dimensions
        if goal.left_target.shape[0] != 9 or goal.right_target.shape[0] != 9:
            logger.error("Invalid target dimensions: must have 9 elements per arm")
            return False

        # Check duration
        if goal.duration <= 0:
            logger.error("Invalid duration: must be positive")
            return False

        # Safety check if safety system is available
        if self.safety_system:
            is_safe, reason = self.safety_system.is_safe_to_move(
                goal.left_target, goal.right_target
            )
            if not is_safe:
                logger.error(f"Goal not safe: {reason}")
                return False

        return True

    def _start_coordination_goal(self, goal: CoordinationGoal):
        """Start executing a coordination goal."""
        logger.info(f"Starting coordination goal: {goal.mode.value}")

        # Get current state
        current_state = self.dual_arm_manager.get_current_state()
        if not current_state:
            logger.error("Cannot get current state")
            return

        # Generate trajectory based on coordination mode
        self.current_trajectory = self._generate_coordination_trajectory(
            current_state, goal
        )

        if not self.current_trajectory:
            logger.error("Failed to generate coordination trajectory")
            return

        # Update coordination state
        self.coordination_state.current_mode = goal.mode
        self.coordination_state.current_goal = goal
        self.coordination_state.progress = 0.0
        self.coordination_state.estimated_completion = time.time() + goal.duration
        self.coordination_state.is_active = True

        self.trajectory_start_time = time.time()
        self.trajectory_duration = goal.duration

    def _generate_coordination_trajectory(self, current_state: DualArmState, goal: CoordinationGoal) -> Optional[Dict[str, np.ndarray]]:
        """Generate trajectory for coordinated movement."""
        try:
            num_points = int(goal.duration * 50)  # 50 Hz trajectory generation
            t = np.linspace(0, 1, num_points)

            # Generate smooth interpolation profiles
            if goal.mode == CoordinationMode.SYNCHRONIZED:
                return self._generate_synchronized_trajectory(
                    current_state, goal, t
                )
            elif goal.mode == CoordinationMode.RELATIVE:
                return self._generate_relative_trajectory(
                    current_state, goal, t
                )
            elif goal.mode == CoordinationMode.MASTER_SLAVE:
                return self._generate_master_slave_trajectory(
                    current_state, goal, t
                )
            else:
                # Default to independent trajectories
                return self._generate_independent_trajectories(
                    current_state, goal, t
                )

        except Exception as e:
            logger.error(f"Trajectory generation failed: {e}")
            return None

    def _generate_synchronized_trajectory(self, current_state: DualArmState, goal: CoordinationGoal, t: np.ndarray) -> Dict[str, np.ndarray]:
        """Generate synchronized trajectory for both arms."""
        # Smooth interpolation using minimum-jerk profile
        s = self._minimum_jerk_profile(t)

        # Interpolate positions for both arms
        left_trajectory = np.zeros((len(t), 9))
        right_trajectory = np.zeros((len(t), 9))

        for i in range(9):
            left_trajectory[:, i] = current_state.left_arm.positions[i] + s * (goal.left_target[i] - current_state.left_arm.positions[i])
            right_trajectory[:, i] = current_state.right_arm.positions[i] + s * (goal.right_target[i] - current_state.right_arm.positions[i])

        return {
            'left_positions': left_trajectory,
            'right_positions': right_trajectory,
            'time_points': t,
            'profile': s
        }

    def _generate_relative_trajectory(self, current_state: DualArmState, goal: CoordinationGoal, t: np.ndarray) -> Dict[str, np.ndarray]:
        """Generate relative motion trajectory."""
        s = self._minimum_jerk_profile(t)

        # Calculate relative motion
        if goal.relative_constraint is not None:
            # Maintain specified relative pose
            left_trajectory = np.zeros((len(t), 9))
            right_trajectory = np.zeros((len(t), 9))

            for i in range(9):
                left_trajectory[:, i] = current_state.left_arm.positions[i] + s * (goal.left_target[i] - current_state.left_arm.positions[i])
                # Right arm follows to maintain relative constraint
                right_trajectory[:, i] = left_trajectory[:, i] + goal.relative_constraint[i]
        else:
            # Default synchronized motion
            return self._generate_synchronized_trajectory(current_state, goal, t)

        return {
            'left_positions': left_trajectory,
            'right_positions': right_trajectory,
            'time_points': t,
            'profile': s
        }

    def _generate_master_slave_trajectory(self, current_state: DualArmState, goal: CoordinationGoal, t: np.ndarray) -> Dict[str, np.ndarray]:
        """Generate master-slave trajectory (left arm is master)."""
        s = self._minimum_jerk_profile(t)

        # Master trajectory (left arm)
        left_trajectory = np.zeros((len(t), 9))
        for i in range(9):
            left_trajectory[:, i] = current_state.left_arm.positions[i] + s * (goal.left_target[i] - current_state.left_arm.positions[i])

        # Slave trajectory (right arm follows with scaling)
        right_trajectory = np.zeros((len(t), 9))
        scaling_factor = 0.8  # Right arm moves 80% of left arm motion

        for i in range(9):
            if goal.right_target[i] != 0:  # Non-zero target
                right_trajectory[:, i] = current_state.right_arm.positions[i] + scaling_factor * s * (goal.right_target[i] - current_state.right_arm.positions[i])

        return {
            'left_positions': left_trajectory,
            'right_positions': right_trajectory,
            'time_points': t,
            'profile': s
        }

    def _generate_independent_trajectories(self, current_state: DualArmState, goal: CoordinationGoal, t: np.ndarray) -> Dict[str, np.ndarray]:
        """Generate independent trajectories for each arm."""
        s = self._minimum_jerk_profile(t)

        left_trajectory = np.zeros((len(t), 9))
        right_trajectory = np.zeros((len(t), 9))

        for i in range(9):
            left_trajectory[:, i] = current_state.left_arm.positions[i] + s * (goal.left_target[i] - current_state.left_arm.positions[i])
            right_trajectory[:, i] = current_state.right_arm.positions[i] + s * (goal.right_target[i] - current_state.right_arm.positions[i])

        return {
            'left_positions': left_trajectory,
            'right_positions': right_trajectory,
            'time_points': t,
            'profile': s
        }

    def _minimum_jerk_profile(self, t: np.ndarray) -> np.ndarray:
        """Generate minimum-jerk trajectory profile."""
        # Minimum jerk: s(t) = 10t³ - 15t⁴ + 6t⁵
        return 10 * t**3 - 15 * t**4 + 6 * t**5

    def _execute_coordination_goal(self, current_time: float):
        """Execute the current coordination goal."""
        if not self.current_trajectory:
            return

        # Calculate progress
        elapsed_time = current_time - self.trajectory_start_time
        progress = min(elapsed_time / self.trajectory_duration, 1.0)

        # Get trajectory index
        trajectory_length = len(self.current_trajectory['time_points'])
        trajectory_index = min(int(progress * (trajectory_length - 1)), trajectory_length - 1)

        # Extract target positions
        left_target = self.current_trajectory['left_positions'][trajectory_index]
        right_target = self.current_trajectory['right_positions'][trajectory_index]

        # Send commands to arms
        success = self.dual_arm_manager.move_both_arms(left_target, right_target)

        # Update progress
        self.coordination_state.progress = progress

        # Check if completed
        if progress >= 1.0:
            self._complete_coordination_goal(success)
        elif not success:
            logger.warning("Failed to send coordinated movement commands")
            self.coordination_stats['synchronization_errors'] += 1

    def _complete_coordination_goal(self, success: bool):
        """Complete the current coordination goal."""
        if success:
            self.coordination_stats['tasks_completed'] += 1
            logger.info("Coordination goal completed successfully")
        else:
            self.coordination_stats['tasks_failed'] += 1
            logger.error("Coordination goal failed")

        # Update statistics
        execution_time = time.time() - self.trajectory_start_time
        if self.coordination_stats['tasks_completed'] > 0:
            total_tasks = self.coordination_stats['tasks_completed'] + self.coordination_stats['tasks_failed']
            self.coordination_stats['average_execution_time'] = (
                (self.coordination_stats['average_execution_time'] * (total_tasks - 1) + execution_time) / total_tasks
            )

        # Reset coordination state
        self.coordination_state.is_active = False
        self.coordination_state.current_goal = None
        self.coordination_state.progress = 0.0
        self.current_trajectory = None

    # High-level coordination methods
    def synchronized_move(self, left_target: np.ndarray, right_target: np.ndarray, duration: float) -> bool:
        """Move both arms synchronously to target positions."""
        goal = CoordinationGoal(
            mode=CoordinationMode.SYNCHRONIZED,
            left_target=left_target,
            right_target=right_target,
            duration=duration
        )
        return self.add_coordination_goal(goal)

    def relative_move(self, left_target: np.ndarray, relative_constraint: np.ndarray, duration: float) -> bool:
        """Move arms with relative constraint."""
        # Calculate right arm target based on relative constraint
        right_target = left_target + relative_constraint

        goal = CoordinationGoal(
            mode=CoordinationMode.RELATIVE,
            left_target=left_target,
            right_target=right_target,
            duration=duration,
            relative_constraint=relative_constraint
        )
        return self.add_coordination_goal(goal)

    def transport_object(self, start_positions: Tuple[np.ndarray, np.ndarray],
                        end_positions: Tuple[np.ndarray, np.ndarray],
                        duration: float) -> bool:
        """Transport object cooperatively from start to end positions."""
        # Move to start positions
        start_goal = CoordinationGoal(
            mode=CoordinationMode.SYNCHRONIZED,
            left_target=start_positions[0],
            right_target=start_positions[1],
            duration=duration * 0.3  # 30% of time to reach start
        )

        # Transport to end positions
        end_goal = CoordinationGoal(
            mode=CoordinationMode.COOPERATIVE,
            left_target=end_positions[0],
            right_target=end_positions[1],
            duration=duration * 0.7  # 70% of time for transport
        )

        return (self.add_coordination_goal(start_goal) and
                self.add_coordination_goal(end_goal))

    def handover_object(self, from_arm: str, to_arm: str, handover_position: np.ndarray, duration: float) -> bool:
        """Perform object handover between arms."""
        if from_arm == "left" and to_arm == "right":
            # Left arm moves to handover position
            approach_goal = CoordinationGoal(
                mode=CoordinationMode.INDEPENDENT,
                left_target=handover_position,
                right_target=np.zeros(9),  # Right arm waits
                duration=duration * 0.4
            )

            # Both arms meet at handover position
            handover_goal = CoordinationGoal(
                mode=CoordinationMode.SYNCHRONIZED,
                left_target=handover_position,
                right_target=handover_position,
                duration=duration * 0.3
            )

            # Right arm retreats with object
            retreat_goal = CoordinationGoal(
                mode=CoordinationMode.INDEPENDENT,
                left_target=handover_position,  # Left arm stays
                right_target=np.zeros(9),       # Right arm retreats
                duration=duration * 0.3
            )

        elif from_arm == "right" and to_arm == "left":
            # Mirror of above with roles reversed
            approach_goal = CoordinationGoal(
                mode=CoordinationMode.INDEPENDENT,
                left_target=np.zeros(9),        # Left arm waits
                right_target=handover_position,
                duration=duration * 0.4
            )

            handover_goal = CoordinationGoal(
                mode=CoordinationMode.SYNCHRONIZED,
                left_target=handover_position,
                right_target=handover_position,
                duration=duration * 0.3
            )

            retreat_goal = CoordinationGoal(
                mode=CoordinationMode.INDEPENDENT,
                left_target=np.zeros(9),        # Left arm retreats with object
                right_target=handover_position, # Right arm stays
                duration=duration * 0.3
            )
        else:
            logger.error("Invalid handover arm specification")
            return False

        return (self.add_coordination_goal(approach_goal) and
                self.add_coordination_goal(handover_goal) and
                self.add_coordination_goal(retreat_goal))

    def get_coordination_status(self) -> Dict[str, Any]:
        """Get current coordination system status."""
        return {
            'state': {
                'current_mode': self.coordination_state.current_mode.value,
                'progress': self.coordination_state.progress,
                'is_active': self.coordination_state.is_active,
                'estimated_completion': self.coordination_state.estimated_completion
            },
            'queue_size': self.goal_queue.qsize(),
            'statistics': self.coordination_stats.copy(),
            'parameters': self.coordination_params.copy()
        }

    def print_coordination_status(self):
        """Print coordination system status."""
        status = self.get_coordination_status()

        print("\n🤝 Coordination System Status:")
        print("=" * 50)
        print(f"Current Mode: {status['state']['current_mode']}")
        print(f"Active: {'✅ Yes' if status['state']['is_active'] else '❌ No'}")
        print(f"Progress: {status['state']['progress']:.1%}")
        print(f"Queue Size: {status['queue_size']}")

        print(f"\nStatistics:")
        print(f"  Tasks Completed: {status['statistics']['tasks_completed']}")
        print(f"  Tasks Failed: {status['statistics']['tasks_failed']}")
        print(f"  Average Execution Time: {status['statistics']['average_execution_time']:.2f}s")
        print(f"  Synchronization Errors: {status['statistics']['synchronization_errors']}")
        print(f"  Coordination Rate: {status['statistics']['coordination_rate']:.1f} Hz")

        print(f"\nParameters:")
        print(f"  Sync Tolerance: {status['parameters']['synchronization_tolerance']}s")
        print(f"  Velocity Scaling: {status['parameters']['max_velocity_scaling']}")
        print(f"  Inter-arm Coupling: {status['parameters']['inter_arm_coupling']}")
        print("=" * 50)

    def shutdown(self):
        """Shutdown coordination system."""
        logger.info("Shutting down coordination system...")

        # Stop coordination thread
        self.stop_coordination_thread()

        # Clear queues
        while not self.goal_queue.empty():
            try:
                self.goal_queue.get_nowait()
            except:
                break

        # Reset state
        self.coordination_state.is_active = False
        self.current_trajectory = None

        logger.info("Coordination system shutdown complete")