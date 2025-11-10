#!/usr/bin/env python3
"""
Camera Visualization Test for IC_CAN

This script provides a visual interface to test the camera capture system
with real-time visualization, overlays, and performance monitoring.

Features:
- Real-time camera preview
- Performance metrics overlay
- Robot state visualization
- Recording controls
- Multiple visualization modes
"""

import cv2
import numpy as np
import time
import argparse
import threading
from collections import deque
from datetime import datetime
import sys
import os

# Add build directory to path for ic_can import
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..', 'build', 'python'))

try:
    import ic_can
    from ic_can.camera_module import CameraDataCollector, CameraConfig
    IC_CAN_AVAILABLE = True
except ImportError:
    IC_CAN_AVAILABLE = False
    print("Warning: IC_CAN not available. Running camera-only test.")
    # Define fallback CameraConfig for standalone use
    from dataclasses import dataclass
    @dataclass
    class CameraConfig:
        camera_id: int = 0
        resolution: tuple = (640, 480)
        fps: int = 30
        quality: int = 18
        fourcc: str = 'avc1'
        save_frames: bool = False
        buffer_size: int = 10
        auto_exposure: float = -6.0
        focus_mode: int = 0


class VisualizationTest:
    """
    Visual test interface for camera system.
    """

    def __init__(self, camera_id: int = 0, resolution: tuple = (640, 480)):
        """
        Initialize visualization test.

        Args:
            camera_id: Camera device ID
            resolution: Camera resolution
        """
        self.camera_id = camera_id
        self.resolution = resolution
        self.is_running = False

        # Camera
        self.camera_config = CameraConfig(
            camera_id=camera_id,
            resolution=resolution,
            fps=30,
            quality=18
        )
        self.camera = None

        # Robot (if available)
        self.robot = None
        self.robot_available = False

        # Visualization data
        self.frame_buffer = deque(maxlen=30)  # Last 30 frames
        self.performance_buffer = deque(maxlen=100)
        self.recording = False

        # UI state
        self.current_mode = "preview"
        self.show_stats = True
        self.show_robot = True
        self.show_fps = True

        # Performance tracking
        self.last_frame_time = 0
        self.frame_count = 0
        self.fps = 0

    def initialize(self) -> bool:
        """
        Initialize camera and robot systems.

        Returns:
            True if initialization successful
        """
        print("🎥 Initializing Camera Visualization Test...")

        # Initialize camera
        if IC_CAN_AVAILABLE:
            self.camera = CameraDataCollector(self.camera_config)
            if not self.camera.initialize():
                print("❌ Failed to initialize camera")
                return False
            print(f"✅ Camera initialized: {self.resolution[0]}x{self.resolution[1]} @ 30fps")
        else:
            # Fallback simple camera initialization
            import cv2
            self.simple_camera = cv2.VideoCapture(self.camera_id)
            if not self.simple_camera.isOpened():
                print("❌ Failed to initialize camera")
                return False
            self.simple_camera.set(cv2.CAP_PROP_FRAME_WIDTH, self.resolution[0])
            self.simple_camera.set(cv2.CAP_PROP_FRAME_HEIGHT, self.resolution[1])
            self.simple_camera.set(cv2.CAP_PROP_FPS, 30)
            print(f"✅ Camera initialized: {self.resolution[0]}x{self.resolution[1]} @ 30fps (simple mode)")

        # Initialize robot if available
        if IC_CAN_AVAILABLE:
            try:
                self.robot = ic_can.RobotArm(debug=False)
                if self.robot.initialize():
                    self.robot.enable()
                    self.robot_available = True
                    print("✅ Robot initialized and enabled")
                else:
                    print("⚠️ Robot initialization failed, camera-only mode")
            except Exception as e:
                print(f"⚠️ Robot initialization error: {e}")
                print("Running camera-only mode")
        else:
            print("⚠️ IC_CAN not available, running camera-only mode")

        return True

    def draw_performance_overlay(self, frame: np.ndarray) -> np.ndarray:
        """
        Draw performance metrics overlay on frame.

        Args:
            frame: Input frame

        Returns:
            Frame with performance overlay
        """
        overlay = frame.copy()
        h, w = overlay.shape[:2]

        # Background for stats
        cv2.rectangle(overlay, (10, 10), (350, 150), (0, 0, 0), -1)
        cv2.rectangle(overlay, (10, 10), (350, 150), (0, 255, 0), 2)

        # FPS
        color = (0, 255, 0) if self.fps > 25 else (0, 255, 255) if self.fps > 15 else (0, 0, 255)
        cv2.putText(overlay, f"FPS: {self.fps:.1f}", (20, 35),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, color, 2)

        # Frame count
        cv2.putText(overlay, f"Frame: {self.frame_count}", (20, 60),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)

        # Camera stats
        if self.camera:
            stats = self.camera.get_statistics()
            cv2.putText(overlay, f"Captured: {stats['frames_captured']}", (20, 85),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)
            cv2.putText(overlay, f"Dropped: {stats['dropped_frames']}", (20, 105),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)

            if stats.get('drop_rate_percent', 0) > 5:
                cv2.putText(overlay, f"⚠ HIGH DROP RATE", (20, 130),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 2)

        # Recording indicator
        if self.recording:
            cv2.circle(overlay, (w - 30, 30), 10, (0, 0, 255), -1)
            cv2.putText(overlay, "REC", (w - 80, 35),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 255), 2)

        return overlay

    def draw_robot_overlay(self, frame: np.ndarray) -> np.ndarray:
        """
        Draw robot state overlay on frame.

        Args:
            frame: Input frame

        Returns:
            Frame with robot overlay
        """
        if not self.robot_available:
            return frame

        overlay = frame.copy()
        h, w = overlay.shape[:2]

        try:
            # Get robot state
            state = self.robot.refresh()

            # Background for robot info
            cv2.rectangle(overlay, (w - 250, 10), (w - 10, 150), (0, 0, 0), -1)
            cv2.rectangle(overlay, (w - 250, 10), (w - 10, 150), (255, 0, 0), 2)

            # Robot status
            status_color = (0, 255, 0) if state.is_safe() else (0, 0, 255)
            cv2.putText(overlay, "ROBOT", (w - 240, 35),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)

            # Control mode
            cv2.putText(overlay, f"Mode: {state.control_mode.value[:4]}", (w - 240, 60),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, status_color, 1)

            # Safety status
            safety_text = "SAFE" if state.is_safe() else "UNSAFE"
            cv2.putText(overlay, f"Status: {safety_text}", (w - 240, 80),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, status_color, 1)

            # Joint positions (show first 3)
            if state.positions is not None and len(state.positions) >= 3:
                pos_text = f"J1: {state.positions[0]:.2f}"
                cv2.putText(overlay, pos_text, (w - 240, 105),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.4, (200, 200, 200), 1)

                pos_text = f"J2: {state.positions[1]:.2f}"
                cv2.putText(overlay, pos_text, (w - 240, 125),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.4, (200, 200, 200), 1)

                pos_text = f"J3: {state.positions[2]:.2f}"
                cv2.putText(overlay, pos_text, (w - 240, 145),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.4, (200, 200, 200), 1)

        except Exception as e:
            # Show error if robot fails
            cv2.putText(overlay, "ROBOT ERROR", (w - 240, 50),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 255), 2)
            cv2.putText(overlay, str(e)[:20], (w - 240, 75),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.4, (200, 200, 200), 1)

        return overlay

    def draw_controls_overlay(self, frame: np.ndarray) -> np.ndarray:
        """
        Draw control instructions overlay.

        Args:
            frame: Input frame

        Returns:
            Frame with controls overlay
        """
        overlay = frame.copy()
        h, w = overlay.shape[:2]

        # Controls help
        controls = [
            "Controls:",
            "SPACE - Toggle recording",
            "S - Toggle stats",
            "R - Toggle robot info",
            "F - Toggle FPS",
            "Q - Quit",
            "",
            "Current mode: " + ("RECORDING" if self.recording else "PREVIEW")
        ]

        # Draw semi-transparent background
        cv2.rectangle(overlay, (10, h - 180), (250, h - 10), (0, 0, 0), -1)
        cv2.rectangle(overlay, (10, h - 180), (250, h - 10), (255, 255, 255), 1)

        # Draw text
        for i, text in enumerate(controls):
            y_pos = h - 160 + i * 20
            color = (255, 255, 255) if i < 1 else (200, 200, 200)
            if "RECORDING" in text and self.recording:
                color = (0, 0, 255)

            cv2.putText(overlay, text, (20, y_pos),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.4, color, 1)

        return overlay

    def update_performance(self):
        """Update performance metrics."""
        current_time = time.time()
        if self.last_frame_time > 0:
            dt = current_time - self.last_frame_time
            if dt > 0:
                instant_fps = 1.0 / dt
                self.fps = 0.9 * self.fps + 0.1 * instant_fps  # Smooth FPS

        self.last_frame_time = current_time
        self.frame_count += 1

    def process_frame(self, frame: np.ndarray) -> np.ndarray:
        """
        Process a single frame with all overlays.

        Args:
            frame: Input frame

        Returns:
            Processed frame with overlays
        """
        # Update performance
        self.update_performance()

        # Add to buffer
        self.frame_buffer.append(frame.copy())

        # Apply overlays based on settings
        processed = frame.copy()

        if self.show_stats:
            processed = self.draw_performance_overlay(processed)

        if self.show_robot and self.robot_available:
            processed = self.draw_robot_overlay(processed)

        processed = self.draw_controls_overlay(processed)

        return processed

    def run_visualization_test(self):
        """Run the main visualization test loop."""
        if not self.initialize():
            return

        print("\n🎮 Starting Camera Visualization Test")
        print("Press 'q' to quit, 'SPACE' to toggle recording")

        # Start camera recording if using IC_CAN
        if IC_CAN_AVAILABLE and hasattr(self, 'camera'):
            if not self.camera.start_recording():
                print("❌ Failed to start camera recording")
                return

        self.is_running = True

        try:
            while self.is_running:
                # Get frame from camera
                if IC_CAN_AVAILABLE and hasattr(self, 'camera'):
                    camera_frame = self.camera.get_latest_frame(timeout=0.1)
                    if camera_frame:
                        frame = camera_frame.image
                else:
                    # Simple camera capture
                    ret, frame = self.simple_camera.read()
                    if not ret:
                        continue

                if frame is not None:
                    # Process frame with overlays
                    processed_frame = self.process_frame(frame)

                    # Show frame
                    cv2.imshow('IC_CAN Camera Visualization Test', processed_frame)

                # Handle keyboard input
                key = cv2.waitKey(1) & 0xFF

                if key == ord('q'):
                    print("👋 Quitting...")
                    self.is_running = False
                elif key == ord(' '):
                    self.toggle_recording()
                elif key == ord('s'):
                    self.show_stats = not self.show_stats
                    print(f"Stats overlay: {'ON' if self.show_stats else 'OFF'}")
                elif key == ord('r'):
                    self.show_robot = not self.show_robot
                    print(f"Robot overlay: {'ON' if self.show_robot else 'OFF'}")
                elif key == ord('f'):
                    self.show_fps = not self.show_fps
                    print(f"FPS display: {'ON' if self.show_fps else 'OFF'}")

                # Small delay to prevent CPU spinning
                time.sleep(0.01)

        except KeyboardInterrupt:
            print("\n👋 Interrupted by user")

        finally:
            self.cleanup()

    def toggle_recording(self):
        """Toggle recording on/off."""
        if self.recording:
            print("⏹️ Stopping recording...")
            self.recording = False
        else:
            print("⏺️ Starting recording...")
            self.recording = True

    def cleanup(self):
        """Clean up resources."""
        print("🧹 Cleaning up...")

        self.is_running = False

        if IC_CAN_AVAILABLE and hasattr(self, 'camera'):
            self.camera.stop_recording()
            print(f"✅ Camera data saved to: {self.camera.output_dir}")
        elif hasattr(self, 'simple_camera'):
            self.simple_camera.release()
            print("✅ Simple camera released")

        if IC_CAN_AVAILABLE and hasattr(self, 'robot'):
            self.robot.shutdown()
            print("✅ Robot shutdown")

        cv2.destroyAllWindows()
        print("✅ Visualization closed")

    def print_final_stats(self):
        """Print final performance statistics."""
        print("\n" + "="*50)
        print("VISUALIZATION TEST STATISTICS")
        print("="*50)

        if IC_CAN_AVAILABLE and hasattr(self, 'camera'):
            stats = self.camera.get_statistics()
            print(f"Camera frames captured: {stats['frames_captured']}")
            print(f"Camera frames encoded: {stats['frames_encoded']}")
            print(f"Camera drop rate: {stats.get('drop_rate_percent', 0):.2f}%")
            print(f"Average capture latency: {stats.get('avg_capture_latency', 0)*1000:.2f} ms")
            print(f"Output directory: {self.camera.output_dir}")
        else:
            print("Camera mode: Simple OpenCV capture")
            print("No recording statistics available in simple mode")

        print(f"Total frames displayed: {self.frame_count}")
        print(f"Average FPS: {self.fps:.2f}")
        print("="*50)


def main():
    """Main function."""
    parser = argparse.ArgumentParser(description="IC_CAN Camera Visualization Test")
    parser.add_argument("--camera", "-c", type=int, default=0,
                       help="Camera ID (default: 0)")
    parser.add_argument("--resolution", "-r", type=str, default="640x480",
                       help="Resolution (default: 640x480)")
    parser.add_argument("--list", "-l", action="store_true",
                       help="List available cameras")

    args = parser.parse_args()

    # List cameras if requested
    if args.list:
        print("📷 Scanning for cameras...")
        for i in range(5):
            cap = cv2.VideoCapture(i)
            if cap.isOpened():
                ret, _ = cap.read()
                if ret:
                    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
                    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
                    fps = cap.get(cv2.CAP_PROP_FPS)
                    print(f"  Camera {i}: {width}x{height} @ {fps:.2f} FPS")
                cap.release()
        return

    # Parse resolution
    try:
        width, height = map(int, args.resolution.split('x'))
        resolution = (width, height)
    except:
        print("❌ Invalid resolution format. Use WIDTHxHEIGHT")
        return

    print("🚀 IC_CAN Camera Visualization Test")
    print("="*40)
    print(f"Camera ID: {args.camera}")
    print(f"Resolution: {width}x{height}")
    print(f"IC_CAN Available: {'Yes' if IC_CAN_AVAILABLE else 'No'}")
    print("="*40)

    # Create and run visualization test
    viz_test = VisualizationTest(args.camera, resolution)

    try:
        viz_test.run_visualization_test()
        viz_test.print_final_stats()
    except Exception as e:
        print(f"❌ Visualization test failed: {e}")
        viz_test.cleanup()


if __name__ == "__main__":
    main()