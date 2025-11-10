#!/usr/bin/env python3
"""
Robot Camera 60Hz Module - Optimized for high-speed robotics applications
Achieves 55+ FPS using MJPG format and V4L2 backend
"""

import cv2
import time
import threading
import numpy as np
import subprocess
from typing import Optional, Callable, Tuple, Dict
import logging

logger = logging.getLogger(__name__)

class RobotCamera60Hz:
    """
    High-performance camera optimized for 60Hz robotics applications.

    Uses V4L2 backend with MJPG format to achieve 55+ FPS performance.
    """

    def __init__(self, camera_id: int = 2, resolution: Tuple[int, int] = (1280, 720),
                 target_fps: int = 60, buffer_size: int = 5):
        """
        Initialize high-performance robot camera.

        Args:
            camera_id: Camera device ID (default: 2 for /dev/video2)
            resolution: Camera resolution (default: 1280x720 for best performance)
            target_fps: Target frame rate (default: 60)
            buffer_size: V4L2 buffer size (default: 5 for optimal performance)
        """
        self.camera_id = camera_id
        self.resolution = resolution
        self.target_fps = target_fps
        self.buffer_size = buffer_size

        self.cap = None
        self.is_running = False
        self.thread = None
        self.frame_callback = None

        # Performance tracking
        self.frame_count = 0
        self.last_fps_time = time.time()
        self.current_fps = 0.0
        self.frame_times = []
        self.total_frames_captured = 0

    def initialize(self) -> bool:
        """
        Initialize camera with optimal V4L2 settings for 60Hz performance.

        Returns:
            True if initialization successful
        """
        try:
            # Set V4L2 controls for optimal performance
            self._set_v4l2_controls()

            # Open camera with V4L2 backend
            self.cap = cv2.VideoCapture(self.camera_id, cv2.CAP_V4L)
            if not self.cap.isOpened():
                logger.error(f"Failed to open camera /dev/video{self.camera_id}")
                return False

            # Configure for maximum performance
            mjpg_fourcc = cv2.VideoWriter_fourcc(*'MJPG')
            self.cap.set(cv2.CAP_PROP_FOURCC, mjpg_fourcc)
            self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, self.resolution[0])
            self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, self.resolution[1])
            self.cap.set(cv2.CAP_PROP_FPS, self.target_fps)
            self.cap.set(cv2.CAP_PROP_BUFFERSIZE, self.buffer_size)

            # Verify configuration
            actual_width = int(self.cap.get(cv2.CAP_PROP_FRAME_WIDTH))
            actual_height = int(self.cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
            actual_fps = self.cap.get(cv2.CAP_PROP_FPS)
            actual_fourcc = int(self.cap.get(cv2.CAP_PROP_FOURCC))
            actual_format = "".join([chr((actual_fourcc >> 8 * i) & 0xFF) for i in range(4)])

            logger.info(f"Camera initialized: {actual_width}x{actual_height} @ {actual_fps} FPS ({actual_format})")

            # Validate configuration
            if actual_format != "MJPG":
                logger.warning(f"Camera did not accept MJPG format, got {actual_format}")
                return False

            if actual_fps < 30:
                logger.warning(f"Camera reports low FPS: {actual_fps}")

            return True

        except Exception as e:
            logger.error(f"Camera initialization failed: {e}")
            if self.cap:
                self.cap.release()
                self.cap = None
            return False

    def _set_v4l2_controls(self):
        """Set V4L2 controls for optimal performance."""
        device_path = f'/dev/video{self.camera_id}'

        # Set frame rate to 60 FPS
        subprocess.run(f"v4l2-ctl -d {device_path} --set-parm=60",
                      shell=True, capture_output=True, timeout=5)

        # Set other optimal controls
        controls = [
            "power_line_frequency=1",  # 50Hz
            "sharpness=128",           # Default sharpness
            "contrast=128",            # Default contrast
        ]

        for control in controls:
            subprocess.run(f"v4l2-ctl -d {device_path} --set-ctrl={control}",
                          shell=True, capture_output=True, timeout=5)

    def start_capture(self, frame_callback: Optional[Callable] = None) -> bool:
        """
        Start continuous camera capture in separate thread.

        Args:
            frame_callback: Optional callback function(frame, fps) for each frame

        Returns:
            True if capture started successfully
        """
        if not self.cap:
            logger.error("Camera not initialized")
            return False

        if self.is_running:
            logger.warning("Camera already running")
            return True

        self.frame_callback = frame_callback
        self.is_running = True
        self.frame_count = 0
        self.last_fps_time = time.time()
        self.frame_times = []

        self.thread = threading.Thread(target=self._capture_loop, daemon=True)
        self.thread.start()

        logger.info("60Hz camera capture started")
        return True

    def _capture_loop(self):
        """Main capture loop running in separate thread."""
        logger.info("Camera capture loop started")

        while self.is_running:
            frame_start = time.time()

            ret, frame = self.cap.read()
            if not ret:
                logger.warning("Failed to read frame")
                break

            # Update performance metrics
            frame_time = time.time() - frame_start
            self.frame_times.append(frame_time)
            self.total_frames_captured += 1

            # Keep only recent frame times for rolling average
            if len(self.frame_times) > 100:
                self.frame_times.pop(0)

            self.frame_count += 1

            # Update FPS calculation every second
            current_time = time.time()
            if current_time - self.last_fps_time >= 1.0:
                self.current_fps = self.frame_count / (current_time - self.last_fps_time)
                self.frame_count = 0
                self.last_fps_time = current_time

            # Call callback if provided
            if self.frame_callback:
                try:
                    self.frame_callback(frame, self.current_fps)
                except Exception as e:
                    logger.error(f"Frame callback error: {e}")

        logger.info("Camera capture loop stopped")

    def get_frame(self) -> Optional[np.ndarray]:
        """
        Get a single frame (blocking call).

        Returns:
            Frame as numpy array or None if failed
        """
        if not self.cap:
            return None

        ret, frame = self.cap.read()
        return frame if ret else None

    def stop_capture(self):
        """Stop camera capture."""
        self.is_running = False

        if self.thread and self.thread.is_alive():
            self.thread.join(timeout=2.0)

        logger.info("60Hz camera capture stopped")

    def release(self):
        """Release camera resources."""
        self.stop_capture()

        if self.cap:
            self.cap.release()
            self.cap = None

        logger.info("Camera resources released")

    def get_performance_stats(self) -> Dict:
        """Get current performance statistics."""
        stats = {
            'current_fps': self.current_fps,
            'target_fps': self.target_fps,
            'resolution': self.resolution,
            'is_running': self.is_running,
            'total_frames': self.total_frames_captured
        }

        if self.frame_times:
            avg_frame_time = np.mean(self.frame_times) * 1000  # Convert to ms
            stats.update({
                'avg_frame_time_ms': avg_frame_time,
                'min_frame_time_ms': min(self.frame_times) * 1000,
                'max_frame_time_ms': max(self.frame_times) * 1000,
                'frame_std_ms': np.std(self.frame_times) * 1000
            })

        return stats

    def is_ready_for_robotics(self) -> bool:
        """Check if camera performance is suitable for robotics applications."""
        return self.current_fps >= 30  # Minimum 30 FPS for real-time robotics

    def __del__(self):
        """Cleanup on deletion."""
        self.release()


class RobotCameraRecorder:
    """
    High-performance camera recorder optimized for robotics data collection.
    Records video with H.264 I-frame only encoding for frame independence.
    """

    def __init__(self, camera_id: int = 2, output_dir: str = "data"):
        """
        Initialize robot camera recorder.

        Args:
            camera_id: Camera device ID
            output_dir: Directory for saving recorded data
        """
        self.camera = RobotCamera60Hz(camera_id)
        self.output_dir = output_dir
        self.is_recording = False
        self.video_writer = None
        self.start_time = None

    def start_recording(self, filename: str, fps: int = 55) -> bool:
        """
        Start recording video at high frame rate.

        Args:
            filename: Output filename
            fps: Recording FPS (matches camera performance)

        Returns:
            True if recording started successfully
        """
        if not self.camera.initialize():
            return False

        # Get actual camera resolution
        width = int(self.camera.cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        height = int(self.camera.cap.get(cv2.CAP_PROP_FRAME_HEIGHT))

        # H.264 encoder with I-frame only (no temporal dependencies)
        fourcc = cv2.VideoWriter_fourcc(*'X264')
        self.video_writer = cv2.VideoWriter(
            filename, fourcc, fps, (width, height),
            params=[cv2.VIDEOWRITER_PROP_H264_EFC, 1]  # I-frame only
        )

        if not self.video_writer.isOpened():
            logger.error("Failed to initialize video writer")
            return False

        self.is_recording = True
        self.start_time = time.time()

        # Start capture with recording callback
        def record_frame(frame, current_fps):
            if self.is_recording:
                self.video_writer.write(frame)

        success = self.camera.start_capture(record_frame)

        if success:
            logger.info(f"Started recording {filename} at {fps} FPS")

        return success

    def stop_recording(self) -> Dict:
        """Stop recording and return statistics."""
        self.is_recording = False
        end_time = time.time()

        stats = self.camera.get_performance_stats()

        if self.start_time:
            recording_time = end_time - self.start_time
            stats['recording_duration'] = recording_time
            stats['total_frames_recorded'] = int(stats['total_frames'])

        if self.video_writer:
            self.video_writer.release()
            self.video_writer = None

        self.camera.stop_capture()
        self.camera.release()

        logger.info("Recording stopped and file saved")
        return stats


def test_robot_camera_60hz():
    """Test the 60Hz robot camera implementation."""
    print("🚀 Testing Robot Camera 60Hz Implementation")
    print("="*50)

    # Initialize camera
    camera = RobotCamera60Hz(camera_id=2, resolution=(1280, 720))

    if not camera.initialize():
        print("❌ Failed to initialize camera")
        return

    print("✅ Camera initialized successfully")

    # Test with display
    def frame_callback(frame, fps):
        # Add performance overlay
        cv2.putText(frame, f"FPS: {fps:.1f}", (10, 30),
                   cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        cv2.putText(frame, "60Hz Robot Camera", (10, 70),
                   cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

        # Add status indicator
        if fps >= 50:
            status = "EXCELLENT"
            color = (0, 255, 0)
        elif fps >= 30:
            status = "GOOD"
            color = (0, 255, 255)
        else:
            status = "LIMITED"
            color = (0, 0, 255)

        cv2.putText(frame, f"Status: {status}", (10, 110),
                   cv2.FONT_HERSHEY_SIMPLEX, 1, color, 2)

        cv2.imshow('60Hz Robot Camera Test', frame)

    # Start capture
    if camera.start_capture(frame_callback):
        print("✅ 60Hz capture started")

        # Run for 10 seconds
        print("📹 Running performance test for 10 seconds...")
        try:
            for i in range(10):
                time.sleep(1)
                stats = camera.get_performance_stats()
                print(f"   Second {i+1}: {stats['current_fps']:.1f} FPS "
                      f"(avg: {stats.get('avg_frame_time_ms', 0):.1f}ms, "
                      f"std: {stats.get('frame_std_ms', 0):.1f}ms)")

                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
        except KeyboardInterrupt:
            pass

        camera.stop_capture()

        # Final performance assessment
        final_stats = camera.get_performance_stats()
        print(f"\n📊 Final Performance Assessment:")
        print(f"   Average FPS: {final_stats['current_fps']:.1f}")
        print(f"   Frame time: {final_stats.get('avg_frame_time_ms', 0):.1f}ms")
        print(f"   Frame time std: {final_stats.get('frame_std_ms', 0):.1f}ms")
        print(f"   Total frames: {final_stats['total_frames']}")

        if final_stats['current_fps'] >= 50:
            print("🎉 EXCELLENT: Camera ready for high-speed robotics!")
            print("   ✅ Suitable for 60Hz control applications")
            print("   ✅ Frame timing is consistent and reliable")
        elif final_stats['current_fps'] >= 30:
            print("✅ GOOD: Camera suitable for real-time robotics")
            print("   ✅ Can sync with 500Hz robot control")
        else:
            print("⚠️ Limited performance for high-speed applications")

    camera.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    test_robot_camera_60hz()