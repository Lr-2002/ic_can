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
IC_CAN Python Package
High Performance CAN Control Library for Robotic Arms

This package provides Python bindings for the IC_CAN C++ library,
enabling high-performance control of robotic arms with CAN bus communication.
"""

__version__ = "1.0.0"
__author__ = "IC_CAN Team"
__email__ = "openarm@enactic.ai"
__license__ = "Apache License 2.0"

# Import core components
from .core import IC_CANController
from .exceptions import IC_CANError, USB2CANError, MotorError, SafetyError

# Import high-level RL/DNN interface
from .motor_state import MotorState, MotorCommand, ControlMode
from .robot_arm import RobotArm, create_robot_arm

# Import low-level bindings for advanced users
try:
    from .ic_can_python import IC_CAN
    _LOW_LEVEL_AVAILABLE = True
except ImportError:
    _LOW_LEVEL_AVAILABLE = False

# Define what gets imported with 'from ic_can import *'
__all__ = [
    # High-level interface (recommended for RL/DNN)
    'RobotArm',
    'create_robot_arm',
    'MotorState',
    'MotorCommand',
    'ControlMode',

    # Existing interface
    'IC_CANController',

    # Exceptions
    'IC_CANError',
    'USB2CANError',
    'MotorError',
    'SafetyError',

    # Low-level interface (advanced users)
    'IC_CAN',

    # Metadata
    '__version__',
    '__author__',
    '__email__',
    '__license__'
]

# Convenience shortcuts for high-level interface
def quick_start(device_sn: str = "F561E08C892274DB09496BCC1102DBC5"):
    """
    Quick start function for immediate robot control.

    Example:
        >>> import ic_can
        >>> robot = ic_can.quick_start()
        >>> robot.enable()
        >>> state = robot.refresh()
        >>> robot.step([0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.0, 0.0, 0.0])
        >>> robot.shutdown()
    """
    return create_robot_arm(device_sn=device_sn, debug=False)