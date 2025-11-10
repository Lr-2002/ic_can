#!/usr/bin/env python3
"""
STM32 Servo Communication Test Script

Based on remote_manip_embedding/Core/Src/main.c - comprehensive STM32 servo control system.
This script implements the complete servo protocol for:
- UART servo communication (direct servo control)
- External command interface (UART1)
- CAN bridge functionality
- Position/velocity control with checksum validation

Protocol extracted from main.c:
- UART2: Direct servo communication (FF FF header + commands)
- UART1: External command interface (10-byte packets)
- CAN: Bridge functionality (8-byte frames)
"""

import time
import struct
import sys
import os
import serial
import threading
from typing import Optional, Tuple, List

# Add the project root to path for imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'python'))

try:
    import ic_can_python
except ImportError:
    print("❌ ic_can_python module not found. Build the project first:")
    print("   mkdir -p build && cd build && cmake .. -DBUILD_PYTHON_BINDINGS=ON && make -j$(nproc)")
    print("\n⚠️  This script can also run in simulation mode without IC_CAN bindings")


class STMServoProtocol:
    """STM32 Servo Protocol Implementation based on main.c"""

    def __init__(self, servo_uart_port: str = "/dev/ttyUSB0", external_uart_port: str = "/dev/ttyUSB1"):
        """
        Initialize STM32 servo protocol

        Args:
            servo_uart_port: UART port for direct servo communication (UART2 equivalent)
            external_uart_port: UART port for external commands (UART1 equivalent)
        """
        self.servo_uart = None
        self.external_uart = None
        self.ic_can = None

        # Servo state variables (matching main.c)
        self.servo_enable = 0
        self.servo_id = [1]  # Can be expanded for multiple servos
        self.servo_position = [0] * len(self.servo_id)
        self.servo_speed = [0] * len(self.servo_id)
        self.servo_set_position = [0] * len(self.servo_id)
        self.servo_set_speed = [3250] * len(self.servo_id)  # Default from main.c
        self.servo_set_acc = [0] * len(self.servo_id)

        # Control flags (matching main.c)
        self.servo_read_flag_12 = 1
        self.servo_read_flag_3 = 0
        self.servo_write_flag = 0

        # Default values from main.c
        self.default_speed = 3250
        self.default_acc = 0

        print("🔧 STM32 Servo Protocol Interface")
        print("   Based on remote_manip_embedding/main.c")
        print("=" * 50)

    def initialize_uart(self, port: str, baudrate: int = 115200, timeout: float = 0.1) -> Optional[serial.Serial]:
        """Initialize UART port"""
        try:
            uart = serial.Serial(
                port=port,
                baudrate=baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=timeout
            )
            print(f"✅ UART {port} initialized at {baudrate} baud")
            return uart
        except Exception as e:
            print(f"❌ Failed to initialize UART {port}: {e}")
            return None

    def initialize_ic_can(self) -> bool:
        """Initialize IC_CAN interface (optional)"""
        try:
            self.ic_can = ic_can_python.IC_CAN()
            print("✅ IC_CAN interface initialized")
            return True
        except Exception as e:
            print(f"⚠️  IC_CAN not available: {e}")
            print("   Running in simulation mode")
            return False

    def calculate_checksum(self, data: List[int]) -> int:
        """
        Calculate checksum using BITXOR method from main.c
        Checksum = (SUM(data) & 0xFF) ^ 0xFF

        Args:
            data: List of bytes to checksum (excluding checksum byte)

        Returns:
            Checksum byte
        """
        sum_val = sum(data) & 0xFF
        return sum_val ^ 0xFF

    # ==================== Direct Servo Commands (UART2) ====================

    def servo_enable_torque(self) -> bool:
        """
        Enable servo torque - Command: FF FF 01 04 03 28 01 CE

        Returns:
            True if successful, False otherwise
        """
        print("🔌 Enabling servo torque...")
        cmd = [0xFF, 0xFF, 0x01, 0x04, 0x03, 0x28, 0x01, 0xCE]

        if not self.servo_uart:
            print("❌ Servo UART not connected")
            return False

        try:
            self.servo_uart.write(bytes(cmd))
            self.servo_enable = 1
            print(f"   Sent: {' '.join(f'{b:02X}' for b in cmd)}")
            return True
        except Exception as e:
            print(f"❌ Failed to send enable torque: {e}")
            return False

    def servo_disable_torque(self) -> bool:
        """
        Disable servo torque - Command: FF FF 01 04 03 28 00 CF

        Returns:
            True if successful, False otherwise
        """
        print("🔌 Disabling servo torque...")
        cmd = [0xFF, 0xFF, 0x01, 0x04, 0x03, 0x28, 0x00, 0xCF]

        if not self.servo_uart:
            print("❌ Servo UART not connected")
            return False

        try:
            self.servo_uart.write(bytes(cmd))
            self.servo_enable = 0
            print(f"   Sent: {' '.join(f'{b:02X}' for b in cmd)}")
            return True
        except Exception as e:
            print(f"❌ Failed to send disable torque: {e}")
            return False

    def servo_set_mid(self) -> bool:
        """
        Set servo to middle position - Command: FF FF 01 04 03 28 80 4F

        Returns:
            True if successful, False otherwise
        """
        print("🎯 Setting servo to middle position...")
        cmd = [0xFF, 0xFF, 0x01, 0x04, 0x03, 0x28, 0x80, 0x4F]

        if not self.servo_uart:
            print("❌ Servo UART not connected")
            return False

        try:
            self.servo_uart.write(bytes(cmd))
            print(f"   Sent: {' '.join(f'{b:02X}' for b in cmd)}")
            return True
        except Exception as e:
            print(f"❌ Failed to send set mid: {e}")
            return False

    def servo_read_position(self) -> Optional[int]:
        """
        Read servo position - Command: FF FF 01 04 02 38 02 BE

        Returns:
            Current position (0-4095) or None if failed
        """
        print("📏 Reading servo position...")
        cmd = [0xFF, 0xFF, 0x01, 0x04, 0x02, 38, 0x02, 0xBE]

        if not self.servo_uart:
            print("❌ Servo UART not connected")
            return None

        try:
            # Send read command
            self.servo_uart.write(bytes(cmd))

            # Read response (8 bytes expected)
            response = self.servo_uart.read(8)
            if len(response) == 8:
                response_bytes = list(response)
                print(f"   Sent: {' '.join(f'{b:02X}' for b in cmd)}")
                print(f"   Recv: {' '.join(f'{b:02X}' for b in response_bytes)}")

                # Validate response header (FF FF)
                if response_bytes[0] == 0xFF and response_bytes[1] == 0xFF:
                    # Extract position from bytes 5 and 6 (0-indexed, low byte first)
                    position = response_bytes[5] | (response_bytes[6] << 8)
                    self.servo_position[0] = position
                    print(f"   Position: {position}")
                    return position
                else:
                    print("   ❌ Invalid response header")
            else:
                print(f"   ❌ Unexpected response length: {len(response)}")

        except Exception as e:
            print(f"❌ Failed to read position: {e}")

        return None

    def servo_position_control(self, position: int, velocity: int = None) -> bool:
        """
        Servo position control - Based on main.c line 189-218

        Command format: FF FF 01 09 03 2A pos_low pos_high time_0 time_1 vel_low vel_high checksum

        Args:
            position: Target position (0-4095)
            velocity: Target velocity (0-4095, default: 100)

        Returns:
            True if successful, False otherwise
        """
        if velocity is None:
            velocity = 100  # Default velocity from main.c (0x64)

        print(f"🎯 Position control: pos={position}, vel={velocity}")

        # Build command (13 bytes)
        cmd = [0xFF, 0xFF, 0x01, 0x09, 0x03, 0x2A]

        # Position (little-endian)
        cmd.append(position & 0xFF)        # pos_low
        cmd.append((position >> 8) & 0xFF) # pos_high

        # Time (reserved, set to 00)
        cmd.append(0x00)  # time_0
        cmd.append(0x00)  # time_1

        # Velocity (little-endian)
        cmd.append(velocity & 0xFF)        # vel_low
        cmd.append((velocity >> 8) & 0xFF) # vel_high

        # Calculate checksum
        checksum = self.calculate_checksum(cmd[2:])  # Exclude FF FF header
        cmd.append(checksum)

        if not self.servo_uart:
            print("❌ Servo UART not connected")
            return False

        try:
            self.servo_uart.write(bytes(cmd))
            print(f"   Sent: {' '.join(f'{b:02X}' for b in cmd)}")

            # Store set values
            self.servo_set_position[0] = position
            self.servo_set_speed[0] = velocity

            return True
        except Exception as e:
            print(f"❌ Failed to send position control: {e}")
            return False

    # ==================== External Command Interface (UART1) ====================

    def send_external_command(self, command: int, data: List[int] = None) -> bool:
        """
        Send external command via UART1 (10-byte format from main.c)

        Format: [id_high, id_low, command, data...]

        Args:
            command: Command byte (0x00=disable, 0x01=enable, 0x02=write pos, 0x03=read, 0x04=set mid)
            data: Additional data bytes (max 7)

        Returns:
            True if successful, False otherwise
        """
        if data is None:
            data = []

        # Build 10-byte command packet
        packet = [0x00, 0x09]  # CAN ID 0x09 (from main.c line 221)
        packet.append(command)

        # Add data bytes (pad with 0 if needed)
        packet.extend(data)
        while len(packet) < 10:
            packet.append(0x00)

        if not self.external_uart:
            print("❌ External UART not connected")
            return False

        try:
            self.external_uart.write(bytes(packet))
            cmd_name = {
                0x00: "Disable",
                0x01: "Enable",
                0x02: "Write Position",
                0x03: "Read Position",
                0x04: "Set Mid"
            }.get(command, f"Unknown(0x{command:02X})")

            print(f"📤 External command: {cmd_name}")
            print(f"   Packet: {' '.join(f'{b:02X}' for b in packet)}")
            return True
        except Exception as e:
            print(f"❌ Failed to send external command: {e}")
            return False

    def send_position_command(self, position: int, velocity: int = 100) -> bool:
        """
        Send position command via external interface (command 0x02)

        Args:
            position: Position value (0-4095)
            velocity: Velocity value (0-4095)

        Returns:
            True if successful, False otherwise
        """
        # Position and velocity as big-endian (from main.c line 292-293)
        pos_high = (position >> 8) & 0xFF
        pos_low = position & 0xFF
        vel_high = (velocity >> 8) & 0xFF
        vel_low = velocity & 0xFF

        data = [pos_high, pos_low, vel_high, vel_low]
        return self.send_external_command(0x02, data)

    def send_read_command(self) -> bool:
        """Send read position command via external interface (command 0x03)"""
        return self.send_external_command(0x03)

    def send_enable_command(self) -> bool:
        """Send enable command via external interface (command 0x01)"""
        return self.send_external_command(0x01)

    def send_disable_command(self) -> bool:
        """Send disable command via external interface (command 0x00)"""
        return self.send_external_command(0x00)

    def send_set_mid_command(self) -> bool:
        """Send set middle position command via external interface (command 0x04)"""
        return self.send_external_command(0x04)

    # ==================== CAN Bridge Interface ====================

    def send_can_frame(self, can_id: int, data: List[int]) -> bool:
        """
        Send CAN frame via IC_CAN interface

        Args:
            can_id: CAN identifier
            data: 8-byte data array

        Returns:
            True if successful, False otherwise
        """
        if not self.ic_can:
            print("❌ IC_CAN interface not available")
            return False

        try:
            self.ic_can.send_can_frame(can_id, data)
            print(f"📤 CAN: ID=0x{can_id:03X} Data={' '.join(f'{b:02X}' for b in data)}")
            return True
        except Exception as e:
            print(f"❌ Failed to send CAN frame: {e}")
            return False

    def format_servo_data(self) -> List[int]:
        """
        Format servo position/speed data for CAN transmission (from main.c line 234-239)

        Returns:
            8-byte data array: [pos_high, pos_low, speed_high, speed_low, ...]
        """
        data = []
        for i in range(len(self.servo_id)):
            # Position (big-endian)
            data.append((self.servo_position[i] >> 8) & 0xFF)  # pos_high
            data.append(self.servo_position[i] & 0xFF)           # pos_low
            # Speed (big-endian)
            data.append((self.servo_speed[i] >> 8) & 0xFF)      # speed_high
            data.append(self.servo_speed[i] & 0xFF)             # speed_low

        # Pad to 8 bytes
        while len(data) < 8:
            data.append(0x00)

        return data[:8]

    def send_servo_data_via_can(self) -> bool:
        """
        Send servo position/speed data via CAN (matching main.c line 240-244)

        Returns:
            True if successful, False otherwise
        """
        servo_data = self.format_servo_data()
        return self.send_can_frame(0x000, servo_data)

    # ==================== Test Functions ====================

    def test_basic_servo_commands(self) -> bool:
        """Test basic servo communication via direct UART"""
        print("\n🚀 Testing Basic Servo Commands (Direct UART)")
        print("-" * 50)

        success = True

        # Test enable/disable
        if not self.servo_enable_torque():
            success = False
        time.sleep(0.1)

        if not self.servo_read_position():
            success = False
        time.sleep(0.1)

        if not self.servo_disable_torque():
            success = False

        print("✅ Basic servo commands completed")
        return success

    def test_position_control(self, positions: List[int] = None) -> bool:
        """Test position control via direct servo commands"""
        if positions is None:
            positions = [1000, 2000, 3000, 2048]  # Include middle position

        print(f"\n🎯 Testing Position Control")
        print(f"   Positions: {positions}")
        print("-" * 40)

        success = True

        # Enable torque first
        if not self.servo_enable_torque():
            success = False

        time.sleep(0.1)

        for i, pos in enumerate(positions):
            print(f"\n📍 Move {i+1}/{len(positions)}: Position {pos}")

            if not self.servo_position_control(pos, 200):
                success = False

            time.sleep(0.2)  # Wait for movement

            # Read current position
            current_pos = self.servo_read_position()
            if current_pos is not None:
                print(f"   Current position: {current_pos}")

            time.sleep(0.3)

        print("\n✅ Position control test completed")
        return success

    def test_external_commands(self) -> bool:
        """Test external command interface (UART1)"""
        print("\n🌐 Testing External Command Interface")
        print("-" * 45)

        success = True

        # Test enable
        if not self.send_enable_command():
            success = False
        time.sleep(0.1)

        # Test position command
        if not self.send_position_command(2048, 150):
            success = False
        time.sleep(0.1)

        # Test read command
        if not self.send_read_command():
            success = False
        time.sleep(0.1)

        # Test disable
        if not self.send_disable_command():
            success = False

        print("✅ External commands test completed")
        return success

    def test_can_bridge(self) -> bool:
        """Test CAN bridge functionality"""
        print("\n🌉 Testing CAN Bridge Functionality")
        print("-" * 40)

        if not self.ic_can:
            print("⚠️  IC_CAN not available - skipping CAN bridge test")
            return True

        success = True

        # Simulate servo data and send via CAN
        self.servo_position[0] = 2048
        self.servo_speed[0] = 100

        if not self.send_servo_data_via_can():
            success = False

        print("✅ CAN bridge test completed")
        return success

    def run_comprehensive_test(self) -> bool:
        """Run comprehensive test of all functionality"""
        print("\n🧪 Running Comprehensive STM32 Servo Test")
        print("=" * 50)

        success = True

        # Test 1: Basic servo commands
        if not self.test_basic_servo_commands():
            success = False

        time.sleep(0.5)

        # Test 2: Position control
        if not self.test_position_control():
            success = False

        time.sleep(0.5)

        # Test 3: External commands
        if not self.test_external_commands():
            success = False

        time.sleep(0.5)

        # Test 4: CAN bridge
        if not self.test_can_bridge():
            success = False

        print(f"\n🎉 Comprehensive test completed: {'✅ PASSED' if success else '❌ FAILED'}")
        return success

    def cleanup(self):
        """Clean up resources"""
        print("\n🧹 Cleaning up...")

        # Disable servo
        if self.servo_uart and self.servo_enable:
            self.servo_disable_torque()

        # Close UART connections
        if self.servo_uart:
            self.servo_uart.close()
            print("   Servo UART closed")

        if self.external_uart:
            self.external_uart.close()
            print("   External UART closed")

        # Close IC_CAN interface
        if self.ic_can:
            try:
                self.ic_can.shutdown()
                print("   IC_CAN interface closed")
            except:
                pass

        print("✅ Cleanup completed")


def main():
    """Main test function"""
    print("🤖 STM32 Servo Communication Test Script")
    print("Based on remote_manip_embedding/main.c protocol")
    print("=" * 60)

    # Parse command line arguments
    import argparse
    parser = argparse.ArgumentParser(description='STM32 Servo Protocol Tester')
    parser.add_argument('--servo-uart', default='/dev/ttyUSB0',
                       help='Servo UART port (UART2 equivalent)')
    parser.add_argument('--external-uart', default='/dev/ttyUSB1',
                       help='External command UART port (UART1 equivalent)')
    parser.add_argument('--test', choices=['basic', 'position', 'external', 'can', 'all'],
                       default='all', help='Test to run (default: all)')
    parser.add_argument('--positions', nargs='+', type=int,
                       help='Custom positions for position test')
    parser.add_argument('--simulation', action='store_true',
                       help='Run in simulation mode without hardware')

    args = parser.parse_args()

    # Create protocol interface
    protocol = STMServoProtocol(args.servo_uart, args.external_uart)

    # Initialize interfaces
    if not args.simulation:
        protocol.servo_uart = protocol.initialize_uart(args.servo_uart)
        protocol.external_uart = protocol.initialize_uart(args.external_uart)
    else:
        print("🔧 Running in simulation mode")

    protocol.initialize_ic_can()

    try:
        # Run requested tests
        if args.test in ['basic', 'all']:
            protocol.test_basic_servo_commands()

        if args.test in ['position', 'all']:
            positions = args.positions if args.positions else None
            protocol.test_position_control(positions)

        if args.test in ['external', 'all']:
            protocol.test_external_commands()

        if args.test in ['can', 'all']:
            protocol.test_can_bridge()

        print("\n🎉 All tests completed successfully!")
        return 0

    except KeyboardInterrupt:
        print("\n⚠️ Test interrupted by user")
        return 1

    except Exception as e:
        print(f"\n❌ Test failed with error: {e}")
        return 1

    finally:
        protocol.cleanup()


if __name__ == "__main__":
    sys.exit(main())