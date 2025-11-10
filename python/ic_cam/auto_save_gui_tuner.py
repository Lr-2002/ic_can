#!/usr/bin/env python3
"""
Auto-Save GUI Camera Tuner - Saves every configuration change automatically
"""

import cv2
import numpy as np
import json
import subprocess
import time
from datetime import datetime
import os

class AutoSaveGUITuner:
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

        # Auto-save tracking
        self.config_counter = 0
        self.best_config = None
        self.best_fps = 0
        self.auto_save_dir = "camera_configs"

        # Create auto-save directory
        os.makedirs(self.auto_save_dir, exist_ok=True)

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

        # Apply initial settings
        self.apply_all_settings()

        # Performance tracking
        self.frame_count = 0
        self.start_time = time.time()
        self.last_fps_time = time.time()
        self.current_fps = 0
        self.fps_history = []

        print("✅ Auto-Save GUI Camera Tuner Initialized")
        print(f"📁 Saving configs to: {self.auto_save_dir}/")

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

    def save_current_config(self, note=""):
        """Save current configuration with auto-save."""
        self.config_counter += 1

        # Test current performance
        fps = self.test_current_performance()

        config = {
            'config_number': self.config_counter,
            'timestamp': datetime.now().isoformat(),
            'note': note,
            'settings': {name: info['value'] for name, info in self.controls.items()},
            'performance': {
                'fps': fps,
                'timestamp': datetime.now().isoformat()
            }
        }

        # Save JSON config
        filename = f"{self.auto_save_dir}/config_{self.config_counter:04d}_fps_{fps:.1f}.json"
        with open(filename, 'w') as f:
            json.dump(config, f, indent=2)

        # Create shell script
        script_filename = f"{self.auto_save_dir}/apply_config_{self.config_counter:04d}.sh"
        with open(script_filename, 'w') as f:
            f.write("#!/bin/bash\n")
            f.write(f"# Config #{self.config_counter} - {config['timestamp']}\n")
            f.write(f"# FPS: {fps:.1f} - {note}\n")
            f.write("echo 'Applying camera configuration...'\n\n")

            f.write("v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_automatic=0\n")
            for name, value in config['settings'].items():
                f.write(f"v4l2-ctl -d /dev/video2 --set-ctrl={name}={value}\n")

        subprocess.run(f"chmod +x {script_filename}", shell=True)

        # Check if this is the best config
        if fps > self.best_fps:
            self.best_fps = fps
            self.best_config = config.copy()
            self.save_best_config()

        print(f"💾 Saved Config #{self.config_counter}: {fps:.1f} FPS - {note}")
        return filename

    def save_best_config(self):
        """Save the current best configuration."""
        if self.best_config:
            # Save as latest best
            best_filename = f"{self.auto_save_dir}/BEST_CONFIG.json"
            with open(best_filename, 'w') as f:
                json.dump(self.best_config, f, indent=2)

            # Create best script
            best_script = f"{self.auto_save_dir}/apply_BEST_CONFIG.sh"
            with open(best_script, 'w') as f:
                f.write("#!/bin/bash\n")
                f.write(f"# BEST CONFIG - {self.best_config['timestamp']}\n")
                f.write(f"# BEST FPS: {self.best_fps:.1f}\n")
                f.write("echo 'Applying BEST camera configuration...'\n\n")

                f.write("v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_automatic=0\n")
                for name, value in self.best_config['settings'].items():
                    f.write(f"v4l2-ctl -d /dev/video2 --set-ctrl={name}={value}\n")

            subprocess.run(f"chmod +x {best_script}", shell=True)

    def test_current_performance(self, duration=2):
        """Test current camera performance."""
        frame_count = 0
        start_time = time.time()

        while time.time() - start_time < duration:
            ret, frame = self.cap.read()
            if ret:
                frame_count += 1

        fps = frame_count / duration if duration > 0 else 0
        return fps

    def create_trackbars(self):
        """Create trackbars for all camera controls."""
        cv2.namedWindow("Camera Controls", cv2.WINDOW_NORMAL)
        cv2.resizeWindow("Camera Controls", 400, 300)

        for control_name, control_info in self.controls.items():
            trackbar_name = f"{control_name}"
            cv2.createTrackbar(trackbar_name, "Camera Controls",
                              control_info['value'] - control_info['min'],  # Normalize to 0-based
                              control_info['max'] - control_info['min'],
                              lambda val, name=control_name: self.on_trackbar_change(name, val))

    def on_trackbar_change(self, control_name, trackbar_value):
        """Handle trackbar value changes with auto-save."""
        # Convert back from 0-based to actual value
        control_info = self.controls[control_name]
        actual_value = trackbar_value + control_info['min']

        # Update control value
        self.controls[control_name]['value'] = actual_value

        # Apply setting immediately
        self.apply_setting(control_name, actual_value)

        # Auto-save configuration change
        note = f"Changed {control_name} to {actual_value}"
        self.save_current_config(note)

        # Special handling for white balance
        if control_name == 'white_balance_automatic':
            if actual_value == 1:
                # When auto is enabled, reset temperature to default
                default_temp = self.controls['white_balance_temperature']['default']
                self.controls['white_balance_temperature']['value'] = default_temp
                self.apply_setting('white_balance_temperature', default_temp)

                # Update trackbar
                temp_control = self.controls['white_balance_temperature']
                temp_trackbar_value = default_temp - temp_control['min']
                cv2.setTrackbarPos("white_balance_temperature", "Camera Controls", temp_trackbar_value)

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
            self.fps_history.append(self.current_fps)
            if len(self.fps_history) > 10:
                self.fps_history.pop(0)
            self.frame_count = 0
            self.last_fps_time = current_time

        return frame

    def add_info_overlay(self, frame):
        """Add information overlay to frame."""
        h, w = frame.shape[:2]

        # Create semi-transparent overlay for info
        overlay = frame.copy()
        cv2.rectangle(overlay, (10, 10), (450, 300), (0, 0, 0), -1)
        frame = cv2.addWeighted(frame, 1, overlay, 0.7, 0)

        # FPS info
        avg_fps = np.mean(self.fps_history) if self.fps_history else 0
        fps_color = (0, 255, 0) if avg_fps >= 50 else (0, 255, 255) if avg_fps >= 30 else (0, 0, 255)

        cv2.putText(frame, f"FPS: {self.current_fps:.1f}", (20, 40),
                   cv2.FONT_HERSHEY_SIMPLEX, 1, fps_color, 2)
        cv2.putText(frame, f"Avg: {avg_fps:.1f}", (20, 70),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.8, fps_color, 2)

        # Current settings
        y_offset = 110
        for control_name in ['brightness', 'contrast', 'saturation', 'gamma']:
            value = self.controls[control_name]['value']
            cv2.putText(frame, f"{control_name}: {value}", (20, y_offset),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 1)
            y_offset += 25

        # White balance info
        wb_auto = self.controls['white_balance_automatic']['value']
        wb_temp = self.controls['white_balance_temperature']['value']
        wb_text = "Auto" if wb_auto else f"{wb_temp}K"
        cv2.putText(frame, f"White Balance: {wb_text}", (20, y_offset),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 1)

        # Auto-save info
        y_offset += 30
        cv2.putText(frame, f"Configs Saved: {self.config_counter}", (20, y_offset),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 1)

        if self.best_config:
            y_offset += 25
            cv2.putText(frame, f"Best FPS: {self.best_fps:.1f}", (20, y_offset),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 1)

        # Instructions
        instructions = [
            "🎛️ Adjust sliders to tune camera",
            "💾 All changes auto-saved!",
            f"📁 Configs in: {self.auto_save_dir}/",
            "Press 'q' to quit, 'b' to apply best",
        ]

        y_offset = h - 100
        for instruction in instructions:
            cv2.putText(frame, instruction, (20, y_offset),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)
            y_offset += 20

        return frame

    def apply_best_config(self):
        """Apply the best configuration found so far."""
        if self.best_config:
            print(f"🏆 Applying best config: {self.best_fps:.1f} FPS")

            for control_name, value in self.best_config['settings'].items():
                self.controls[control_name]['value'] = value
                self.apply_setting(control_name, value)

                # Update trackbar
                control_info = self.controls[control_name]
                trackbar_value = value - control_info['min']
                cv2.setTrackbarPos(control_name, "Camera Controls", trackbar_value)

            self.save_current_config("Applied BEST config")

    def run(self):
        """Run the auto-save GUI tuner."""
        print("🎥 Auto-Save GUI Camera Tuner")
        print("="*40)
        print("💾 Every slider change is automatically saved!")
        print("🏆 Best configuration is tracked and saved")
        print("🎛️ Adjust sliders to find optimal settings")

        # Save initial configuration
        self.save_current_config("Initial settings")

        # Create trackbars
        self.create_trackbars()

        try:
            while True:
                frame = self.get_frame()
                if frame is None:
                    print("❌ Failed to get frame")
                    break

                # Add info overlay
                display_frame = self.add_info_overlay(frame)

                # Show frame
                cv2.imshow('Auto-Save Camera Tuner', display_frame)

                # Handle key presses
                key = cv2.waitKey(1) & 0xFF

                if key == ord('q'):
                    print("👋 Quitting...")
                    break
                elif key == ord('b'):
                    self.apply_best_config()
                elif key == ord('r'):
                    # Reset to defaults
                    for control_name, control_info in self.controls.items():
                        self.controls[control_name]['value'] = control_info['default']
                        self.apply_setting(control_name, control_info['default'])

                        # Update trackbar
                        trackbar_value = control_info['default'] - control_info['min']
                        cv2.setTrackbarPos(control_name, "Camera Controls", trackbar_value)

                    self.save_current_config("Reset to defaults")

        except KeyboardInterrupt:
            print("\n⚠️ Interrupted")

        finally:
            self.cleanup()

    def cleanup(self):
        """Clean up resources."""
        if self.cap:
            self.cap.release()
        cv2.destroyAllWindows()

        print(f"\n📊 Final Summary:")
        print(f"   Total configs saved: {self.config_counter}")
        print(f"   Best FPS achieved: {self.best_fps:.1f}")
        print(f"   All configs saved in: {self.auto_save_dir}/")
        print(f"   Run './{self.auto_save_dir}/apply_BEST_CONFIG.sh' to apply best settings")

def main():
    """Main function."""
    try:
        tuner = AutoSaveGUITuner(camera_id=2)
        tuner.run()
    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    main()