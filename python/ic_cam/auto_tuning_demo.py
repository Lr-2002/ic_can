#!/usr/bin/env python3
"""
Auto Tuning Demo - Automatic camera tuning demonstration
"""

import cv2
import numpy as np
import json
import subprocess
import time
import os
from datetime import datetime

class AutoTuningDemo:
    def __init__(self, camera_id=2):
        self.camera_id = camera_id
        self.device_path = f'/dev/video{camera_id}'

        # Camera control ranges
        self.controls = {
            'brightness': {'min': -64, 'max': 64, 'default': 0, 'value': 0},
            'contrast': {'min': 0, 'max': 100, 'default': 35, 'value': 80},
            'saturation': {'min': 0, 'max': 100, 'default': 64, 'value': 80},
            'gamma': {'min': 100, 'max': 500, 'default': 300, 'value': 150},
            'white_balance_temperature': {'min': 2800, 'max': 6500, 'default': 4600, 'value': 5000},
        }

        # Results tracking
        self.configs_tested = []
        self.best_config = None
        self.best_fps = 0

        # Session directory
        self.session_dir = f"auto_tuning_demo_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        os.makedirs(self.session_dir, exist_ok=True)

        # Initialize camera
        self.init_camera()

        print(f"🤖 Auto Tuning Demo Started")
        print(f"📁 Session: {self.session_dir}")

    def init_camera(self):
        """Initialize camera."""
        self.cap = cv2.VideoCapture(self.camera_id, cv2.CAP_V4L)
        if not self.cap.isOpened():
            raise RuntimeError(f"Failed to open camera {self.camera_id}")

        # Configure for MJPG 60Hz
        mjpg_fourcc = cv2.VideoWriter_fourcc(*'MJPG')
        self.cap.set(cv2.CAP_PROP_FOURCC, mjpg_fourcc)
        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
        self.cap.set(cv2.CAP_PROP_FPS, 60)
        self.cap.set(cv2.CAP_PROP_BUFFERSIZE, 5)

        # Set initial settings
        self.apply_all_settings()

    def apply_setting(self, control_name, value):
        """Apply a single camera setting."""
        try:
            cmd = f"v4l2-ctl -d {self.device_path} --set-ctrl={control_name}={value}"
            subprocess.run(cmd, shell=True, capture_output=True, timeout=2)
            self.controls[control_name]['value'] = value
        except Exception as e:
            print(f"❌ Failed to set {control_name}: {e}")

    def apply_all_settings(self):
        """Apply all current settings."""
        # Disable auto white balance
        subprocess.run(f"v4l2-ctl -d {self.device_path} --set-ctrl=white_balance_automatic=0",
                      shell=True, capture_output=True)

        for control_name, control_info in self.controls.items():
            self.apply_setting(control_name, control_info['value'])

    def test_performance(self, duration=3):
        """Test current camera performance."""
        frame_count = 0
        start_time = time.time()

        while time.time() - start_time < duration:
            ret, frame = self.cap.read()
            if ret:
                frame_count += 1
            else:
                break

        fps = frame_count / duration if duration > 0 else 0
        return fps

    def test_and_save_config(self, config_name, settings=None):
        """Test and save a configuration."""
        if settings:
            # Apply new settings
            for name, value in settings.items():
                self.apply_setting(name, value)
        else:
            # Use current settings
            settings = {name: info['value'] for name, info in self.controls.items()}

        # Wait for settings to settle
        time.sleep(0.5)

        # Test performance
        fps = self.test_performance()

        config = {
            'name': config_name,
            'timestamp': datetime.now().isoformat(),
            'settings': settings,
            'performance': {
                'fps': fps,
                'timestamp': datetime.now().isoformat()
            }
        }

        # Save configuration
        filename = f"{self.session_dir}/{config_name.replace(' ', '_')}_fps_{fps:.1f}.json"
        with open(filename, 'w') as f:
            json.dump(config, f, indent=2)

        # Create shell script
        script_filename = f"{self.session_dir}/apply_{config_name.replace(' ', '_')}.sh"
        with open(script_filename, 'w') as f:
            f.write("#!/bin/bash\n")
            f.write(f"# {config_name}\n")
            f.write(f"# Performance: {fps:.1f} FPS\n")
            f.write("echo 'Applying camera configuration...'\n\n")

            f.write("v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_automatic=0\n")
            for name, value in settings.items():
                f.write(f"v4l2-ctl -d /dev/video2 --set-ctrl={name}={value}\n")

        subprocess.run(f"chmod +x {script_filename}", shell=True)

        self.configs_tested.append(config)

        # Check if best
        if fps > self.best_fps:
            self.best_fps = fps
            self.best_config = config.copy()
            self.save_best_config()

        print(f"💾 {config_name}: {fps:.1f} FPS")
        return fps

    def save_best_config(self):
        """Save the best configuration."""
        if self.best_config:
            # Save as best
            best_filename = f"{self.session_dir}/BEST_CONFIG.json"
            with open(best_filename, 'w') as f:
                json.dump(self.best_config, f, indent=2)

            # Create best script
            best_script = f"{self.session_dir}/apply_BEST_CONFIG.sh"
            with open(best_script, 'w') as f:
                f.write("#!/bin/bash\n")
                f.write(f"# BEST CONFIG - {self.best_config['timestamp']}\n")
                f.write(f"# BEST FPS: {self.best_fps:.1f}\n")
                f.write("echo 'Applying BEST camera configuration...'\n\n")

                f.write("v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_automatic=0\n")
                for name, value in self.best_config['settings'].items():
                    f.write(f"v4l2-ctl -d /dev/video2 --set-ctrl={name}={value}\n")

            subprocess.run(f"chmod +x {best_script}", shell=True)

    def run_auto_tuning(self):
        """Run automatic tuning demonstration."""
        print("\n🤖 Starting Automatic Tuning Demo")
        print("="*50)
        print("Testing different camera configurations automatically")
        print()

        # Test initial configuration
        print("🔄 Testing initial configuration...")
        self.test_and_save_config("Initial_Settings")

        # Test presets
        presets = [
            ("Balanced", {
                'brightness': 0,
                'contrast': 75,
                'saturation': 85,
                'gamma': 140,
                'white_balance_temperature': 5000,
            }),
            ("Vibrant", {
                'brightness': 5,
                'contrast': 90,
                'saturation': 100,
                'gamma': 130,
                'white_balance_temperature': 5500,
            }),
            ("Natural", {
                'brightness': -5,
                'contrast': 70,
                'saturation': 75,
                'gamma': 160,
                'white_balance_temperature': 4500,
            }),
            ("High_Contrast", {
                'brightness': 0,
                'contrast': 100,
                'saturation': 90,
                'gamma': 120,
                'white_balance_temperature': 5200,
            }),
            ("Warm_Tones", {
                'brightness': 2,
                'contrast': 80,
                'saturation': 85,
                'gamma': 140,
                'white_balance_temperature': 6000,
            }),
        ]

        for name, settings in presets:
            print(f"\n🎨 Testing {name} preset...")
            self.test_and_save_config(name, settings)

        # Test systematic variations around best preset
        print(f"\n🔍 Testing systematic variations...")
        base_preset = presets[1][1]  # Start with vibrant

        for brightness_offset in [-10, 0, 10]:
            for contrast_offset in [-5, 0, 5]:
                config = base_preset.copy()
                config['brightness'] = max(-64, min(64, config['brightness'] + brightness_offset))
                config['contrast'] = max(0, min(100, config['contrast'] + contrast_offset))

                name = f"Var_B{brightness_offset:+d}_C{contrast_offset:+d}"
                self.test_and_save_config(name, config)

        # Show results
        print(f"\n📊 Tuning Results Summary")
        print("="*50)
        print(f"Total configurations tested: {len(self.configs_tested)}")
        print(f"Best performance: {self.best_fps:.1f} FPS")
        print(f"Best config: {self.best_config['name']}")
        print(f"Best settings: {self.best_config['settings']}")

        # Show top 5
        print(f"\n🏆 Top 5 Configurations:")
        sorted_configs = sorted(self.configs_tested, key=lambda x: x['performance']['fps'], reverse=True)
        for i, config in enumerate(sorted_configs[:5]):
            print(f"   {i+1}. {config['name']}: {config['performance']['fps']:.1f} FPS")

        # Save session summary
        summary = {
            'session_dir': self.session_dir,
            'timestamp': datetime.now().isoformat(),
            'total_configs': len(self.configs_tested),
            'best_fps': self.best_fps,
            'best_config': self.best_config,
            'all_configs': self.configs_tested
        }

        summary_file = f"{self.session_dir}/TUNING_SUMMARY.json"
        with open(summary_file, 'w') as f:
            json.dump(summary, f, indent=2)

        print(f"\n💾 Complete tuning data saved to: {self.session_dir}/")
        print(f"🚀 To apply best config: {self.session_dir}/apply_BEST_CONFIG.sh")

        # Apply best config
        print(f"\n✅ Applying best configuration...")
        for name, value in self.best_config['settings'].items():
            self.apply_setting(name, value)

        # Test final performance
        print(f"🎯 Testing final performance (10 seconds)...")
        final_fps = self.test_performance(10)
        print(f"📊 Final performance: {final_fps:.1f} FPS")

        return self.best_config

    def show_live_preview(self, duration=10):
        """Show live preview with current settings."""
        print(f"\n📹 Live Preview ({duration} seconds)")
        print("Press Ctrl+C to stop preview")

        start_time = time.time()
        frame_count = 0

        try:
            while time.time() - start_time < duration:
                ret, frame = self.cap.read()
                if not ret:
                    break

                frame_count += 1
                current_time = time.time()

                # Calculate FPS
                fps = frame_count / (current_time - start_time) if current_time > start_time else 0

                # Add overlay
                cv2.putText(frame, f"FPS: {fps:.1f}", (10, 30),
                           cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
                cv2.putText(frame, f"Best Config Applied", (10, 70),
                           cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
                cv2.putText(frame, f"{self.best_fps:.1f} FPS Expected", (10, 110),
                           cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

                cv2.imshow('Camera Preview - Optimized Settings', frame)

                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break

        except KeyboardInterrupt:
            print("\n⚠️ Preview stopped")

        cv2.destroyAllWindows()

    def cleanup(self):
        """Clean up resources."""
        if self.cap:
            self.cap.release()
        cv2.destroyAllWindows()

def main():
    """Main function."""
    print("🤖 Auto Tuning Demo")
    print("="*30)
    print("Automatic camera tuning with optimal configuration")

    try:
        tuner = AutoTuningDemo(camera_id=2)

        # Run automatic tuning
        best_config = tuner.run_auto_tuning()

        # Show live preview
        tuner.show_live_preview(10)

        print(f"\n🎉 Auto tuning complete!")
        print(f"🏆 Best configuration found and applied")
        print(f"📁 All data saved for future reference")

    except Exception as e:
        print(f"❌ Error: {e}")
    finally:
        if 'tuner' in locals():
            tuner.cleanup()

if __name__ == "__main__":
    main()