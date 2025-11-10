#!/usr/bin/env python3
"""
Simple CLI Camera Tuner - Command-line interface without GUI threading issues
"""

import cv2
import time
import json
import subprocess
from datetime import datetime

class SimpleCLITuner:
    def __init__(self, camera_id=2):
        self.camera_id = camera_id
        self.device_path = f'/dev/video{camera_id}'

        # Camera control ranges
        self.controls = {
            'brightness': {'min': -64, 'max': 64, 'current': 0, 'step': 1},
            'contrast': {'min': 0, 'max': 100, 'current': 80, 'step': 1},
            'saturation': {'min': 0, 'max': 100, 'current': 80, 'step': 1},
            'gamma': {'min': 100, 'max': 500, 'current': 150, 'step': 1},
            'white_balance_automatic': {'min': 0, 'max': 1, 'current': 0, 'step': 1},
            'white_balance_temperature': {'min': 2800, 'max': 6500, 'current': 5000, 'step': 10},
        }

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
            self.controls[control_name]['current'] = value
            print(f"   ✅ {control_name}: {value}")
        except Exception as e:
            print(f"   ❌ Failed to set {control_name}: {e}")

    def show_current_settings(self):
        """Display current camera settings."""
        print("\n📋 Current Camera Settings:")
        print("="*40)
        for control_name, info in self.controls.items():
            current = info['current']
            range_str = f"({info['min']}-{info['max']})"
            print(f"   {control_name:25}: {current:6} {range_str}")
        print("="*40)

    def show_menu(self):
        """Display the tuning menu."""
        print("\n🎥 Camera Tuner Menu:")
        print("="*30)
        print("1. Adjust brightness")
        print("2. Adjust contrast")
        print("3. Adjust saturation")
        print("4. Adjust gamma")
        print("5. Toggle auto white balance")
        print("6. Adjust white balance temperature")
        print("7. Show current settings")
        print("8. Apply preset (good colors)")
        print("9. Apply preset (vibrant)")
        print("10. Reset to defaults")
        print("11. Save configuration")
        print("12. Load configuration")
        print("13. Test camera performance")
        print("0. Quit")
        print("="*30)

    def adjust_setting(self, control_name):
        """Interactive setting adjustment."""
        info = self.controls[control_name]
        print(f"\n🎛️ Adjusting {control_name}")
        print(f"   Range: {info['min']} to {info['max']}")
        print(f"   Current: {info['current']}")
        print(f"   Step: {info['step']}")

        while True:
            try:
                user_input = input(f"   Enter new value (or 'q' to finish): ").strip()
                if user_input.lower() == 'q':
                    break

                new_value = int(user_input)
                if info['min'] <= new_value <= info['max']:
                    self.apply_setting(control_name, new_value)
                else:
                    print(f"   ❌ Value must be between {info['min']} and {info['max']}")

            except ValueError:
                print("   ❌ Please enter a valid number")

    def apply_preset_good_colors(self):
        """Apply preset for good color quality."""
        print("\n🎨 Applying 'Good Colors' preset...")
        preset = {
            'brightness': 0,
            'contrast': 75,
            'saturation': 85,
            'gamma': 140,
            'white_balance_automatic': 0,
            'white_balance_temperature': 5000,
        }

        for control_name, value in preset.items():
            self.apply_setting(control_name, value)

    def apply_preset_vibrant(self):
        """Apply preset for vibrant colors."""
        print("\n🌈 Applying 'Vibrant' preset...")
        preset = {
            'brightness': 5,
            'contrast': 90,
            'saturation': 100,
            'gamma': 130,
            'white_balance_automatic': 0,
            'white_balance_temperature': 5500,
        }

        for control_name, value in preset.items():
            self.apply_setting(control_name, value)

    def reset_to_defaults(self):
        """Reset all settings to defaults."""
        print("\n🔄 Resetting to default settings...")
        defaults = {
            'brightness': 0,
            'contrast': 35,
            'saturation': 64,
            'gamma': 300,
            'white_balance_automatic': 1,
            'white_balance_temperature': 4600,
        }

        for control_name, value in defaults.items():
            self.apply_setting(control_name, value)

    def save_configuration(self):
        """Save current configuration."""
        config = {
            'timestamp': datetime.now().isoformat(),
            'camera_id': self.camera_id,
            'settings': {name: info['current'] for name, info in self.controls.items()}
        }

        filename = f"camera_config_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
        with open(filename, 'w') as f:
            json.dump(config, f, indent=2)

        # Create shell script
        script_filename = f"apply_config_{datetime.now().strftime('%Y%m%d_%H%M%S')}.sh"
        with open(script_filename, 'w') as f:
            f.write("#!/bin/bash\n")
            f.write(f"# Camera configuration saved: {config['timestamp']}\n")
            f.write(f"echo 'Applying camera configuration...'\n\n")

            for name, value in config['settings'].items():
                f.write(f"v4l2-ctl -d {self.device_path} --set-ctrl={name}={value}\n")

        subprocess.run(f"chmod +x {script_filename}", shell=True)

        print(f"✅ Configuration saved:")
        print(f"   JSON: {filename}")
        print(f"   Script: {script_filename}")

    def load_configuration(self):
        """Load configuration from file."""
        filename = input("Enter config filename to load: ").strip()
        try:
            with open(filename, 'r') as f:
                config = json.load(f)

            for name, value in config['settings'].items():
                if name in self.controls:
                    self.apply_setting(name, value)

            print(f"✅ Configuration loaded from {filename}")

        except Exception as e:
            print(f"❌ Failed to load configuration: {e}")

    def test_performance(self):
        """Test camera performance with current settings."""
        print("\n⚡ Testing camera performance...")
        print("   Press 'q' to quit test")

        frame_count = 0
        start_time = time.time()
        last_time = start_time

        try:
            while True:
                ret, frame = self.cap.read()
                if not ret:
                    print("❌ Failed to read frame")
                    break

                frame_count += 1
                current_time = time.time()

                # Calculate FPS every second
                if current_time - last_time >= 1.0:
                    fps = 1.0 / (current_time - last_time)
                    print(f"   📊 FPS: {fps:.1f}")
                    last_time = current_time

                # Add simple overlay
                cv2.putText(frame, f"FPS: {frame_count/(current_time-start_time+0.001):.1f}", (10, 30),
                           cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

                cv2.imshow('Camera Test', frame)

                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break

        except KeyboardInterrupt:
            pass

        cv2.destroyAllWindows()

        # Final stats
        total_time = time.time() - start_time
        avg_fps = frame_count / total_time if total_time > 0 else 0

        print(f"\n📊 Performance Results:")
        print(f"   Total frames: {frame_count}")
        print(f"   Duration: {total_time:.1f}s")
        print(f"   Average FPS: {avg_fps:.1f}")

        if avg_fps >= 50:
            print("🎉 EXCELLENT: Ready for 60Hz robotics!")
        elif avg_fps >= 30:
            print("✅ GOOD: Suitable for real-time applications")
        else:
            print("⚠️ Limited performance")

    def run(self):
        """Run the CLI tuner."""
        print("🎥 Simple CLI Camera Tuner")
        print("="*40)
        print("Tune your camera settings for optimal color and performance")

        while True:
            self.show_menu()

            try:
                choice = input("\nEnter your choice (0-13): ").strip()

                if choice == '0':
                    print("👋 Goodbye!")
                    break
                elif choice == '1':
                    self.adjust_setting('brightness')
                elif choice == '2':
                    self.adjust_setting('contrast')
                elif choice == '3':
                    self.adjust_setting('saturation')
                elif choice == '4':
                    self.adjust_setting('gamma')
                elif choice == '5':
                    current = self.controls['white_balance_automatic']['current']
                    new_value = 1 - current
                    self.apply_setting('white_balance_automatic', new_value)
                elif choice == '6':
                    self.adjust_setting('white_balance_temperature')
                elif choice == '7':
                    self.show_current_settings()
                elif choice == '8':
                    self.apply_preset_good_colors()
                elif choice == '9':
                    self.apply_preset_vibrant()
                elif choice == '10':
                    self.reset_to_defaults()
                elif choice == '11':
                    self.save_configuration()
                elif choice == '12':
                    self.load_configuration()
                elif choice == '13':
                    self.test_performance()
                else:
                    print("❌ Invalid choice. Please enter 0-13.")

            except KeyboardInterrupt:
                print("\n👋 Goodbye!")
                break
            except Exception as e:
                print(f"❌ Error: {e}")

        self.cleanup()

    def cleanup(self):
        """Clean up resources."""
        if self.cap:
            self.cap.release()
        cv2.destroyAllWindows()

def main():
    """Main function."""
    try:
        tuner = SimpleCLITuner(camera_id=2)
        tuner.run()
    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    main()