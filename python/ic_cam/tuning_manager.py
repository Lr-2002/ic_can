#!/usr/bin/env python3
"""
Camera Tuning Manager - Handle the complete camera tuning process
Robust system without GUI threading issues
"""

import cv2
import numpy as np
import json
import subprocess
import time
import os
from datetime import datetime
import signal
import sys

class CameraTuningManager:
    def __init__(self, camera_id=2):
        self.camera_id = camera_id
        self.device_path = f'/dev/video{camera_id}'

        # Camera control ranges
        self.controls = {
            'brightness': {'min': -64, 'max': 64, 'default': 0, 'value': 0, 'step': 1},
            'contrast': {'min': 0, 'max': 100, 'default': 35, 'value': 80, 'step': 1},
            'saturation': {'min': 0, 'max': 100, 'default': 64, 'value': 80, 'step': 1},
            'gamma': {'min': 100, 'max': 500, 'default': 300, 'value': 150, 'step': 1},
            'white_balance_automatic': {'min': 0, 'max': 1, 'default': 1, 'value': 0, 'step': 1},
            'white_balance_temperature': {'min': 2800, 'max': 6500, 'default': 4600, 'value': 5000, 'step': 10},
        }

        # Tuning session data
        self.session_dir = f"tuning_session_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        os.makedirs(self.session_dir, exist_ok=True)

        # Results tracking
        self.configs_tested = []
        self.best_config = None
        self.best_fps = 0

        # Initialize camera
        self.init_camera()

        print(f"🎯 Camera Tuning Manager Initialized")
        print(f"📁 Session directory: {self.session_dir}")

    def init_camera(self):
        """Initialize camera with MJPG format."""
        self.cap = cv2.VideoCapture(self.camera_id, cv2.CAP_V4L)
        if not self.cap.isOpened():
            raise RuntimeError(f"Failed to open camera {self.camera_id}")

        # Configure for MJPG 60Hz
        mjpg_fourcc = cv2.VideoWriter_fourcc(*'MJPG')
        self.cap.set(cv2.CAP_PROP_FOURCC, mjpg_fourcc)
        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
        self.cap.set(cv2.CAP_PROP_FPS, 60)
        self.cap.set(cv2.CAP_PROP_BUFFERSIZE, 5)

        # Apply initial settings
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
        self.apply_setting('white_balance_automatic', 0)  # Disable auto first
        for control_name, control_info in self.controls.items():
            if control_name != 'white_balance_automatic':
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

    def save_config(self, note=""):
        """Save current configuration with performance test."""
        config_num = len(self.configs_tested) + 1

        # Test performance
        fps = self.test_performance()

        config = {
            'config_number': config_num,
            'timestamp': datetime.now().isoformat(),
            'note': note,
            'settings': {name: info['value'] for name, info in self.controls.items()},
            'performance': {
                'fps': fps,
                'timestamp': datetime.now().isoformat()
            }
        }

        # Save JSON
        filename = f"{self.session_dir}/config_{config_num:03d}_fps_{fps:.1f}.json"
        with open(filename, 'w') as f:
            json.dump(config, f, indent=2)

        # Create shell script
        script_filename = f"{self.session_dir}/apply_config_{config_num:03d}.sh"
        with open(script_filename, 'w') as f:
            f.write("#!/bin/bash\n")
            f.write(f"# Config #{config_num} - {note}\n")
            f.write(f"# Performance: {fps:.1f} FPS\n")
            f.write("echo 'Applying camera configuration...'\n\n")

            f.write("v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_automatic=0\n")
            for name, value in config['settings'].items():
                f.write(f"v4l2-ctl -d /dev/video2 --set-ctrl={name}={value}\n")

        subprocess.run(f"chmod +x {script_filename}", shell=True)

        self.configs_tested.append(config)

        # Check if best
        if fps > self.best_fps:
            self.best_fps = fps
            self.best_config = config.copy()
            self.save_best_config()

        print(f"💾 Saved Config #{config_num}: {fps:.1f} FPS - {note}")
        return config

    def save_best_config(self):
        """Save the current best configuration."""
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

    def interactive_tuning(self):
        """Interactive tuning with step-by-step adjustments."""
        print("\n🎛️ Interactive Camera Tuning")
        print("="*40)
        print("Adjust settings step by step")
        print("Every change is automatically tested and saved!")
        print()

        # Save initial config
        self.save_config("Initial settings")

        while True:
            print("\n" + "="*50)
            print("Current Settings:")
            for name, info in self.controls.items():
                current = info['value']
                range_str = f"({info['min']}-{info['max']})"
                print(f"  {name:25}: {current:6} {range_str}")

            if self.best_config:
                print(f"\n🏆 Best so far: {self.best_fps:.1f} FPS")
                print("Press 'b' to apply best config")

            print("\nOptions:")
            print("1-6: Adjust specific setting")
            print("p: Apply preset (balanced)")
            print("v: Apply preset (vibrant)")
            print("n: Apply preset (natural)")
            print("b: Apply best config found")
            print("r: Reset to defaults")
            print("t: Test current performance")
            print("s: Show summary")
            print("q: Quit and save session")

            choice = input("\nYour choice: ").strip().lower()

            if choice == 'q':
                break
            elif choice == '1':
                self.adjust_setting_interactive('brightness')
            elif choice == '2':
                self.adjust_setting_interactive('contrast')
            elif choice == '3':
                self.adjust_setting_interactive('saturation')
            elif choice == '4':
                self.adjust_setting_interactive('gamma')
            elif choice == '5':
                self.toggle_auto_wb()
            elif choice == '6':
                self.adjust_setting_interactive('white_balance_temperature')
            elif choice == 'p':
                self.apply_preset_balanced()
            elif choice == 'v':
                self.apply_preset_vibrant()
            elif choice == 'n':
                self.apply_preset_natural()
            elif choice == 'b':
                self.apply_best_found()
            elif choice == 'r':
                self.reset_to_defaults()
            elif choice == 't':
                self.test_current_settings()
            elif choice == 's':
                self.show_summary()
            else:
                print("❌ Invalid choice")

    def adjust_setting_interactive(self, control_name):
        """Interactive setting adjustment."""
        info = self.controls[control_name]
        print(f"\n🎛️ Adjusting {control_name}")
        print(f"   Range: {info['min']} to {info['max']}")
        print(f"   Current: {info['value']}")
        print(f"   Step: {info['step']}")

        while True:
            try:
                user_input = input(f"   Enter new value (or 'q' to finish): ").strip()
                if user_input.lower() == 'q':
                    break

                new_value = int(user_input)
                if info['min'] <= new_value <= info['max']:
                    old_value = info['value']
                    self.apply_setting(control_name, new_value)
                    note = f"Changed {control_name} from {old_value} to {new_value}"
                    self.save_config(note)
                    break
                else:
                    print(f"   ❌ Value must be between {info['min']} and {info['max']}")

            except ValueError:
                print("   ❌ Please enter a valid number")

    def toggle_auto_wb(self):
        """Toggle auto white balance."""
        current = self.controls['white_balance_automatic']['value']
        new_value = 1 - current
        self.apply_setting('white_balance_automatic', new_value)

        if new_value == 1:
            # Reset temperature to default when auto enabled
            default_temp = self.controls['white_balance_temperature']['default']
            self.apply_setting('white_balance_temperature', default_temp)
            note = "Enabled auto white balance"
        else:
            note = "Disabled auto white balance"

        self.save_config(note)

    def apply_preset_balanced(self):
        """Apply balanced preset."""
        print("\n🎨 Applying balanced preset...")
        preset = {
            'brightness': 0,
            'contrast': 75,
            'saturation': 85,
            'gamma': 140,
            'white_balance_automatic': 0,
            'white_balance_temperature': 5000,
        }

        for name, value in preset.items():
            self.apply_setting(name, value)

        self.save_config("Applied balanced preset")

    def apply_preset_vibrant(self):
        """Apply vibrant preset."""
        print("\n🌈 Applying vibrant preset...")
        preset = {
            'brightness': 5,
            'contrast': 90,
            'saturation': 100,
            'gamma': 130,
            'white_balance_automatic': 0,
            'white_balance_temperature': 5500,
        }

        for name, value in preset.items():
            self.apply_setting(name, value)

        self.save_config("Applied vibrant preset")

    def apply_preset_natural(self):
        """Apply natural preset."""
        print("\n🌿 Applying natural preset...")
        preset = {
            'brightness': -5,
            'contrast': 70,
            'saturation': 75,
            'gamma': 160,
            'white_balance_automatic': 0,
            'white_balance_temperature': 4500,
        }

        for name, value in preset.items():
            self.apply_setting(name, value)

        self.save_config("Applied natural preset")

    def apply_best_found(self):
        """Apply the best configuration found."""
        if self.best_config:
            print(f"\n🏆 Applying best config: {self.best_fps:.1f} FPS")
            for name, value in self.best_config['settings'].items():
                self.apply_setting(name, value)
            self.save_config("Applied best config found")
        else:
            print("❌ No best config found yet")

    def reset_to_defaults(self):
        """Reset to default settings."""
        print("\n🔄 Resetting to defaults...")
        defaults = {
            'brightness': 0,
            'contrast': 35,
            'saturation': 64,
            'gamma': 300,
            'white_balance_automatic': 1,
            'white_balance_temperature': 4600,
        }

        for name, value in defaults.items():
            self.apply_setting(name, value)

        self.save_config("Reset to defaults")

    def test_current_settings(self):
        """Test current settings performance."""
        print("\n⚡ Testing current performance...")
        fps = self.test_performance(5)  # 5 second test
        print(f"📊 Current performance: {fps:.1f} FPS")

    def show_summary(self):
        """Show session summary."""
        print(f"\n📊 Session Summary ({self.session_dir})")
        print("="*50)
        print(f"Total configs tested: {len(self.configs_tested)}")

        if self.best_config:
            print(f"Best FPS: {self.best_fps:.1f}")
            print(f"Best settings: {self.best_config['settings']}")

        print(f"\nTop 5 configs:")
        sorted_configs = sorted(self.configs_tested, key=lambda x: x['performance']['fps'], reverse=True)
        for i, config in enumerate(sorted_configs[:5]):
            print(f"  {i+1}. Config #{config['config_number']}: {config['performance']['fps']:.1f} FPS - {config['note']}")

    def save_session_summary(self):
        """Save complete session summary."""
        summary = {
            'session_dir': self.session_dir,
            'timestamp': datetime.now().isoformat(),
            'total_configs': len(self.configs_tested),
            'best_fps': self.best_fps,
            'best_config': self.best_config,
            'all_configs': self.configs_tested
        }

        summary_file = f"{self.session_dir}/SESSION_SUMMARY.json"
        with open(summary_file, 'w') as f:
            json.dump(summary, f, indent=2)

        print(f"\n💾 Session summary saved to: {summary_file}")

    def cleanup(self):
        """Clean up resources."""
        if self.cap:
            self.cap.release()
        cv2.destroyAllWindows()

    def run(self):
        """Run the tuning manager."""
        try:
            # Set up signal handlers for graceful exit
            def signal_handler(signum, frame):
                print("\n🛑 Interrupted, saving session...")
                self.save_session_summary()
                self.cleanup()
                sys.exit(0)

            signal.signal(signal.SIGINT, signal_handler)
            signal.signal(signal.SIGTERM, signal_handler)

            # Run interactive tuning
            self.interactive_tuning()

            # Save final session summary
            self.save_session_summary()

            print(f"\n🎉 Tuning session complete!")
            print(f"📁 All data saved in: {self.session_dir}")
            print(f"🏆 Best performance: {self.best_fps:.1f} FPS")
            print(f"🚀 To apply best config: {self.session_dir}/apply_BEST_CONFIG.sh")

        except Exception as e:
            print(f"❌ Error: {e}")
        finally:
            self.cleanup()

def main():
    """Main function."""
    print("🎥 Camera Tuning Manager")
    print("="*30)
    print("Complete camera tuning control")

    try:
        manager = CameraTuningManager(camera_id=2)
        manager.run()
    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    main()