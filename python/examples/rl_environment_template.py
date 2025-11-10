#!/usr/bin/env python3
"""
RL Environment Template for IC_ARM

This file provides a template for integrating the IC_ARM robot with
reinforcement learning frameworks like Stable Baselines3, RLlib, or custom RL code.

The template demonstrates how to:
1. Create a gym-compatible environment
2. Define observation and action spaces
3. Handle safety and error conditions
4. Implement step() and reset() methods
5. Manage episode termination and rewards
"""

import sys
import time
import numpy as np
from pathlib import Path
from typing import Tuple, Dict, Any, Optional

# Add the python directory to the path for development
python_dir = Path(__file__).parent.parent
sys.path.insert(0, str(python_dir))

import ic_can
from ic_can import RobotArm, ControlMode, MotorState

try:
    import gym
    from gym import spaces
    GYM_AVAILABLE = True
except ImportError:
    print("Warning: gym not available. Install with: pip install gym")
    GYM_AVAILABLE = False


class ICArmEnvironment:
    """
    RL Environment template for IC_ARM robotic arm.

    This environment provides a clean interface for training RL agents
    to control the 9-DOF robotic arm. It handles all the low-level
    details while providing standard RL environment methods.
    """

    def __init__(self,
                 device_sn: str = "F561E08C892274DB09496BCC1102DBC5",
                 debug: bool = False,
                 max_episode_steps: int = 200,
                 control_frequency: float = 10.0):  # 10 Hz for RL (can go up to 500 Hz)
        """
        Initialize the RL environment.

        Args:
            device_sn: Robot device serial number
            debug: Enable debug logging
            max_episode_steps: Maximum steps per episode
            control_frequency: Control frequency in Hz
        """
        self.device_sn = device_sn
        self.debug = debug
        self.max_episode_steps = max_episode_steps
        self.control_dt = 1.0 / control_frequency
        self.current_step = 0

        # Robot interface
        self.robot: Optional[RobotArm] = None

        # Define action and observation spaces
        # Actions: target positions for 9 motors (radians)
        self.action_space = spaces.Box(
            low=-np.pi, high=np.pi,
            shape=(9,), dtype=np.float32
        )

        # Observations: current state (positions, velocities, torques)
        self.observation_space = spaces.Box(
            low=-np.inf, high=np.inf,
            shape=(27,), dtype=np.float32  # 9 positions + 9 velocities + 9 torques
        )

        # Target state for task (example: reach specific position)
        self.target_position = np.array([0.5, -0.3, 0.2, -0.1, 0.0, 0.0, 0.0, 0.0, 0.0])

        # State tracking
        self.current_state: Optional[MotorState] = None
        self.episode_reward = 0.0

    def reset(self) -> np.ndarray:
        """
        Reset the environment for a new episode.

        Returns:
            Initial observation
        """
        if self.debug:
            print("Resetting environment...")

        # Initialize robot if needed
        if self.robot is None:
            self.robot = ic_can.create_robot_arm(self.device_sn, debug=self.debug)

        # Reset step counter
        self.current_step = 0
        self.episode_reward = 0.0

        # Enable robot and set safe mode
        self.robot.enable()
        self.robot.set_control_mode(ControlMode.EXECUTION)

        # Move to initial position (safe zero position)
        initial_position = np.zeros(9)
        self.robot.step(initial_position)
        time.sleep(0.5)  # Wait for movement

        # Read initial state
        self.current_state = self.robot.refresh()

        # Randomize target for variety
        self.target_position = np.random.uniform(-0.5, 0.5, 9)
        self.target_position[6:9] = 0.0  # Keep wrist and gripper neutral

        if self.debug:
            print(f"Target position: {self.target_position}")
            print(f"Initial position: {self.current_state.positions}")

        return self._get_observation()

    def step(self, action: np.ndarray) -> Tuple[np.ndarray, float, bool, Dict[str, Any]]:
        """
        Execute one environment step.

        Args:
            action: Array of target positions for 9 motors

        Returns:
            observation: New state observation
            reward: Reward for this step
            done: Whether episode is finished
            info: Additional information
        """
        if self.robot is None or self.current_state is None:
            raise RuntimeError("Environment not reset. Call reset() first.")

        # Validate action
        action = np.clip(action, self.action_space.low, self.action_space.high)

        # Send action to robot
        success = self.robot.step(action)

        if not success:
            if self.debug:
                print("Failed to send action to robot")

        # Wait for control timestep
        time.sleep(self.control_dt)

        # Read new state
        self.current_state = self.robot.refresh()

        # Calculate reward
        reward = self._calculate_reward(action)

        # Check termination
        done = self._is_done()

        # Update step counter
        self.current_step += 1
        self.episode_reward += reward

        # Additional info
        info = {
            'step': self.current_step,
            'episode_reward': self.episode_reward,
            'success': success,
            'target_distance': np.linalg.norm(self.current_state.positions - self.target_position),
            'safety_status': 'safe' if self.current_state.is_safe() else 'unsafe'
        }

        if self.debug and self.current_step % 50 == 0:
            print(f"Step {self.current_step}: reward={reward:.3f}, "
                  f"target_dist={info['target_distance']:.3f}")

        return self._get_observation(), reward, done, info

    def _get_observation(self) -> np.ndarray:
        """Get current observation as flat vector."""
        if self.current_state is None:
            return np.zeros(27, dtype=np.float32)

        # Combine positions, velocities, and torques
        state_vector = self.current_state.to_vector()
        return state_vector.astype(np.float32)

    def _calculate_reward(self, action: np.ndarray) -> float:
        """
        Calculate reward for the current step.

        This is a simple distance-based reward. In practice, you would
        implement task-specific reward functions here.
        """
        if self.current_state is None:
            return 0.0

        # Distance to target
        position_error = np.linalg.norm(self.current_state.positions - self.target_position)

        # Reward: negative distance (closer is better)
        distance_reward = -position_error

        # Penalty for large actions (encourage smooth movements)
        action_penalty = -0.01 * np.linalg.norm(action - self.current_state.positions)

        # Bonus for reaching target
        target_bonus = 10.0 if position_error < 0.1 else 0.0

        # Safety penalty
        safety_penalty = -5.0 if not self.current_state.is_safe() else 0.0

        total_reward = distance_reward + action_penalty + target_bonus + safety_penalty
        return total_reward

    def _is_done(self) -> bool:
        """Check if episode should terminate."""
        if self.current_state is None:
            return True

        # Terminate if max steps reached
        if self.current_step >= self.max_episode_steps:
            return True

        # Terminate if target reached
        position_error = np.linalg.norm(self.current_state.positions - self.target_position)
        if position_error < 0.05:  # Within 5cm
            return True

        # Terminate if unsafe
        if not self.current_state.is_safe():
            return True

        return False

    def close(self):
        """Clean up resources."""
        if self.robot is not None:
            self.robot.shutdown()
            self.robot = None
            if self.debug:
                print("Environment closed")

    def render(self, mode='human'):
        """Optional rendering for visualization."""
        if mode == 'human' and self.debug:
            if self.current_state:
                print(f"Step {self.current_step}: "
                      f"pos=[{np.mean(self.current_state.positions):.3f}], "
                      f"target_dist={np.linalg.norm(self.current_state.positions - self.target_position):.3f}")


def test_environment():
    """Test the environment with random actions."""
    print("Testing IC_ARM RL Environment...")
    print()

    env = ICArmEnvironment(debug=True, control_frequency=5.0)

    try:
        # Test reset
        print("Testing reset...")
        obs = env.reset()
        print(f"Observation shape: {obs.shape}")
        print(f"Action space: {env.action_space}")
        print(f"Observation space: {env.observation_space}")

        # Test a few steps with random actions
        print("\nTesting random actions...")
        for step in range(10):
            action = env.action_space.sample()
            obs, reward, done, info = env.step(action)
            print(f"Step {step + 1}: reward={reward:.3f}, done={done}, "
                  f"target_dist={info['target_distance']:.3f}")

            if done:
                print("Episode finished early")
                break

        print("\n✓ Environment test completed successfully")

    except Exception as e:
        print(f"Environment test failed: {e}")
        print("This is expected if hardware is not connected.")

    finally:
        env.close()


def rl_training_example():
    """Example of how to integrate with RL training."""
    print("\nRL Training Integration Example")
    print("=" * 40)
    print()

    print("""
# Example: Training with Stable Baselines3

import gym
from stable_baselines3 import PPO
from ic_can_environment import ICArmEnvironment

# Create environment
env = ICArmEnvironment(control_frequency=10.0)

# Wrap for gym compatibility if needed
class GymICArm(gym.Env):
    def __init__(self):
        super().__init__()
        self.env = ICArmEnvironment()
        self.action_space = self.env.action_space
        self.observation_space = self.env.observation_space

    def reset(self):
        return self.env.reset()

    def step(self, action):
        return self.env.step(action)

    def close(self):
        self.env.close()

# Train agent
gym_env = GymICArm()
model = PPO("MlpPolicy", gym_env, verbose=1)

# Training loop
model.learn(total_timesteps=10000)

# Save trained model
model.save("ic_arm_ppo_model")

gym_env.close()
""")


if __name__ == "__main__":
    test_environment()
    rl_training_example()