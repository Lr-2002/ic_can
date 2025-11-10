#!/usr/bin/env python3
"""
STM32 Servo USB Communication Test Script

USB-only version for testing STM32 servo control system via USB/UART interfaces.
Based on remote_manip_embedding/Core/Src/main.c - removes CAN dependencies and focuses on:
- Direct servo communication via USB (UART2 equivalent)
- External command interface via USB (UART1 equivalent)
- Position/velocity control with checksum validation
- Real-time servo monitoring and control

Perfect for testing with USB-connected STM32 boards without CAN hardware.
"""

import time
import struct
import sys
import os
import serial
import threading
from typing import Optional, Tuple, List

# Add the project root to path for imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "python"))


class STMServoUSBProtocol:
    """STM32 Servo USB Protocol Implementation - CAN-free version"""

    def __init__(
        self,
        servo_usb_port: str = "/dev/ttyUSB1",
        external_usb_port: str = "/dev/ttyUSB1",
    ):
        """
        Initialize STM32 servo USB protocol

        Args:
            servo_usb_port: USB port for direct servo communication (UART2 equivalent)
            external_usb_port: USB port for external commands (UART1 equivalent)
        """
        self.servo_usb = None
        self.external_usb = None

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

        # USB communication settings
        self.baudrate = 115200
        self.timeout = 0.1

        print("🔧 STM32 Servo USB Protocol Interface")
        print("   USB-only version (no CAN dependencies)")
        print("   Based on remote_manip_embedding/main.c")
        print("=" * 55)

    def list_available_usb_devices(self) -> List[str]:
        """
        List available USB serial devices

        Returns:
            List of available USB device paths
        """
        import glob

        devices = []

        # Common USB serial device patterns
        patterns = [
            "/dev/ttyUSB*",
            "/dev/ttyACM*",
            "/dev/tty.usbserial*",  # macOS
            "/dev/cu.usbserial*",  # macOS
            "COM*",  # Windows
        ]

        for pattern in patterns:
            try:
                devices.extend(glob.glob(pattern))
            except:
                pass

        # Filter and sort
        usb_devices = []
        for device in devices:
            if any(x in device for x in ["USB", "ACM", "usbserial", "cu.usbserial"]):
                usb_devices.append(device)

        return sorted(usb_devices)

    def initialize_usb(
        self, port: str, description: str = "USB Device"
    ) -> Optional[serial.Serial]:
        """
        Initialize USB serial port with auto-detection

        Args:
            port: USB port path
            description: Description for logging

        Returns:
            Serial object or None if failed
        """
        try:
            usb = serial.Serial(
                port=port,
                baudrate=self.baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=self.timeout,
            )
            print(f"✅ {description} ({port}) initialized at {self.baudrate} baud")
            return usb
        except Exception as e:
            print(f"❌ Failed to initialize {description} ({port}): {e}")

            # Suggest available devices
            available = self.list_available_usb_devices()
            if available:
                print(f"   Available USB devices: {', '.join(available)}")
            else:
                print("   No USB serial devices found")

            return None

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

    # ==================== Direct Servo Commands (USB UART2) ====================

    def servo_enable_torque(self) -> bool:
        """
        Enable servo torque - Command: FF FF 01 04 03 28 01 CE

        Returns:
            True if successful, False otherwise
        """
        print("🔌 Enabling servo torque...")
        cmd = [0xFF, 0xFF, 0x01, 0x04, 0x03, 0x28, 0x01, 0xCE]

        if not self.servo_usb:
            print("❌ Servo USB not connected")
            return False

        try:
            self.servo_usb.write(bytes(cmd))
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

        if not self.servo_usb:
            print("❌ Servo USB not connected")
            return False

        try:
            self.servo_usb.write(bytes(cmd))
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

        if not self.servo_usb:
            print("❌ Servo USB not connected")
            return False

        try:
            self.servo_usb.write(bytes(cmd))
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
        cmd = [0xFF, 0xFF, 0x01, 0x04, 0x02, 0x38, 0x02, 0xBE]

        if not self.servo_usb:
            print("❌ Servo USB not connected")
            return None

        try:
            # Clear input buffer
            self.servo_usb.reset_input_buffer()

            # Send read command
            self.servo_usb.write(bytes(cmd))

            # Read response (8 bytes expected)
            response = self.servo_usb.read(8)
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
                if len(response) > 0:
                    print(f"   Raw response: {' '.join(f'{b:02X}' for b in response)}")

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

        # Validate position range
        if not (0 <= position <= 4095):
            print(f"   ❌ Invalid position {position}, must be 0-4095")
            return False

        # Build command (13 bytes)
        cmd = [0xFF, 0xFF, 0x01, 0x09, 0x03, 0x2A]

        # Position (little-endian)
        cmd.append(position & 0xFF)  # pos_low
        cmd.append((position >> 8) & 0xFF)  # pos_high

        # Time (reserved, set to 00)
        cmd.append(0x00)  # time_0
        cmd.append(0x00)  # time_1

        # Velocity (little-endian)
        cmd.append(velocity & 0xFF)  # vel_low
        cmd.append((velocity >> 8) & 0xFF)  # vel_high

        # Calculate checksum
        checksum = self.calculate_checksum(cmd[2:])  # Exclude FF FF header
        cmd.append(checksum)

        if not self.servo_usb:
            print("❌ Servo USB not connected")
            return False

        try:
            self.servo_usb.write(bytes(cmd))
            print(f"   Sent: {' '.join(f'{b:02X}' for b in cmd)}")

            # Store set values
            self.servo_set_position[0] = position
            self.servo_set_speed[0] = velocity

            return True
        except Exception as e:
            print(f"❌ Failed to send position control: {e}")
            return False

    # ==================== External Command Interface (USB UART1) ====================

    def send_external_command(self, command: int, data: List[int] = None) -> bool:
        """
        Send external command via USB (10-byte format from main.c)

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

        if not self.external_usb:
            print("❌ External USB not connected")
            return False

        try:
            self.external_usb.write(bytes(packet))
            cmd_name = {
                0x00: "Disable",
                0x01: "Enable",
                0x02: "Write Position",
                0x03: "Read Position",
                0x04: "Set Mid",
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

    # ==================== USB Monitoring ====================

    def monitor_servo_usb(self, duration: float = 5.0) -> None:
        """
        Monitor servo USB communication for debugging

        Args:
            duration: Monitoring duration in seconds
        """
        if not self.servo_usb:
            print("❌ Servo USB not connected")
            return

        print(f"\n👂 Monitoring servo USB for {duration}s...")
        print("   Press Ctrl+C to stop early")

        start_time = time.time()
        try:
            while time.time() - start_time < duration:
                if self.servo_usb.in_waiting > 0:
                    data = self.servo_usb.read(self.servo_usb.in_waiting)
                    timestamp = time.strftime("%H:%M:%S.%f")[:-3]
                    print(f"   [{timestamp}] RX: {' '.join(f'{b:02X}' for b in data)}")
                time.sleep(0.01)  # 10ms polling
        except KeyboardInterrupt:
            print(f"\n   Monitoring stopped after {time.time() - start_time:.1f}s")

        print("✅ Monitoring completed")

    def monitor_external_usb(self, duration: float = 5.0) -> None:
        """
        Monitor external USB communication for debugging

        Args:
            duration: Monitoring duration in seconds
        """
        if not self.external_usb:
            print("❌ External USB not connected")
            return

        print(f"\n👂 Monitoring external USB for {duration}s...")
        print("   Press Ctrl+C to stop early")

        start_time = time.time()
        try:
            while time.time() - start_time < duration:
                if self.external_usb.in_waiting > 0:
                    data = self.external_usb.read(self.external_usb.in_waiting)
                    timestamp = time.strftime("%H:%M:%S.%f")[:-3]
                    print(f"   [{timestamp}] RX: {' '.join(f'{b:02X}' for b in data)}")
                time.sleep(0.01)  # 10ms polling
        except KeyboardInterrupt:
            print(f"\n   Monitoring stopped after {time.time() - start_time:.1f}s")

        print("✅ Monitoring completed")

    # ==================== Test Functions ====================

    def test_basic_servo_commands(self) -> bool:
        """Test basic servo communication via direct USB"""
        print("\n🚀 Testing Basic Servo Commands (Direct USB)")
        print("-" * 50)

        success = True

        # Test enable/disable
        if not self.servo_enable_torque():
            success = False
        time.sleep(0.2)

        if not self.servo_read_position():
            success = False
        time.sleep(0.2)

        if not self.servo_disable_torque():
            success = False

        print("✅ Basic servo commands completed")
        return success

    def test_position_control(self, positions: List[int] = None) -> bool:
        """Test position control via direct servo commands"""
        if positions is None:
            positions = [1000, 2048, 3000]  # Include middle position

        print(f"\n🎯 Testing Position Control")
        print(f"   Positions: {positions}")
        print("-" * 40)

        success = True

        # Enable torque first
        if not self.servo_enable_torque():
            success = False

        time.sleep(0.2)

        for i, pos in enumerate(positions):
            print(f"\n📍 Move {i+1}/{len(positions)}: Position {pos}")

            if not self.servo_position_control(pos, 200):
                success = False

            time.sleep(0.5)  # Wait for movement

            # Read current position
            current_pos = self.servo_read_position()
            if current_pos is not None:
                print(f"   Current position: {current_pos}")

            time.sleep(0.3)

        # Disable at the end
        self.servo_disable_torque()

        print("\n✅ Position control test completed")
        return success

    def test_external_commands(self) -> bool:
        """Test external command interface (USB)"""
        print("\n🌐 Testing External Command Interface (USB)")
        print("-" * 50)

        success = True

        # Test enable
        if not self.send_enable_command():
            success = False
        time.sleep(0.2)

        # Test position command
        if not self.send_position_command(2048, 150):
            success = False
        time.sleep(0.2)

        # Test read command
        if not self.send_read_command():
            success = False
        time.sleep(0.2)

        # Test set mid command
        if not self.send_set_mid_command():
            success = False
        time.sleep(0.2)

        # Test disable
        if not self.send_disable_command():
            success = False

        print("✅ External commands test completed")
        return success

    def test_bidirectional_communication(self) -> bool:
        """Test bidirectional communication between USB interfaces"""
        print("\n🔄 Testing Bidirectional Communication")
        print("-" * 45)

        success = True

        if not self.servo_usb or not self.external_usb:
            print("❌ Both USB interfaces required for bidirectional test")
            return False

        # Send enable command via external interface
        print("📤 Sending enable via external USB...")
        if not self.send_enable_command():
            success = False
        time.sleep(0.3)

        # Monitor servo USB for responses
        print("👂 Monitoring servo USB for responses...")
        self.monitor_servo_usb(duration=2.0)

        # Send position command via external interface
        print("📤 Sending position command via external USB...")
        if not self.send_position_command(1500, 180):
            success = False
        time.sleep(0.3)

        # Monitor servo USB for responses
        print("👂 Monitoring servo USB for responses...")
        self.monitor_servo_usb(duration=2.0)

        # Send disable command via external interface
        print("📤 Sending disable via external USB...")
        if not self.send_disable_command():
            success = False

        print("✅ Bidirectional communication test completed")
        return success

    def run_comprehensive_test(self) -> bool:
        """Run comprehensive test of all USB functionality"""
        print("\n🧪 Running Comprehensive STM32 Servo USB Test")
        print("=" * 60)

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

        # Test 4: Bidirectional communication (if both interfaces available)
        if self.servo_usb and self.external_usb:
            if not self.test_bidirectional_communication():
                success = False

        print(
            f"\n🎉 Comprehensive USB test completed: {'✅ PASSED' if success else '❌ FAILED'}"
        )
        return success

    def interactive_mode(self) -> None:
        """Interactive mode for manual servo control"""
        print("\n🎮 Interactive Servo Control Mode")
        print("=" * 40)
        print("Commands:")
        print("  enable    - Enable servo torque")
        print("  disable   - Disable servo torque")
        print("  mid       - Set middle position")
        print("  read      - Read current position")
        print("  pos <0-4095> [vel] - Move to position")
        print("  monitor <seconds> - Monitor servo USB")
        print("  quit      - Exit interactive mode")
        print("-" * 40)

        try:
            while True:
                try:
                    cmd_input = input("🎮 servo> ").strip().lower()

                    if not cmd_input:
                        continue

                    parts = cmd_input.split()
                    cmd = parts[0]

                    if cmd == "quit" or cmd == "q":
                        break
                    elif cmd == "enable" or cmd == "en":
                        self.servo_enable_torque()
                    elif cmd == "disable" or cmd == "dis":
                        self.servo_disable_torque()
                    elif cmd == "mid" or cmd == "m":
                        self.servo_set_mid()
                    elif cmd == "read" or cmd == "r":
                        self.servo_read_position()
                    elif cmd == "pos" or cmd == "p":
                        if len(parts) >= 2:
                            try:
                                position = int(parts[1])
                                velocity = int(parts[2]) if len(parts) > 2 else 150
                                self.servo_position_control(position, velocity)
                            except ValueError:
                                print("   ❌ Invalid position value")
                        else:
                            print("   Usage: pos <position> [velocity]")
                    elif cmd == "monitor" or cmd == "mon":
                        duration = float(parts[1]) if len(parts) > 1 else 5.0
                        self.monitor_servo_usb(duration)
                    else:
                        print(f"   ❌ Unknown command: {cmd}")
                        print("   Type 'quit' to exit")

                except EOFError:
                    break
                except KeyboardInterrupt:
                    print("\n   Exiting interactive mode...")
                    break

        finally:
            # Ensure servo is disabled
            if self.servo_enable:
                self.servo_disable_torque()

        print("✅ Interactive mode completed")

    def cleanup(self):
        """Clean up USB resources"""
        print("\n🧹 Cleaning up...")

        # Disable servo
        if self.servo_usb and self.servo_enable:
            self.servo_disable_torque()

        # Close USB connections
        if self.servo_usb:
            self.servo_usb.close()
            print("   Servo USB closed")

        if self.external_usb:
            self.external_usb.close()
            print("   External USB closed")

        print("✅ Cleanup completed")


def main():
    """Main test function"""
    print("🤖 STM32 Servo USB Communication Test Script")
    print("USB-only version - no CAN dependencies required")
    print("Based on remote_manip_embedding/main.c protocol")
    print("=" * 70)

    # Parse command line arguments
    import argparse

    parser = argparse.ArgumentParser(description="STM32 Servo USB Protocol Tester")
    parser.add_argument(
        "--servo-usb", default="/dev/ttyUSB0", help="Servo USB port (UART2 equivalent)"
    )
    parser.add_argument(
        "--external-usb",
        default="/dev/ttyUSB1",
        help="External command USB port (UART1 equivalent)",
    )
    parser.add_argument(
        "--test",
        choices=[
            "basic",
            "position",
            "external",
            "bidirectional",
            "all",
            "interactive",
        ],
        default="all",
        help="Test to run (default: all)",
    )
    parser.add_argument(
        "--positions", nargs="+", type=int, help="Custom positions for position test"
    )
    parser.add_argument(
        "--baudrate", type=int, default=115200, help="USB baudrate (default: 115200)"
    )
    parser.add_argument(
        "--list-ports", action="store_true", help="List available USB ports and exit"
    )
    parser.add_argument(
        "--monitor", type=float, help="Monitor servo USB for specified seconds"
    )

    args = parser.parse_args()

    # Create protocol interface
    protocol = STMServoUSBProtocol(args.servo_usb, args.external_usb)
    protocol.baudrate = args.baudrate

    # List available ports if requested
    if args.list_ports:
        available = protocol.list_available_usb_devices()
        print("Available USB serial devices:")
        if available:
            for device in available:
                print(f"   {device}")
        else:
            print("   No USB serial devices found")
        return 0

    # Initialize USB interfaces
    servo_connected = False
    external_connected = False

    if args.servo_usb:
        protocol.servo_usb = protocol.initialize_usb(args.servo_usb, "Servo USB")
        servo_connected = protocol.servo_usb is not None

    if args.external_usb:
        protocol.external_usb = protocol.initialize_usb(
            args.external_usb, "External USB"
        )
        external_connected = protocol.external_usb is not None

    # Monitor mode if requested
    if args.monitor:
        if not servo_connected:
            print("❌ Servo USB required for monitoring")
            return 1
        protocol.monitor_servo_usb(args.monitor)
        return 0

    # Check if at least one interface is connected
    if not servo_connected and not external_connected:
        print("❌ No USB interfaces connected")
        print("   Use --list-ports to see available devices")
        print("   Use --simulation flag for testing without hardware")
        return 1

    try:
        # Run requested tests
        if args.test == "interactive":
            if not servo_connected:
                print("❌ Servo USB required for interactive mode")
                return 1
            protocol.interactive_mode()

        elif args.test in ["basic", "all"]:
            if servo_connected:
                protocol.test_basic_servo_commands()
            else:
                print("⚠️  Skipping basic test - servo USB not connected")

        if args.test in ["position", "all"]:
            if servo_connected:
                positions = args.positions if args.positions else None
                protocol.test_position_control(positions)
            else:
                print("⚠️  Skipping position test - servo USB not connected")

        if args.test in ["external", "all"]:
            if external_connected:
                protocol.test_external_commands()
            else:
                print("⚠️  Skipping external test - external USB not connected")

        if args.test in ["bidirectional", "all"]:
            if servo_connected and external_connected:
                protocol.test_bidirectional_communication()
            else:
                print("⚠️  Skipping bidirectional test - both USB interfaces required")

        if args.test == "all":
            print(f"\n🎉 All USB tests completed successfully!")
        else:
            print(f"\n🎉 {args.test} test completed successfully!")

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

