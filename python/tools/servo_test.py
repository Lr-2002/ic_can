#!/usr/bin/env python3
"""
Servo Communication Test Script

Fast test script for servo angle send/read commands based on IC_CAN protocol.
Extracted from remote_manip_embedding servo control system.

Commands extracted:
- Position control: 0x02 0x08 0x00 (write position)
- Status read: 0x02 0x05 00 (read status)
- Enable servo: 0x02 0x01 0x00
- Disable servo: 0x02 0x01 0x01

Protocol:
- CAN ID: 0x09 for servo commands
- Response ID: 0x19 for servo feedback
- 8-byte CAN frames
"""

import time
import struct
import sys
import os

# Add the project root to path for imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'python'))

try:
    import ic_can_python
except ImportError:
    print("❌ ic_can_python module not found. Build the project first:")
    print("   mkdir -p build && cd build && cmake .. -DBUILD_PYTHON_BINDINGS=ON && make -j$(nproc)")
    sys.exit(1)


class ServoTester:
    """Fast servo communication tester based on extracted I/O commands"""

    def __init__(self, motor_id=9):
        """
        Initialize servo tester

        Args:
            motor_id: Servo motor ID (default: 9 for gripper)
        """
        self.motor_id = motor_id
        self.can_id = 0x09  # Servo CAN send ID
        self.response_id = 0x19  # Expected response ID

        # Initialize IC_CAN interface
        self.ic_can = None
        self.initialized = False

        print(f"🔧 Servo Tester - Motor ID: {motor_id}, CAN ID: 0x{self.can_id:02X}")
        print("=" * 50)

    def initialize(self):
        """Initialize CAN communication"""
        try:
            self.ic_can = ic_can_python.IC_CAN()
            print("✅ IC_CAN interface initialized")
            self.initialized = True
            return True
        except Exception as e:
            print(f"❌ Failed to initialize IC_CAN: {e}")
            return False

    def send_raw_command(self, data_bytes, description=""):
        """
        Send raw CAN command to servo

        Args:
            data_bytes: List of 8 bytes to send
            description: Description of command for logging
        """
        if not self.initialized:
            print("❌ IC_CAN not initialized")
            return False

        try:
            # Pad to 8 bytes if needed
            cmd_data = data_bytes + [0x00] * (8 - len(data_bytes))

            # Send command via IC_CAN
            self.ic_can.send_can_frame(self.can_id, cmd_data)

            print(f"📤 SENT: {description}")
            print(f"   ID: 0x{self.can_id:02X} Data: {' '.join(f'{b:02X}' for b in cmd_data)}")
            return True

        except Exception as e:
            print(f"❌ Send failed: {e}")
            return False

    def read_response(self, timeout_ms=100):
        """
        Read response from servo

        Args:
            timeout_ms: Timeout in milliseconds

        Returns:
            List of 8 response bytes or None if timeout
        """
        if not self.initialized:
            return None

        start_time = time.time()
        timeout_sec = timeout_ms / 1000.0

        while (time.time() - start_time) < timeout_sec:
            try:
                # Try to read a CAN frame
                frame = self.ic_can.receive_can_frame(timeout_ms=1)
                if frame and len(frame) >= 2:
                    can_id, data = frame[0], frame[1]
                    if can_id == self.response_id:
                        print(f"📥 RECV: ID: 0x{can_id:02X} Data: {' '.join(f'{b:02X}' for b in data)}")
                        return data
            except:
                pass

        print(f"📥 No response within {timeout_ms}ms")
        return None

    def enable_servo(self):
        """Enable servo motor - Command: [0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]"""
        print("\n🔌 Enabling servo...")
        return self.send_raw_command([0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00], "Enable Servo")

    def disable_servo(self):
        """Disable servo motor - Command: [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]"""
        print("\n🔌 Disabling servo...")
        return self.send_raw_command([0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00], "Disable Servo")

    def write_position(self, position):
        """
        Write position to servo - Command: [0x02, pos_high, pos_low, 0x00, 0x00, 0x00, 0x00, 0x00]

        Args:
            position: Position value (0-4095 for typical servos)
        """
        # Convert position to 2-byte big-endian format (matching IC_CAN implementation)
        pos_high = (position >> 8) & 0xFF
        pos_low = position & 0xFF

        print(f"\n🎯 Writing position: {position}")
        return self.send_raw_command([0x02, pos_high, pos_low, 0x00, 0x00, 0x00, 0x00, 0x00],
                                   f"Write Position {position}")

    def read_status(self):
        """Read servo status - Command: [0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]"""
        print("\n📊 Reading servo status...")
        return self.send_raw_command([0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00], "Read Status")

    def read_position(self):
        """Read current position - Command: [0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]"""
        print("\n📏 Reading current position...")
        return self.send_raw_command([0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00], "Read Position")

    def test_basic_communication(self):
        """Test basic servo communication patterns"""
        print("\n🚀 Starting Basic Communication Test")
        print("-" * 40)

        # Test 1: Enable servo
        if self.enable_servo():
            self.read_response()
            time.sleep(0.1)

        # Test 2: Read status
        if self.read_status():
            response = self.read_response()
            if response:
                print(f"   Status response: {' '.join(f'{b:02X}' for b in response)}")
            time.sleep(0.1)

        # Test 3: Read position
        if self.read_position():
            response = self.read_response()
            if response and len(response) >= 3:
                # Extract position from response (typically bytes 2-3)
                pos = response[2] | (response[3] << 8)
                print(f"   Current position: {pos}")
            time.sleep(0.1)

        print("\n✅ Basic communication test completed")

    def test_position_control(self, positions=None):
        """Test position control with multiple positions"""
        if positions is None:
            positions = [1000, 2000, 3000, 2000, 1000, 0]

        print(f"\n🎯 Testing Position Control")
        print(f"   Target positions: {positions}")
        print("-" * 40)

        for i, pos in enumerate(positions):
            print(f"\n📍 Move {i+1}/{len(positions)}: Position {pos}")

            if self.write_position(pos):
                self.read_response()

                # Read current position after command
                time.sleep(0.2)
                if self.read_position():
                    response = self.read_response()
                    if response and len(response) >= 3:
                        current_pos = response[2] | (response[3] << 8)
                        print(f"   → Reached position: {current_pos}")

                time.sleep(0.3)  # Wait between moves

        print("\n✅ Position control test completed")

    def test_alternating_commands(self, count=10, interval_ms=100):
        """Test alternating commands (like can_test.cpp)"""
        print(f"\n🔄 Testing Alternating Commands")
        print(f"   Commands: 02 08 00 ↔ 02 05 00 (matching can_test.cpp)")
        print(f"   Count: {count}, Interval: {interval_ms}ms")
        print("-" * 50)

        print("   Count        Time             SEND              RECEIVE")
        print("-" * 64)

        start_time = time.time()

        for i in range(count):
            current_time = (time.time() - start_time) * 1000  # ms

            if i % 2 == 0:
                # Send position command (02 08 00)
                self.send_raw_command([0x02, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00], "Position 02 08 00")
            else:
                # Send status command (02 05 00) - using read status command format
                self.send_raw_command([0x02, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00], "Status 02 05 00")

            # Listen for response
            response = self.read_response(timeout_ms=50)

            print(f"   {i+1:2d}     [{current_time:7.1f}ms]    {'✅':6s}     {'✅' if response else '❌':6s}")

            # Wait for next command
            if i < count - 1:
                time.sleep(interval_ms / 1000.0)

        actual_freq = count / ((time.time() - start_time))
        print(f"\n📊 Alternating test completed")
        print(f"   Actual frequency: {actual_freq:.2f} Hz")

    def cleanup(self):
        """Clean up and disable servo"""
        print("\n🧹 Cleaning up...")
        if self.initialized:
            self.disable_servo()
            self.read_response()

            # Close IC_CAN interface if available
            if hasattr(self.ic_can, 'shutdown'):
                self.ic_can.shutdown()

        print("✅ Cleanup completed")


def main():
    """Main test function"""
    print("🤖 Servo Communication Test Script")
    print("Based on remote_manip_embedding I/O commands")
    print("=" * 60)

    # Parse command line arguments
    import argparse
    parser = argparse.ArgumentParser(description='Servo Communication Tester')
    parser.add_argument('--motor-id', type=int, default=9, help='Motor ID (default: 9)')
    parser.add_argument('--test', choices=['basic', 'position', 'alternating', 'all'],
                       default='all', help='Test to run (default: all)')
    parser.add_argument('--positions', nargs='+', type=int,
                       help='Custom positions for position test')
    parser.add_argument('--count', type=int, default=10,
                       help='Number of alternating commands (default: 10)')
    parser.add_argument('--interval', type=int, default=100,
                       help='Interval between commands in ms (default: 100)')

    args = parser.parse_args()

    # Create servo tester
    tester = ServoTester(motor_id=args.motor_id)

    # Initialize
    if not tester.initialize():
        print("❌ Failed to initialize servo tester")
        return 1

    try:
        # Run requested tests
        if args.test in ['basic', 'all']:
            tester.test_basic_communication()

        if args.test in ['position', 'all']:
            positions = args.positions if args.positions else None
            tester.test_position_control(positions)

        if args.test in ['alternating', 'all']:
            tester.test_alternating_commands(count=args.count, interval_ms=args.interval)

        print("\n🎉 All tests completed successfully!")
        return 0

    except KeyboardInterrupt:
        print("\n⚠️ Test interrupted by user")
        return 1

    except Exception as e:
        print(f"\n❌ Test failed with error: {e}")
        return 1

    finally:
        tester.cleanup()


if __name__ == "__main__":
    sys.exit(main())