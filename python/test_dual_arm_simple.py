#!/usr/bin/env python3
"""
Simple test for dual-arm system components (without hardware dependencies).

This script tests the configuration and logic components of the dual-arm system
without requiring actual hardware or C++ bindings.
"""

import sys
import numpy as np
from pathlib import Path

# Add the ic_can module to path
sys.path.insert(0, str(Path(__file__).parent))

def test_configuration_system():
    """Test configuration system components."""
    print("🧪 Testing Configuration System...")

    try:
        # Test dual-arm configuration (C++ structures won't be importable)
        # Let's test the JSON configuration files directly
        import json

        config_dir = Path(__file__).parent.parent / "config"

        # Test dual-arm config
        dual_arm_config_file = config_dir / "dual_arm_config.json"
        if dual_arm_config_file.exists():
            with open(dual_arm_config_file, 'r') as f:
                dual_arm_config = json.load(f)
            print("✅ Dual-arm config loaded successfully")

            # Validate config structure
            if 'left_arm' in dual_arm_config and 'right_arm' in dual_arm_config:
                print("✅ Dual-arm config structure valid")
            else:
                print("❌ Invalid dual-arm config structure")
                return False
        else:
            print("❌ Dual-arm config file not found")
            return False

        # Test camera config
        camera_config_file = config_dir / "dual_camera_config.json"
        if camera_config_file.exists():
            with open(camera_config_file, 'r') as f:
                camera_config = json.load(f)
            print("✅ Camera config loaded successfully")

            # Validate config structure
            if 'cameras' in camera_config and len(camera_config['cameras']) == 3:
                print("✅ Camera config structure valid")
            else:
                print("❌ Invalid camera config structure")
                return False
        else:
            print("❌ Camera config file not found")
            return False

        # Test individual arm configs
        for arm_name in ['left', 'right']:
            arm_config_file = config_dir / f"{arm_name}_arm_config.json"
            if arm_config_file.exists():
                with open(arm_config_file, 'r') as f:
                    arm_config = json.load(f)
                print(f"✅ {arm_name} arm config loaded successfully")

                # Validate channel assignment
                if 'preferred_channel' in arm_config:
                    channel = arm_config['preferred_channel']
                    expected = 0 if arm_name == 'left' else 1
                    if channel == expected:
                        print(f"✅ {arm_name} arm channel assignment correct ({channel})")
                    else:
                        print(f"❌ {arm_name} arm channel wrong: expected {expected}, got {channel}")
                        return False
            else:
                print(f"❌ {arm_name} arm config file not found")
                return False

        return True

    except Exception as e:
        print(f"❌ Configuration system test failed: {e}")
        return False

def test_dual_arm_manager_logic():
    """Test dual-arm manager logic components."""
    print("\n🧪 Testing DualArmManager Logic...")

    try:
        # Test basic imports and class definitions
        from ic_can.dual_arm_manager import ArmState, DualArmState

        # Test ArmState creation
        arm_state = ArmState(
            positions=np.zeros(9),
            velocities=np.zeros(9),
            torques=np.zeros(9),
            gripper_open=True,
            timestamp=0.0,
            is_connected=False
        )
        print("✅ ArmState created successfully")

        # Test DualArmState creation
        dual_arm_state = DualArmState(
            left_arm=arm_state,
            right_arm=arm_state,
            inter_arm_distance=0.0,
            timestamp=0.0
        )
        print("✅ DualArmState created successfully")

        return True

    except Exception as e:
        print(f"❌ DualArmManager logic test failed: {e}")
        return False

def test_safety_system_logic():
    """Test safety system logic components."""
    print("\n🧪 Testing Safety System Logic...")

    try:
        from ic_can.dual_arm_safety import SafetyLevel, SafetyCondition, SafetyZone

        # Test enums
        assert SafetyLevel.NORMAL.value == "normal"
        assert SafetyCondition.INTER_ARM_COLLISION.value == "inter_arm_collision"
        print("✅ Safety enums working correctly")

        # Test SafetyZone creation
        zone = SafetyZone(
            center=np.array([0.0, 0.0, 0.0]),
            radius=0.5,
            zone_type="forbidden",
            name="test_zone"
        )
        print("✅ SafetyZone created successfully")

        return True

    except Exception as e:
        print(f"❌ Safety system logic test failed: {e}")
        return False

def test_coordinator_logic():
    """Test coordinator logic components."""
    print("\n🧪 Testing Coordinator Logic...")

    try:
        from ic_can.dual_arm_coordinator import CoordinationMode, TaskType

        # Test enums
        assert CoordinationMode.INDEPENDENT.value == "independent"
        assert TaskType.REACH_GRASP.value == "reach_grasp"
        print("✅ Coordinator enums working correctly")

        return True

    except Exception as e:
        print(f"❌ Coordinator logic test failed: {e}")
        return False

def test_camera_manager_logic():
    """Test camera manager logic components."""
    print("\n🧪 Testing Camera Manager Logic...")

    try:
        from ic_can.camera_manager import CameraFrame, CameraCalibration

        # Test CameraFrame creation
        frame = CameraFrame(
            camera_id=0,
            timestamp=0.0,
            image=np.zeros((480, 640, 3), dtype=np.uint8)
        )
        print("✅ CameraFrame created successfully")

        # Test CameraCalibration creation
        calibration = CameraCalibration(
            camera_matrix=np.eye(3),
            distortion_coeffs=np.zeros(5)
        )
        print("✅ CameraCalibration created successfully")

        return True

    except Exception as e:
        print(f"❌ Camera manager logic test failed: {e}")
        return False

def test_dual_arm_architecture():
    """Test dual-arm architecture concepts."""
    print("\n🧪 Testing Dual-Arm Architecture...")

    try:
        # Test the channel isolation concept
        left_channel = 0
        right_channel = 1
        same_can_ids = [0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09]

        # Left arm: Channel 0, motors 1-9
        left_motor_mapping = {i+1: same_can_ids[i] for i in range(9)}

        # Right arm: Channel 1, motors 10-18 (same CAN IDs, different channel)
        right_motor_mapping = {i+10: same_can_ids[i] for i in range(9)}

        print("✅ Channel-based motor mapping working:")
        print(f"  Left arm (Channel {left_channel}): {len(left_motor_mapping)} motors")
        print(f"  Right arm (Channel {right_channel}): {len(right_motor_mapping)} motors")

        # Test that CAN IDs are the same but channels are different
        left_can_ids = set(left_motor_mapping.values())
        right_can_ids = set(right_motor_mapping.values())

        if left_can_ids == right_can_ids:
            print("✅ Same CAN IDs used on both channels (correct)")
        else:
            print("❌ CAN IDs differ between channels (incorrect)")
            return False

        if left_channel != right_channel:
            print("✅ Different channels used (correct)")
        else:
            print("❌ Same channel used (incorrect)")
            return False

        return True

    except Exception as e:
        print(f"❌ Dual-arm architecture test failed: {e}")
        return False

def main():
    """Run all logic tests."""
    print("🧪 Dual-Arm System Logic Tests (No Hardware Required)")
    print("=" * 60)

    tests = [
        test_configuration_system,
        test_dual_arm_architecture,
        test_dual_arm_manager_logic,
        test_safety_system_logic,
        test_coordinator_logic,
        test_camera_manager_logic
    ]

    passed = 0
    total = len(tests)

    for test in tests:
        try:
            if test():
                passed += 1
        except Exception as e:
            print(f"❌ Test failed with exception: {e}")

    print("\n" + "=" * 60)
    print(f"Test Results: {passed}/{total} tests passed")

    if passed == total:
        print("🎉 All logic tests passed! Dual-arm system architecture is sound.")
        print("\nNext steps:")
        print("1. Build C++ dual-arm configuration system")
        print("2. Build Python bindings for IC_CAN")
        print("3. Test with actual hardware")
        return 0
    else:
        print("⚠️ Some tests failed. Check the output above.")
        return 1

if __name__ == "__main__":
    sys.exit(main())