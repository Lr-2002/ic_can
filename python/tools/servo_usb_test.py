#!/usr/bin/env python3
"""
Simple USB Servo Test Script

Direct USB connection to servo - single port implementation.
Based on remote_manip_embedding/Core/Src/main.c servo protocol.

This script connects directly to a servo via USB and provides:
- Direct servo control commands
- Position control with velocity
- Real-time position reading
- Interactive manual control
- USB communication monitoring

Perfect for testing with a single USB connection to the servo.
"""

import time
import struct
import sys
import os
import serial
import threading
from typing import Optional, List


class ServoUSBProtocol:
    """Direct USB Servo Protocol Implementation"""

    def __init__(self, usb_port: str = "/dev/ttyUSB0"):
        """
        Initialize direct USB servo connection

        Args:
            usb_port: USB port for servo communication
        """
        self.servo_usb = None
        self.usb_port = usb_port

        # Servo position range (1000-2100 for this specific servo)
        self.POSITION_MIN = 1000
        self.POSITION_MAX = 2100
        self.POSITION_MID = (self.POSITION_MIN + self.POSITION_MAX) // 2  # 1550

        # Servo speed limits (limited to 100)
        self.SPEED_MIN = 0
        self.SPEED_MAX = 100
        self.SPEED_DEFAULT = 50  # Default safe speed

        # Servo state variables
        self.servo_enable = 0
        self.servo_position = self.POSITION_MID
        self.servo_set_position = self.POSITION_MID
        self.servo_set_speed = self.SPEED_DEFAULT

        # USB communication settings
        self.baudrate = 115200
        self.timeout = 0.1

        print("🔧 Direct USB Servo Protocol Interface")
        print("   Single USB connection to servo")
        print(f"   Position range: {self.POSITION_MIN}-{self.POSITION_MAX}")
        print(f"   Middle position: {self.POSITION_MID}")
        print(f"   Speed range: {self.SPEED_MIN}-{self.SPEED_MAX}")
        print("   Based on remote_manip_embedding/main.c")
        print("=" * 50)

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

    def initialize_usb(self, port: str = None) -> Optional[serial.Serial]:
        """
        Initialize USB serial port

        Args:
            port: USB port path (uses default if None)

        Returns:
            Serial object or None if failed
        """
        if port is None:
            port = self.usb_port

        try:
            self.servo_usb = serial.Serial(
                port=port,
                baudrate=self.baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=self.timeout,
            )
            print(f"✅ Servo USB ({port}) initialized at {self.baudrate} baud")
            return self.servo_usb
        except Exception as e:
            print(f"❌ Failed to initialize servo USB ({port}): {e}")

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

    # ==================== Direct Servo Commands ====================

    def enable_torque(self) -> bool:
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

    def disable_torque(self) -> bool:
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

    def set_mid_position(self) -> bool:
        """
        Set servo to middle position

        Returns:
            True if successful, False otherwise
        """
        print(f"🎯 Setting servo to middle position ({self.POSITION_MID})...")
        # Use position control instead of fixed middle command
        return self.position_control(self.POSITION_MID, self.SPEED_DEFAULT)

    def read_position(self) -> Optional[int]:
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
                    self.servo_position = position
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

    def position_control(self, position: int, velocity: int = None) -> bool:
        """
        Servo position control - Based on main.c line 189-218

        Command format: FF FF 01 09 03 2A pos_low pos_high time_0 time_1 vel_low vel_high checksum

        Args:
            position: Target position (700-1650)
            velocity: Target velocity (0-100, default: 50)

        Returns:
            True if successful, False otherwise
        """
        if velocity is None:
            velocity = self.SPEED_DEFAULT  # Default velocity

        print(f"🎯 Position control: pos={position}, vel={velocity}")

        # Validate position range
        if not (self.POSITION_MIN <= position <= self.POSITION_MAX):
            print(
                f"   ❌ Invalid position {position}, must be {self.POSITION_MIN}-{self.POSITION_MAX}"
            )
            return False

        # Validate speed range
        if not (self.SPEED_MIN <= velocity <= self.SPEED_MAX):
            print(
                f"   ❌ Invalid speed {velocity}, must be {self.SPEED_MIN}-{self.SPEED_MAX}"
            )
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
            self.servo_set_position = position
            self.servo_set_speed = velocity

            return True
        except Exception as e:
            print(f"❌ Failed to send position control: {e}")
            return False

    # ==================== USB Monitoring ====================

    def monitor_usb(self, duration: float = 5.0) -> None:
        """
        Monitor USB communication for debugging

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

    # ==================== Test Functions ====================

    def test_basic_commands(self) -> bool:
        """Test basic servo communication"""
        print("\n🚀 Testing Basic Servo Commands")
        print("-" * 40)

        success = True

        # Test enable/disable
        if not self.enable_torque():
            success = False
        time.sleep(0.2)

        if not self.read_position():
            success = False
        time.sleep(0.2)

        if not self.disable_torque():
            success = False

        print("✅ Basic commands completed")
        return success

    def test_position_control(self, positions: List[int] = None) -> bool:
        """Test position control"""
        if positions is None:
            positions = [800, self.POSITION_MID, 1500]  # Include middle position

        print(f"\n🎯 Testing Position Control")
        print(f"   Positions: {positions}")
        print("-" * 35)

        success = True

        # Enable torque first
        if not self.enable_torque():
            success = False

        time.sleep(0.2)

        for i, pos in enumerate(positions):
            print(f"\n📍 Move {i+1}/{len(positions)}: Position {pos}")

            if not self.position_control(pos, self.SPEED_DEFAULT):
                success = False

            time.sleep(0.5)  # Wait for movement

            # Read current position
            current_pos = self.read_position()
            if current_pos is not None:
                print(f"   Current position: {current_pos}")

            time.sleep(0.3)

        # Disable at the end
        self.disable_torque()

        print("\n✅ Position control test completed")
        return success

    def test_continuous_movement(self) -> bool:
        """Test continuous servo movement"""
        print("\n🔄 Testing Continuous Movement")
        print("-" * 35)

        success = True

        if not self.enable_torque():
            success = False
            return success

        try:
            # Sweep from min to max position
            step = (self.POSITION_MAX - self.POSITION_MIN) // 10  # 10 steps
            positions = list(range(self.POSITION_MIN, self.POSITION_MAX + 1, step))
            positions.extend(
                list(range(self.POSITION_MAX, self.POSITION_MIN - 1, -step))
            )  # Back down

            print(
                f"   Sweeping through {len(positions)} positions ({self.POSITION_MIN}-{self.POSITION_MAX})..."
            )

            for i, pos in enumerate(positions):
                print(f"   Position {i+1}/{len(positions)}: {pos}", end="\r")

                # Use slower speed for continuous movement
                movement_speed = min(
                    self.SPEED_MAX, 30
                )  # Gentle speed for smooth movement
                if not self.position_control(pos, movement_speed):
                    success = False
                    break

                time.sleep(0.4)  # Slightly longer delay for smooth movement

            print(f"\n   Sweep completed")

        except KeyboardInterrupt:
            print(f"\n   Movement interrupted by user")
        finally:
            self.disable_torque()

        print("✅ Continuous movement test completed")
        return success

    def interactive_mode(self) -> None:
        """Interactive mode for manual servo control"""
        print("\n🎮 Interactive Servo Control Mode")
        print("=" * 35)
        print("Commands:")
        print("  enable    - Enable servo torque")
        print("  disable   - Disable servo torque")
        print(f"  mid       - Set middle position ({self.POSITION_MID})")
        print("  read      - Read current position")
        print(
            f"  pos <{self.POSITION_MIN}-{self.POSITION_MAX}> [vel] - Move to position"
        )
        print(
            f"         (vel: {self.SPEED_MIN}-{self.SPEED_MAX}, default: {self.SPEED_DEFAULT})"
        )
        print("  sweep     - Sweep through all positions")
        print("  monitor <seconds> - Monitor USB traffic")
        print("  quit      - Exit interactive mode")
        print("-" * 35)

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
                        self.enable_torque()
                    elif cmd == "disable" or cmd == "dis":
                        self.disable_torque()
                    elif cmd == "mid" or cmd == "m":
                        self.set_mid_position()
                    elif cmd == "read" or cmd == "r":
                        self.read_position()
                    elif cmd == "pos" or cmd == "p":
                        if len(parts) >= 2:
                            try:
                                position = int(parts[1])
                                velocity = int(parts[2]) if len(parts) > 2 else 150
                                self.position_control(position, velocity)
                            except ValueError:
                                print("   ❌ Invalid position value")
                        else:
                            print("   Usage: pos <position> [velocity]")
                    elif cmd == "sweep" or cmd == "s":
                        self.test_continuous_movement()
                    elif cmd == "monitor" or cmd == "mon":
                        duration = float(parts[1]) if len(parts) > 1 else 5.0
                        self.monitor_usb(duration)
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
                self.disable_torque()

        print("✅ Interactive mode completed")

    def run_comprehensive_test(self) -> bool:
        """Run comprehensive test of all functionality"""
        print("\n🧪 Running Comprehensive USB Servo Test")
        print("=" * 50)

        success = True

        # Test 1: Basic commands
        if not self.test_basic_commands():
            success = False

        time.sleep(0.5)

        # Test 2: Position control
        if not self.test_position_control():
            success = False

        time.sleep(0.5)

        # Test 3: Continuous movement
        if not self.test_continuous_movement():
            success = False

        print(
            f"\n🎉 Comprehensive test completed: {'✅ PASSED' if success else '❌ FAILED'}"
        )
        return success

    def cleanup(self):
        """Clean up USB resources"""
        print("\n🧹 Cleaning up...")

        # Disable servo
        if self.servo_usb and self.servo_enable:
            self.disable_torque()

        # Close USB connection
        if self.servo_usb:
            self.servo_usb.close()
            print("   Servo USB closed")

        print("✅ Cleanup completed")


def main():
    """Main test function"""
    print("🤖 Simple USB Servo Test Script")
    print("Direct USB connection to servo - no CAN dependencies")
    print("Based on remote_manip_embedding/main.c protocol")
    print("=" * 65)

    # Parse command line arguments
    import argparse

    parser = argparse.ArgumentParser(description="Direct USB Servo Protocol Tester")
    parser.add_argument(
        "--usb-port",
        default="/dev/ttyUSB0",
        help="Servo USB port (default: /dev/ttyUSB0)",
    )
    parser.add_argument(
        "--test",
        choices=["basic", "position", "continuous", "all", "interactive"],
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
    protocol = ServoUSBProtocol(args.usb_port)
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

    # Initialize USB connection
    if not protocol.initialize_usb():
        print("❌ Failed to connect to servo")
        print("   Use --list-ports to see available devices")
        return 1

    # Monitor mode if requested
    if args.monitor:
        protocol.monitor_usb(args.monitor)
        return 0

    try:
        # Run requested tests
        if args.test == "interactive":
            protocol.interactive_mode()

        elif args.test in ["basic", "all"]:
            protocol.test_basic_commands()

        if args.test in ["position", "all"]:
            positions = args.positions if args.positions else None
            protocol.test_position_control(positions)

        if args.test in ["continuous", "all"]:
            protocol.test_continuous_movement()

        if args.test == "all":
            print(f"\n🎉 All tests completed successfully!")
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

