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
IC_CAN Exception Classes
Custom exceptions for IC_CAN library error handling.
"""

import logging

logger = logging.getLogger(__name__)


class IC_CANError(Exception):
    """
    Base exception class for IC_CAN library errors.

    This is the parent class for all IC_CAN-specific exceptions.
    Catch this exception to handle any IC_CAN-related errors.
    """

    def __init__(self, message: str, error_code: int = None, context: dict = None):
        """
        Initialize IC_CAN error.

        Args:
            message: Error message
            error_code: Optional error code
            context: Optional error context information
        """
        super().__init__(message)
        self.message = message
        self.error_code = error_code
        self.context = context or {}

        # Log the error
        logger.error(f"IC_CAN Error: {message} (code: {error_code}, context: {context})")

    def __str__(self) -> str:
        """String representation of the error."""
        parts = [self.message]
        if self.error_code is not None:
            parts.append(f"Error Code: {self.error_code}")
        if self.context:
            parts.append(f"Context: {self.context}")
        return " | ".join(parts)


class USB2CANError(IC_CANError):
    """
    Exception raised for USB2CAN communication errors.

    This includes device connection issues, communication failures,
    and hardware-related problems.
    """

    def __init__(self, message: str, device_sn: str = None, **kwargs):
        """
        Initialize USB2CAN error.

        Args:
            message: Error message
            device_sn: USB2CAN device serial number
            **kwargs: Additional error context
        """
        context = kwargs.get('context', {})
        if device_sn:
            context['device_sn'] = device_sn
        kwargs['context'] = context

        super().__init__(f"USB2CAN Error: {message}", **kwargs)
        self.device_sn = device_sn


class MotorError(IC_CANError):
    """
    Exception raised for motor control errors.

    This includes motor initialization failures, control errors,
    and motor-specific issues.
    """

    def __init__(self, message: str, motor_id: int = None,
                 motor_type: str = None, **kwargs):
        """
        Initialize motor error.

        Args:
            message: Error message
            motor_id: Motor identifier
            motor_type: Motor type (DM, HT, SERVO)
            **kwargs: Additional error context
        """
        context = kwargs.get('context', {})
        if motor_id is not None:
            context['motor_id'] = motor_id
        if motor_type:
            context['motor_type'] = motor_type
        kwargs['context'] = context

        motor_info = f"Motor {motor_id} ({motor_type})" if motor_id else "Motor"
        super().__init__(f"{motor_info} Error: {message}", **kwargs)
        self.motor_id = motor_id
        self.motor_type = motor_type


class SafetyError(IC_CANError):
    """
    Exception raised for safety system errors.

    This includes emergency stop conditions, safety violations,
    and safety system failures.
    """

    def __init__(self, message: str, violation_type: str = None,
                 is_emergency: bool = False, **kwargs):
        """
        Initialize safety error.

        Args:
            message: Error message
            violation_type: Type of safety violation
            is_emergency: Whether this is an emergency condition
            **kwargs: Additional error context
        """
        context = kwargs.get('context', {})
        if violation_type:
            context['violation_type'] = violation_type
        context['is_emergency'] = is_emergency
        kwargs['context'] = context

        severity = "EMERGENCY" if is_emergency else "SAFETY"
        super().__init__(f"{severity} Error: {message}", **kwargs)
        self.violation_type = violation_type
        self.is_emergency = is_emergency


class CommunicationError(IC_CANError):
    """
    Exception raised for CAN communication errors.

    This includes bus errors, timeout issues, and message failures.
    """

    def __init__(self, message: str, can_id: int = None,
                 timeout_ms: float = None, **kwargs):
        """
        Initialize communication error.

        Args:
            message: Error message
            can_id: CAN identifier
            timeout_ms: Timeout in milliseconds
            **kwargs: Additional error context
        """
        context = kwargs.get('context', {})
        if can_id is not None:
            context['can_id'] = can_id
        if timeout_ms is not None:
            context['timeout_ms'] = timeout_ms
        kwargs['context'] = context

        super().__init__(f"Communication Error: {message}", **kwargs)
        self.can_id = can_id
        self.timeout_ms = timeout_ms


class InitializationError(IC_CANError):
    """
    Exception raised during system initialization.

    This includes configuration errors, dependency issues,
    and startup failures.
    """

    def __init__(self, message: str, component: str = None, **kwargs):
        """
        Initialize initialization error.

        Args:
            message: Error message
            component: Component that failed to initialize
            **kwargs: Additional error context
        """
        context = kwargs.get('context', {})
        if component:
            context['component'] = component
        kwargs['context'] = context

        component_info = f"Component {component}" if component else "System"
        super().__init__(f"{component_info} Initialization Error: {message}", **kwargs)
        self.component = component


class ConfigurationError(IC_CANError):
    """
    Exception raised for configuration errors.

    This includes invalid parameters, configuration file issues,
    and setup problems.
    """

    def __init__(self, message: str, parameter: str = None,
                 expected_value: str = None, **kwargs):
        """
        Initialize configuration error.

        Args:
            message: Error message
            parameter: Parameter name that caused the error
            expected_value: Expected value or format
            **kwargs: Additional error context
        """
        context = kwargs.get('context', {})
        if parameter:
            context['parameter'] = parameter
        if expected_value:
            context['expected_value'] = expected_value
        kwargs['context'] = context

        param_info = f"Parameter '{parameter}'" if parameter else "Configuration"
        super().__init__(f"{param_info} Error: {message}", **kwargs)
        self.parameter = parameter
        self.expected_value = expected_value


class PerformanceError(IC_CANError):
    """
    Exception raised for performance-related issues.

    This includes frequency errors, timing issues,
    and performance degradation.
    """

    def __init__(self, message: str, expected_frequency: float = None,
                 actual_frequency: float = None, **kwargs):
        """
        Initialize performance error.

        Args:
            message: Error message
            expected_frequency: Expected frequency in Hz
            actual_frequency: Actual frequency in Hz
            **kwargs: Additional error context
        """
        context = kwargs.get('context', {})
        if expected_frequency is not None:
            context['expected_frequency'] = expected_frequency
        if actual_frequency is not None:
            context['actual_frequency'] = actual_frequency
        kwargs['context'] = context

        super().__init__(f"Performance Error: {message}", **kwargs)
        self.expected_frequency = expected_frequency
        self.actual_frequency = actual_frequency


# Error handling utilities
def handle_ic_can_error(func):
    """
    Decorator for handling IC_CAN errors consistently.

    This decorator catches exceptions and re-raises them as appropriate
    IC_CANError subclasses with additional context.
    """
    def wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        except IC_CANError:
            # Re-raise IC_CAN errors as-is
            raise
        except Exception as e:
            # Convert other exceptions to IC_CANError
            raise IC_CANError(f"Unexpected error in {func.__name__}: {str(e)}")

    return wrapper


def log_error(error: Exception, context: dict = None):
    """
    Log an error with context information.

    Args:
        error: Exception to log
        context: Additional context information
    """
    context = context or {}

    if isinstance(error, IC_CANError):
        logger.error(f"IC_CAN Error in context {context}: {error}")
    else:
        logger.error(f"Unexpected error in context {context}: {error}")


# Recovery strategies
class ErrorRecoveryStrategy:
    """Base class for error recovery strategies."""

    def can_recover(self, error: Exception) -> bool:
        """Check if this strategy can recover from the given error."""
        return False

    def recover(self, error: Exception) -> bool:
        """
        Attempt to recover from the error.

        Returns:
            True if recovery successful, False otherwise
        """
        return False


class USB2CANRecoveryStrategy(ErrorRecoveryStrategy):
    """Recovery strategy for USB2CAN errors."""

    def can_recover(self, error: Exception) -> bool:
        return isinstance(error, USB2CANError)

    def recover(self, error: Exception) -> bool:
        if not isinstance(error, USB2CANError):
            return False

        logger.info(f"Attempting USB2CAN recovery for: {error}")

        # Implementation would depend on specific recovery actions
        # For example: device reconnection, reset, etc.

        return False  # Placeholder


class SafetyRecoveryStrategy(ErrorRecoveryStrategy):
    """Recovery strategy for safety errors."""

    def can_recover(self, error: Exception) -> bool:
        return isinstance(error, SafetyError)

    def recover(self, error: Exception) -> bool:
        if not isinstance(error, SafetyError):
            return False

        logger.info(f"Attempting safety recovery for: {error}")

        # Safety errors typically require manual intervention
        # Implementation might include: reset emergency stop, clear violations

        return False  # Placeholder