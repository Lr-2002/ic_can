#!/usr/bin/env python3
"""
Camera Tuner GUI - Interactive camera settings tuner with sliders
"""

import cv2
import numpy as np
import json
import time
import subprocess
from datetime import datetime

class CameraTuner:
    def __init__(self, camera_id=2):
        self.camera_id = camera_id
        self.device_path = f'/dev/video{camera_id}'

        # Initialize camera
        self.cap = cv2.VideoCapture(camera_id, cv2.CAP_V4L)
        if not self.cap.isOpened():
            raise RuntimeError(f"Failed to open camera {camera_id}")

        # Configure for MJPG 60Hz
        mjpg_fourcc = cv2.VideoWriter_fourcc(*'MJPG')
        self.cap.set(cv2.CAP_PROP_FOURCC, mjpg_fourcc)
        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
        self.cap.set(cv2.CAP_PROP_FPS, 60)
        self.cap.set(cv2.CAP_PROP_BUFFERSIZE, 5)

        # Camera control ranges based on v4l2-ctl output
        self.controls = {
            'brightness': {'min': -64, 'max': 64, 'default': 0, 'value': 0, 'step': 1},
            'contrast': {'min': 0, 'max': 100, 'default': 35, 'value': 80, 'step': 1},
            'saturation': {'min': 0, 'max': 100, 'default': 64, 'value': 80, 'step': 1},
            'gamma': {'min': 100, 'max': 500, 'default': 300, 'value': 150, 'step': 1},
            'white_balance_automatic': {'min': 0, 'max': 1, 'default': 1, 'value': 0, 'step': 1},
            'white_balance_temperature': {'min': 2800, 'max': 6500, 'default': 4600, 'value': 5000, 'step': 10},
        }

        # Apply initial settings
        self.apply_all_settings()

        # GUI window
        self.window_name = "Camera Tuner - Press 's' to save, 'r' to reset, 'q' to quit"
        cv2.namedWindow(self.window_name, cv2.WINDOW_NORMAL)

        # Create trackbars
        self.create_trackbars()

        # Performance tracking
        self.frame_count = 0
        self.start_time = time.time()
        self.last_fps_time = time.time()
        self.current_fps = 0

    def create_trackbars(self):
        """Create trackbars for all camera controls."""
        cv2.namedWindow("Controls", cv2.WINDOW_NORMAL)

        for control_name, control_info in self.controls.items():
            trackbar_name = f"{control_name} ({control_info['min']}-{control_info['max']})"
            cv2.createTrackbar(trackbar_name, "Controls",
                              control_info['value'],
                              control_info['max'],
                              lambda val, name=control_name: self.on_trackbar_change(name, val))

    def on_trackbar_change(self, control_name, value):
        """Handle trackbar value changes."""
        # Update control value
        self.controls[control_name]['value'] = value

        # Apply setting immediately
        self.apply_setting(control_name, value)

        # Special handling for white balance
        if control_name == 'white_balance_automatic':
            # When auto is enabled, disable manual temperature
            if value == 1:
                cv2.setTrackbarPos("white_balance_temperature (2800-6500)", "Controls",
                                  self.controls['white_balance_temperature']['default'])
                self.apply_setting('white_balance_temperature',
                                 self.controls['white_balance_temperature']['default'])

    def apply_setting(self, control_name, value):
        """Apply a single camera setting."""
        try:
            cmd = f"v4l2-ctl -d {self.device_path} --set-ctrl={control_name}={value}"
            subprocess.run(cmd, shell=True, capture_output=True, timeout=2)
        except Exception as e:
            print(f"Failed to set {control_name}: {e}")

    def apply_all_settings(self):
        """Apply all current camera settings."""
        for control_name, control_info in self.controls.items():
            self.apply_setting(control_name, control_info['value'])

    def reset_to_defaults(self):
        """Reset all settings to defaults."""
        for control_name, control_info in self.controls.items():
            default_value = control_info['default']
            control_info['value'] = default_value

            # Update trackbar
            trackbar_name = f"{control_name} ({control_info['min']}-{control_info['max']})"
            cv2.setTrackbarPos(trackbar_name, "Controls", default_value)

        self.apply_all_settings()
        print("🔄 Reset to default settings")

    def save_configuration(self):
        """Save current configuration to file."""
        config = {
            'timestamp': datetime.now().isoformat(),
            'camera_id': self.camera_id,
            'settings': {}
        }

        for control_name, control_info in self.controls.items():
            config['settings'][control_name] = control_info['value']

        # Save to JSON file
        filename = f"camera_config_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
        with open(filename, 'w') as f:
            json.dump(config, f, indent=2)

        # Also save as shell script for easy application
        script_filename = f"apply_camera_config_{datetime.now().strftime('%Y%m%d_%H%M%S')}.sh"
        with open(script_filename, 'w') as f:
            f.write("#!/bin/bash\n")
            f.write(f"# Camera configuration generated on {config['timestamp']}\n")
            f.write(f"# Camera ID: {self.camera_id}\n\n")
            f.write("echo 'Applying camera configuration...'\n\n")

            for control_name, value in config['settings'].items():
                f.write(f"v4l2-ctl -d {self.device_path} --set-ctrl={control_name}={value}\n")

            f.write("\necho 'Camera configuration applied!'\n")

        # Make script executable
        subprocess.run(f"chmod +x {script_filename}", shell=True)

        print(f"✅ Configuration saved:")
        print(f"   JSON: {filename}")
        print(f"   Script: {script_filename}")
        return filename, script_filename

    def load_configuration(self, filename):
        """Load configuration from file."""
        try:
            with open(filename, 'r') as f:
                config = json.load(f)

            for control_name, value in config['settings'].items():
                if control_name in self.controls:
                    self.controls[control_name]['value'] = value

                    # Update trackbar
                    trackbar_name = f"{control_name} ({self.controls[control_name]['min']}-{self.controls[control_name]['max']})"
                    cv2.setTrackbarPos(trackbar_name, "Controls", value)

            self.apply_all_settings()
            print(f"✅ Configuration loaded from {filename}")

        except Exception as e:
            print(f"❌ Failed to load configuration: {e}")

    def get_frame(self):
        """Get a frame from camera."""
        ret, frame = self.cap.read()
        if not ret:
            return None

        # Update FPS counter
        self.frame_count += 1
        current_time = time.time()

        if current_time - self.last_fps_time >= 1.0:
            self.current_fps = self.frame_count / (current_time - self.last_fps_time)
            self.frame_count = 0
            self.last_fps_time = current_time

        return frame

    def add_info_overlay(self, frame):
        """Add information overlay to frame."""
        h, w = frame.shape[:2]

        # Create semi-transparent overlay for info
        overlay = frame.copy()
        cv2.rectangle(overlay, (10, 10), (400, 250), (0, 0, 0), -1)
        frame = cv2.addWeighted(frame, 1, overlay, 0.7, 0)

        # Add FPS info
        fps_color = (0, 255, 0) if self.current_fps >= 50 else (0, 255, 255) if self.current_fps >= 30 else (0, 0, 255)
        cv2.putText(frame, f"FPS: {self.current_fps:.1f}", (20, 40),
                   cv2.FONT_HERSHEY_SIMPLEX, 1, fps_color, 2)

        # Add current settings
        y_offset = 80
        for control_name in ['brightness', 'contrast', 'saturation', 'gamma']:
            value = self.controls[control_name]['value']
            cv2.putText(frame, f"{control_name}: {value}", (20, y_offset),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 1)
            y_offset += 25

        # Add white balance info
        wb_auto = self.controls['white_balance_automatic']['value']
        wb_temp = self.controls['white_balance_temperature']['value']
        wb_text = "Auto" if wb_auto else f"{wb_temp}K"
        cv2.putText(frame, f"White Balance: {wb_text}", (20, y_offset),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 1)

        # Add instructions
        instructions = [
            "Press 's' to save configuration",
            "Press 'r' to reset to defaults",
            "Press 'q' to quit",
            "Adjust sliders in 'Controls' window"
        ]

        y_offset = h - 100
        for instruction in instructions:
            cv2.putText(frame, instruction, (20, y_offset),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)
            y_offset += 20

        return frame

    def run(self):
        """Run the camera tuner GUI."""
        print("🎬 Camera Tuner Started")
        print("="*40)
        print("Use sliders in 'Controls' window to adjust settings")
        print("Press 's' to save, 'r' to reset, 'q' to quit")
        print("="*40)

        try:
            while True:
                frame = self.get_frame()
                if frame is None:
                    print("❌ Failed to get frame")
                    break

                # Add info overlay
                display_frame = self.add_info_overlay(frame)

                # Show frame
                cv2.imshow(self.window_name, display_frame)

                # Handle key presses
                key = cv2.waitKey(1) & 0xFF

                if key == ord('q'):
                    print("👋 Quitting camera tuner")
                    break
                elif key == ord('s'):
                    self.save_configuration()
                elif key == ord('r'):
                    self.reset_to_defaults()
                elif key == ord('h'):
                    print("\n📋 Help:")
                    print("  s - Save current configuration")
                    print("  r - Reset to defaults")
                    print("  q - Quit")
                    print("  h - Show this help")

        except KeyboardInterrupt:
            print("\n⚠️ Interrupted by user")

        finally:
            self.cleanup()

    def cleanup(self):
        """Clean up resources."""
        if self.cap:
            self.cap.release()
        cv2.destroyAllWindows()

def main():
    """Main function."""
    print("🎥 Camera Tuner GUI")
    print("="*30)

    try:
        tuner = CameraTuner(camera_id=2)
        tuner.run()
    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    main()