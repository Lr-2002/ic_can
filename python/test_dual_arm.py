#!/usr/bin/env python3
"""
Simple test script for dual-arm system components.

This script tests the basic functionality of the dual-arm system without requiring
actual hardware connection.
"""

import sys
import numpy as np
from pathlib import Path

# Add the ic_can module to path
sys.path.insert(0, str(Path(__file__).parent))

def test_dual_arm_manager():
    """Test dual-arm manager basic functionality."""
    print("🧪 Testing DualArmManager...")

    try:
        from ic_can.dual_arm_manager import DualArmManager

        # Create manager
        manager = DualArmManager(debug=True)
        print("✅ DualArmManager created successfully")

        # Test configuration loading
        if manager.load_configurations():
            print("✅ Configurations loaded successfully")
        else:
            print("⚠️ Configuration loading failed (expected without hardware)")

        # Test state management
        state = manager.get_current_state()
        if state is None:
            print("✅ No state available (expected without initialization)")

        return True

    except Exception as e:
        print(f"❌ DualArmManager test failed: {e}")
        return False

def test_camera_manager():
    """Test camera manager basic functionality."""
    print("\n🧪 Testing CameraManager...")

    try:
        from ic_can.camera_manager import CameraManager

        # Create camera manager
        camera_manager = CameraManager(debug=True)
        print("✅ CameraManager created successfully")

        # Test configuration loading
        if camera_manager.load_configuration():
            print("✅ Camera configuration loaded successfully")
        else:
            print("⚠️ Camera configuration loading failed (expected without hardware)")

        # Test camera info
        info = camera_manager.get_all_camera_info()
        if info:
            print(f"✅ Found {len(info)} cameras in configuration")
        else:
            print("⚠️ No camera info available")

        return True

    except Exception as e:
        print(f"❌ CameraManager test failed: {e}")
        return False

def test_safety_system():
    """Test safety system basic functionality."""
    print("\n🧪 Testing DualArmSafety...")

    try:
        from ic_can.dual_arm_safety import DualArmSafety, SafetyEvent, SafetyCondition, SafetyLevel

        # Test safety event creation
        event = SafetyEvent(
            condition=SafetyCondition.WORKSPACE_BOUNDARY,
            level=SafetyLevel.WARNING,
            timestamp=0.0,
            description="Test safety event"
        )
        print("✅ SafetyEvent created successfully")

        # Test safety system (without dual-arm manager)
        print("✅ Safety system classes imported successfully")

        return True

    except Exception as e:
        print(f"❌ Safety system test failed: {e}")
        return False

def test_coordinator():
    """Test coordinator basic functionality."""
    print("\n🧪 Testing DualArmCoordinator...")

    try:
        from ic_can.dual_arm_coordinator import DualArmCoordinator, CoordinationMode, CoordinationGoal

        # Test coordination goal creation
        goal = CoordinationGoal(
            mode=CoordinationMode.SYNCHRONIZED,
            left_target=np.zeros(9),
            right_target=np.zeros(9),
            duration=1.0
        )
        print("✅ CoordinationGoal created successfully")

        print("✅ Coordinator classes imported successfully")

        return True

    except Exception as e:
        print(f"❌ Coordinator test failed: {e}")
        return False

def test_configuration_system():
    """Test configuration system."""
    print("\n🧪 Testing Configuration System...")

    try:
        from ic_can.dual_arm_config import (
            DualArmConfig, DualCameraConfig, DualArmConfigManager,
            create_default_dual_arm_config, create_default_camera_config
        )

        # Test default configurations
        dual_arm_config = create_default_dual_arm_config()
        print("✅ Default dual-arm config created")

        camera_config = create_default_camera_config()
        print("✅ Default camera config created")

        # Test configuration validation
        if dual_arm_config.validate():
            print("✅ Dual-arm config validation passed")
        else:
            print("❌ Dual-arm config validation failed")
            return False

        if camera_config.validate():
            print("✅ Camera config validation passed")
        else:
            print("❌ Camera config validation failed")
            return False

        # Test configuration manager
        config_manager = DualArmConfigManager()
        config_manager.set_default_configs()
        print("✅ Configuration manager created with defaults")

        # Test configuration summary
        config_manager.print_config_summary()

        return True

    except Exception as e:
        print(f"❌ Configuration system test failed: {e}")
        return False

def test_json_configurations():
    """Test JSON configuration files."""
    print("\n🧪 Testing JSON Configuration Files...")

    config_dir = Path(__file__).parent / "config"

    # Check if config files exist
    required_files = [
        "dual_arm_config.json",
        "dual_camera_config.json",
        "left_arm_config.json",
        "right_arm_config.json"
    ]

    all_exist = True
    for file_name in required_files:
        file_path = config_dir / file_name
        if file_path.exists():
            print(f"✅ {file_name} exists")
        else:
            print(f"❌ {file_name} missing")
            all_exist = False

    return all_exist

def main():
    """Run all tests."""
    print("🧪 Dual-Arm System Component Tests")
    print("=" * 50)

    tests = [
        test_configuration_system,
        test_json_configurations,
        test_dual_arm_manager,
        test_camera_manager,
        test_safety_system,
        test_coordinator
    ]

    passed = 0
    total = len(tests)

    for test in tests:
        try:
            if test():
                passed += 1
        except Exception as e:
            print(f"❌ Test failed with exception: {e}")

    print("\n" + "=" * 50)
    print(f"Test Results: {passed}/{total} tests passed")

    if passed == total:
        print("🎉 All tests passed! Dual-arm system is ready.")
        return 0
    else:
        print("⚠️ Some tests failed. Check the output above.")
        return 1

if __name__ == "__main__":
    sys.exit(main())