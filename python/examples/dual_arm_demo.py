#!/usr/bin/env python3
"""
Dual-Arm System Demonstration

This script demonstrates the capabilities of the dual-arm IC_CAN system including:
- Dual-arm initialization and configuration
- Independent arm control
- Coordinated bimanual movements
- Camera integration
- Safety monitoring
- Performance monitoring

Usage:
    python dual_arm_demo.py [--mode basic|coordinated|camera|safety|full]
"""

import numpy as np
import time
import logging
import argparse
import sys
from pathlib import Path

# Add the ic_can module to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from ic_can.dual_arm_manager import DualArmManager
from ic_can.camera_manager import CameraManager
from ic_can.dual_arm_safety import DualArmSafety
from ic_can.dual_arm_coordinator import DualArmCoordinator, CoordinationMode

# Setup logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class DualArmDemo:
    """Comprehensive dual-arm system demonstration."""

    def __init__(self, debug=False):
        """Initialize demo with debug mode."""
        self.debug = debug
        self.manager = None
        self.camera_manager = None
        self.safety_system = None
        self.coordinator = None

        # Demo parameters
        self.demo_positions = self._generate_demo_positions()

    def _generate_demo_positions(self):
        """Generate demonstration position sequences."""
        return {
            'home': np.zeros(9),
            'ready': np.array([0.5, 0.3, 0.2, 0.1, 0.0, 0.0, 0.0, 0.0, 0.0]),
            'reach': np.array([1.0, 0.5, 0.3, 0.2, 0.1, 0.0, 0.5, 0.0, 0.5]),
            'grasp': np.array([1.2, 0.6, 0.4, 0.3, 0.2, 0.1, 0.8, 0.3, 0.0]),
            'transport': np.array([0.8, 0.4, 0.2, 0.1, 0.0, -0.1, 0.6, 0.0, 0.3]),
            'place': np.array([0.6, 0.2, 0.0, 0.0, -0.2, -0.2, 0.4, -0.2, 0.8]),
        }

    def run_basic_demo(self):
        """Run basic dual-arm demonstration."""
        print("\n🚀 Basic Dual-Arm Demo")
        print("=" * 50)

        # Initialize dual-arm manager
        self.manager = DualArmManager(debug=self.debug)
        print("✅ Dual-arm manager created")

        # Initialize arms
        if not self.manager.initialize_arms():
            print("❌ Failed to initialize arms")
            return False

        # Enable arms
        if not self.manager.enable_arms():
            print("❌ Failed to enable arms")
            return False

        print("✅ Both arms initialized and enabled")

        # Show status
        self.manager.print_status()

        # Move to home position
        print("\n🏠 Moving both arms to home position...")
        if self.manager.home_both_arms():
            print("✅ Both arms at home position")
            time.sleep(2)
        else:
            print("❌ Failed to move to home position")

        # Independent arm movements
        print("\n🦾 Independent arm movements...")

        # Left arm movement
        print("Moving left arm to ready position...")
        if self.manager.move_left_arm(self.demo_positions['ready']):
            print("✅ Left arm moved")
            time.sleep(1)

        # Right arm movement
        print("Moving right arm to reach position...")
        if self.manager.move_right_arm(self.demo_positions['reach']):
            print("✅ Right arm moved")
            time.sleep(1)

        # Show final status
        print("\n📊 Final status:")
        self.manager.print_status()

        return True

    def run_coordinated_demo(self):
        """Run coordinated bimanual demonstration."""
        print("\n🤝 Coordinated Bimanual Demo")
        print("=" * 50)

        if not self.manager:
            print("❌ Dual-arm manager not initialized")
            return False

        # Create safety system
        self.safety_system = DualArmSafety(
            self.manager,
            max_inter_arm_distance=0.3,
            enable_collision_detection=True
        )
        print("✅ Safety system created")

        # Create coordinator
        self.coordinator = DualArmCoordinator(
            self.manager,
            safety_system=self.safety_system
        )
        self.coordinator.start_coordination()
        print("✅ Coordinator started")

        # Synchronized movement
        print("\n🎯 Synchronized movement...")
        left_target = self.demo_positions['grasp']
        right_target = self.demo_positions['transport']

        if self.manager.move_both_arms(left_target, right_target):
            print("✅ Synchronized movement completed")
            time.sleep(2)

        # Coordinated transport task
        print("\n📦 Coordinated transport task...")
        start_pos = (self.demo_positions['grasp'], self.demo_positions['transport'])
        end_pos = (self.demo_positions['place'], self.demo_positions['ready'])

        if self.coordinator.transport_object(start_pos, end_pos, duration=3.0):
            print("✅ Transport task completed")
            time.sleep(4)

        # Show coordination status
        print("\n📊 Coordination status:")
        self.coordinator.print_coordination_status()

        return True

    def run_camera_demo(self):
        """Run camera integration demonstration."""
        print("\n📷 Camera Integration Demo")
        print("=" * 50)

        if not self.manager:
            print("❌ Dual-arm manager not initialized")
            return False

        # Create camera manager
        self.camera_manager = CameraManager(
            dual_arm_manager=self.manager,
            debug=self.debug
        )
        print("✅ Camera manager created")

        # Initialize cameras
        if self.camera_manager.initialize_cameras():
            print("✅ Cameras initialized")
        else:
            print("⚠️ Camera initialization failed (simulated mode)")

        # Start capture
        if self.camera_manager.start_capture():
            print("✅ Camera capture started")
        else:
            print("⚠️ Camera capture failed (simulated mode)")

        # Show camera status
        print("\n📊 Camera status:")
        self.camera_manager.print_status()

        # Capture synchronized frames
        print("\n📸 Capturing synchronized frames...")
        for i in range(3):
            frames = self.camera_manager.get_synchronized_frames(timeout=0.5)
            if frames:
                print(f"✅ Captured frame set {i+1}: {len(frames)} cameras")
                time.sleep(1)
            else:
                print(f"⚠️ No frames captured (set {i+1})")

        return True

    def run_safety_demo(self):
        """Run safety system demonstration."""
        print("\n⚠️ Safety System Demo")
        print("=" * 50)

        if not self.manager:
            print("❌ Dual-arm manager not initialized")
            return False

        if not self.safety_system:
            self.safety_system = DualArmSafety(
                self.manager,
                max_inter_arm_distance=0.2,
                enable_collision_detection=True,
                enable_workspace_monitoring=True
            )
            print("✅ Safety system created")

        # Add safety zones
        print("\n🚫 Adding safety zones...")
        center = np.array([0.5, 0.0, 0.0, 0.0, 0.0, 0.0])
        self.safety_system.add_safety_zone(
            center=center,
            radius=0.1,
            zone_type="forbidden",
            name="danger_zone"
        )
        print("✅ Forbidden zone added")

        # Monitor safety during movement
        print("\n🔍 Monitoring safety during movement...")

        # Refresh states and check safety
        for i in range(5):
            self.manager.refresh_states()
            state = self.manager.get_current_state()

            if state:
                events = self.safety_system.check_safety(state)
                if events:
                    for event in events:
                        print(f"  ⚠️ Safety event: {event.description}")
                else:
                    print(f"  ✅ Check {i+1}: No safety issues")
            else:
                print(f"  ❌ Check {i+1}: No state available")

            time.sleep(0.5)

        # Show safety status
        print("\n📊 Safety status:")
        self.safety_system.print_safety_status()

        return True

    def run_full_demo(self):
        """Run complete dual-arm system demonstration."""
        print("\n🎯 Full Dual-Arm System Demo")
        print("=" * 50)

        success = True

        # Basic setup
        if not self.run_basic_demo():
            success = False

        # Camera integration
        if success:
            if not self.run_camera_demo():
                print("⚠️ Camera demo failed, continuing...")

        # Safety system
        if success:
            if not self.run_safety_demo():
                print("⚠️ Safety demo failed, continuing...")

        # Coordinated control
        if success:
            if not self.run_coordinated_demo():
                print("⚠️ Coordination demo failed")

        # Final status report
        print("\n📋 Final System Status:")
        print("=" * 50)

        if self.manager:
            self.manager.print_status()

        if self.coordinator:
            self.coordinator.print_coordination_status()

        if self.camera_manager:
            self.camera_manager.print_status()

        if self.safety_system:
            self.safety_system.print_safety_status()

        return success

    def cleanup(self):
        """Clean up all system components."""
        print("\n🧹 Cleaning up...")

        if self.coordinator:
            self.coordinator.shutdown()
            print("✅ Coordinator shutdown")

        if self.camera_manager:
            self.camera_manager.shutdown()
            print("✅ Camera manager shutdown")

        if self.safety_system:
            self.safety_system.clear_emergency_stop()
            print("✅ Safety system cleared")

        if self.manager:
            self.manager.shutdown()
            print("✅ Dual-arm manager shutdown")

        print("✅ Cleanup complete")

def main():
    """Main demonstration entry point."""
    parser = argparse.ArgumentParser(description="Dual-Arm IC_CAN System Demo")
    parser.add_argument(
        '--mode',
        choices=['basic', 'coordinated', 'camera', 'safety', 'full'],
        default='basic',
        help='Demo mode to run'
    )
    parser.add_argument(
        '--debug',
        action='store_true',
        help='Enable debug logging'
    )

    args = parser.parse_args()

    # Create demo instance
    demo = DualArmDemo(debug=args.debug)

    try:
        # Run selected demo
        if args.mode == 'basic':
            success = demo.run_basic_demo()
        elif args.mode == 'coordinated':
            # Need basic setup first
            demo.run_basic_demo()
            success = demo.run_coordinated_demo()
        elif args.mode == 'camera':
            # Need basic setup first
            demo.run_basic_demo()
            success = demo.run_camera_demo()
        elif args.mode == 'safety':
            # Need basic setup first
            demo.run_basic_demo()
            success = demo.run_safety_demo()
        elif args.mode == 'full':
            success = demo.run_full_demo()

        if success:
            print(f"\n🎉 Demo '{args.mode}' completed successfully!")
        else:
            print(f"\n❌ Demo '{args.mode}' failed!")
            return 1

    except KeyboardInterrupt:
        print("\n⚠️ Demo interrupted by user")
        return 1
    except Exception as e:
        print(f"\n💥 Demo error: {e}")
        if args.debug:
            import traceback
            traceback.print_exc()
        return 1
    finally:
        # Always cleanup
        demo.cleanup()

    return 0

if __name__ == "__main__":
    sys.exit(main())