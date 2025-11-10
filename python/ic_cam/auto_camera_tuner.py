#!/usr/bin/env python3
"""
Auto Camera Tuner - Automatically tests and saves the best camera configuration
"""

import cv2
import time
import json
import subprocess
import numpy as np
from datetime import datetime
import os

class AutoCameraTuner:
    def __init__(self, camera_id=2):
        self.camera_id = camera_id
        self.device_path = f'/dev/video{camera_id}'

        # Camera control ranges
        self.controls = {
            'brightness': {'min': -64, 'max': 64, 'step': 4},
            'contrast': {'min': 0, 'max': 100, 'step': 5},
            'saturation': {'min': 0, 'max': 100, 'step': 5},
            'gamma': {'min': 100, 'max': 500, 'step': 20},
            'white_balance_temperature': {'min': 2800, 'max': 6500, 'step': 100},
        }

        # Results storage
        self.test_results = []
        self.best_config = None

        # Initialize camera
        self.cap = cv2.VideoCapture(camera_id, cv2.CAP_V4L)
        if not self.cap.isOpened():
            raise RuntimeError(f"Failed to open camera {camera_id}")

        # Configure for MJPG 60Hz
        mjpg_fourcc = cv2.VideoWriter_fourcc(*'MJPG')
        self.cap.set(cv2.CAP_PROP_FOURCC, mjpg_fourcc)
        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
        self.cap.set(cv2.CAP_PROP_FPS, 60)
        self.cap.set(cv2.CAP_PROP_BUFFERSIZE, 5)

        print("✅ Camera initialized with MJPG format for 60Hz performance")

    def apply_setting(self, control_name, value):
        """Apply a single camera setting."""
        try:
            cmd = f"v4l2-ctl -d {self.device_path} --set-ctrl={control_name}={value}"
            subprocess.run(cmd, shell=True, capture_output=True, timeout=2)
        except Exception as e:
            print(f"   ❌ Failed to set {control_name}: {e}")

    def apply_config(self, config):
        """Apply a complete configuration."""
        # Disable auto white balance first
        self.apply_setting('white_balance_automatic', 0)

        for control_name, value in config.items():
            self.apply_setting(control_name, value)

    def test_config_performance(self, config, duration=3):
        """Test performance of a specific configuration."""
        print(f"🧪 Testing: {config}")

        # Apply configuration
        self.apply_config(config)

        # Wait a moment for settings to settle
        time.sleep(0.5)

        # Test performance
        frame_count = 0
        start_time = time.time()
        frame_times = []

        try:
            while time.time() - start_time < duration:
                frame_start = time.time()

                ret, frame = self.cap.read()
                if not ret:
                    print("   ❌ Failed to read frame")
                    return None

                frame_time = time.time() - frame_start
                frame_times.append(frame_time)
                frame_count += 1

        except Exception as e:
            print(f"   ❌ Test failed: {e}")
            return None

        # Calculate metrics
        total_time = time.time() - start_time
        fps = frame_count / total_time if total_time > 0 else 0

        if frame_times:
            avg_frame_time = np.mean(frame_times) * 1000
            frame_time_std = np.std(frame_times) * 1000
        else:
            avg_frame_time = 0
            frame_time_std = 0

        # Score the configuration (higher is better)
        # Prioritize: 1) FPS > 50, 2) Low frame time variance, 3) Good frame time
        score = 0
        if fps >= 50:
            score += 100  # Excellent for 60Hz
        elif fps >= 30:
            score += 50   # Good for real-time
        else:
            score += fps  # Poor but still counted

        # Penalize high frame time variance (bad for robotics)
        score -= frame_time_std * 0.5

        # Reward good average frame time
        if avg_frame_time <= 20:
            score += 20
        elif avg_frame_time <= 30:
            score += 10

        result = {
            'config': config.copy(),
            'fps': fps,
            'frame_count': frame_count,
            'avg_frame_time_ms': avg_frame_time,
            'frame_time_std_ms': frame_time_std,
            'score': score,
            'timestamp': datetime.now().isoformat()
        }

        print(f"   📊 FPS: {fps:.1f}, Frame Time: {avg_frame_time:.1f}±{frame_time_std:.1f}ms, Score: {score:.1f}")

        return result

    def generate_test_configs(self):
        """Generate test configurations to try."""
        configs = []

        # Preset configurations
        presets = [
            {
                'name': 'Balanced',
                'config': {
                    'brightness': 0,
                    'contrast': 75,
                    'saturation': 85,
                    'gamma': 140,
                    'white_balance_temperature': 5000,
                }
            },
            {
                'name': 'Vibrant',
                'config': {
                    'brightness': 5,
                    'contrast': 90,
                    'saturation': 100,
                    'gamma': 130,
                    'white_balance_temperature': 5500,
                }
            },
            {
                'name': 'Natural',
                'config': {
                    'brightness': -5,
                    'contrast': 70,
                    'saturation': 75,
                    'gamma': 160,
                    'white_balance_temperature': 4500,
                }
            },
            {
                'name': 'High Contrast',
                'config': {
                    'brightness': 0,
                    'contrast': 100,
                    'saturation': 90,
                    'gamma': 120,
                    'white_balance_temperature': 5200,
                }
            }
        ]

        for preset in presets:
            configs.append(preset['config'])

        # Generate some variations around the best presets
        base_config = presets[0]['config']  # Start with balanced

        for brightness_offset in [-10, 0, 10]:
            for contrast_offset in [-10, 0, 10]:
                config = base_config.copy()
                config['brightness'] = max(-64, min(64, config['brightness'] + brightness_offset))
                config['contrast'] = max(0, min(100, config['contrast'] + contrast_offset))
                configs.append(config)

        # Remove duplicates
        unique_configs = []
        seen = set()

        for config in configs:
            # Create a hashable representation
            config_tuple = tuple(sorted(config.items()))
            if config_tuple not in seen:
                seen.add(config_tuple)
                unique_configs.append(config)

        print(f"🎯 Generated {len(unique_configs)} unique configurations to test")
        return unique_configs

    def run_auto_tune(self):
        """Run automatic camera tuning."""
        print("🤖 Starting Automatic Camera Tuning")
        print("="*50)
        print("This will test different camera settings and save the best one")
        print()

        # Generate test configurations
        configs = self.generate_test_configs()

        # Test each configuration
        for i, config in enumerate(configs):
            print(f"🔄 Testing configuration {i+1}/{len(configs)}")

            result = self.test_config_performance(config)
            if result:
                self.test_results.append(result)

                # Auto-save every result
                self.save_config_result(result, i+1)

        # Find best configuration
        if self.test_results:
            self.best_config = max(self.test_results, key=lambda x: x['score'])
            print(f"\n🏆 Best Configuration Found:")
            print(f"   FPS: {self.best_config['fps']:.1f}")
            print(f"   Frame Time: {self.best_config['avg_frame_time_ms']:.1f}±{self.best_config['frame_time_std_ms']:.1f}ms")
            print(f"   Score: {self.best_config['score']:.1f}")
            print(f"   Settings: {self.best_config['config']}")

            # Save best as final configuration
            self.save_best_config()

            # Apply best configuration
            self.apply_config(self.best_config['config'])
            print("\n✅ Best configuration applied to camera!")

            return self.best_config

        else:
            print("❌ No valid configurations found")
            return None

    def save_config_result(self, result, test_number):
        """Save an individual test result."""
        filename = f"auto_test_{test_number:03d}_fps_{result['fps']:.1f}_score_{result['score']:.1f}.json"

        with open(filename, 'w') as f:
            json.dump(result, f, indent=2)

        # Also create a shell script for this config
        script_filename = f"auto_test_{test_number:03d}_apply.sh"
        with open(script_filename, 'w') as f:
            f.write("#!/bin/bash\n")
            f.write(f"# Auto test result {test_number}\n")
            f.write(f"# FPS: {result['fps']:.1f}, Score: {result['score']:.1f}\n")
            f.write("echo 'Applying camera configuration...'\n\n")

            f.write("v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_automatic=0\n")
            for name, value in result['config'].items():
                f.write(f"v4l2-ctl -d /dev/video2 --set-ctrl={name}={value}\n")

        subprocess.run(f"chmod +x {script_filename}", shell=True)

    def save_best_config(self):
        """Save the best configuration as the final result."""
        if not self.best_config:
            return

        # Save as main configuration file
        config_data = {
            'timestamp': datetime.now().isoformat(),
            'camera_id': self.camera_id,
            'best_result': self.best_config,
            'all_results': self.test_results
        }

        filename = f"best_camera_config_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
        with open(filename, 'w') as f:
            json.dump(config_data, f, indent=2)

        # Create final application script
        script_filename = f"apply_best_camera_config.sh"
        with open(script_filename, 'w') as f:
            f.write("#!/bin/bash\n")
            f.write(f"# Best camera configuration - {config_data['timestamp']}\n")
            f.write(f"# Performance: {self.best_config['fps']:.1f} FPS\n")
            f.write(f"# Score: {self.best_config['score']:.1f}\n\n")
            f.write("echo 'Applying BEST camera configuration...'\n\n")

            f.write("v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_automatic=0\n")
            for name, value in self.best_config['config'].items():
                f.write(f"v4l2-ctl -d /dev/video2 --set-ctrl={name}={value}\n")

            f.write("\necho '✅ Best camera configuration applied!'\n")
            fps_value = self.best_config["fps"]
            f.write(f"echo 'Expected performance: {fps_value:.1f} FPS'\n")

        subprocess.run(f"chmod +x {script_filename}", shell=True)

        print(f"\n💾 Best Configuration Saved:")
        print(f"   JSON: {filename}")
        print(f"   Script: {script_filename}")
        print(f"   Performance: {self.best_config['fps']:.1f} FPS")

    def test_best_config(self, duration=10):
        """Test the best configuration for longer duration."""
        if not self.best_config:
            print("❌ No best configuration available")
            return

        print(f"\n🎯 Testing Best Configuration (10 seconds)")
        print("="*50)

        result = self.test_config_performance(self.best_config['config'], duration)

        if result:
            print(f"\n📊 Extended Test Results:")
            print(f"   FPS: {result['fps']:.1f}")
            print(f"   Total Frames: {result['frame_count']}")
            print(f"   Frame Time: {result['avg_frame_time_ms']:.1f}±{result['frame_time_std_ms']:.1f}ms")

            if result['fps'] >= 50:
                print("🎉 EXCELLENT: Ready for 60Hz robotics!")
            elif result['fps'] >= 30:
                print("✅ GOOD: Suitable for real-time applications")
            else:
                print("⚠️ Limited performance")

    def cleanup(self):
        """Clean up resources."""
        if self.cap:
            self.cap.release()
        cv2.destroyAllWindows()

def main():
    """Main function."""
    print("🤖 Auto Camera Tuner")
    print("="*30)
    print("Automatically finds and saves the best camera settings")

    try:
        tuner = AutoCameraTuner(camera_id=2)

        # Run automatic tuning
        best_config = tuner.run_auto_tune()

        if best_config:
            # Test the best configuration
            tuner.test_best_config()

            print(f"\n🎉 Auto-tuning complete!")
            print(f"Run './apply_best_camera_config.sh' to apply the best settings")

    except Exception as e:
        print(f"❌ Error: {e}")
    finally:
        if 'tuner' in locals():
            tuner.cleanup()

if __name__ == "__main__":
    main()