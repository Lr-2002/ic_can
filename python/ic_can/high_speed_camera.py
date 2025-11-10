#!/usr/bin/env python3
"""
High-Speed Camera Module - Optimized for 60 FPS performance
Uses MJPG format and optimal buffer settings for maximum frame rate
"""

import cv2
import time
import threading
import numpy as np
from typing import Optional, Callable, Tuple
import logging

logger = logging.getLogger(__name__)

class HighSpeedCamera:
    """
    High-speed camera optimized for robotics applications.

    Uses MJPG format and optimal buffer settings to achieve 30-60 FPS.
    """

    def __init__(self, camera_id: int = 2, resolution: Tuple[int, int] = (640, 480),
                 target_fps: int = 60, buffer_size: int = 5):
        """
        Initialize high-speed camera.

        Args:
            camera_id: Camera device ID
            resolution: (width, height) resolution
            target_fps: Target frame rate
            buffer_size: Internal buffer size for smooth streaming
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

    def initialize(self) -> bool:
        """Initialize camera with optimal settings for high FPS."""
        try:
            self.cap = cv2.VideoCapture(self.camera_id)
            if not self.cap.isOpened():
                logger.error(f"Failed to open camera {self.camera_id}")
                return False

            # Set MJPG format for high performance
            mjpg_fourcc = cv2.VideoWriter_fourcc(*'MJPG')
            self.cap.set(cv2.CAP_PROP_FOURCC, mjpg_fourcc)

            # Configure resolution and FPS
            self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, self.resolution[0])
            self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, self.resolution[1])
            self.cap.set(cv2.CAP_PROP_FPS, self.target_fps)

            # Optimal buffer size for smooth streaming
            self.cap.set(cv2.CAP_PROP_BUFFERSIZE, self.buffer_size)

            # Verify settings
            actual_width = int(self.cap.get(cv2.CAP_PROP_FRAME_WIDTH))
            actual_height = int(self.cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
            actual_fps = self.cap.get(cv2.CAP_PROP_FPS)
            actual_fourcc = int(self.cap.get(cv2.CAP_PROP_FOURCC))
            actual_format = "".join([chr((actual_fourcc >> 8 * i) & 0xFF) for i in range(4)])

            logger.info(f"Camera initialized: {actual_width}x{actual_height} @ {actual_fps} FPS ({actual_format})")

            if actual_format != "MJPG":
                logger.warning(f"Camera did not accept MJPG format, got {actual_format}")

            return True

        except Exception as e:
            logger.error(f"Camera initialization failed: {e}")
            if self.cap:
                self.cap.release()
                self.cap = None
            return False

    def start_capture(self, frame_callback: Optional[Callable] = None) -> bool:
        """
        Start continuous camera capture in separate thread.

        Args:
            frame_callback: Optional callback function for each frame

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

        logger.info("High-speed camera capture started")
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

            # Keep only recent frame times for average
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

        logger.info("High-speed camera capture stopped")

    def release(self):
        """Release camera resources."""
        self.stop_capture()

        if self.cap:
            self.cap.release()
            self.cap = None

        logger.info("Camera resources released")

    def get_performance_stats(self) -> dict:
        """Get current performance statistics."""
        stats = {
            'current_fps': self.current_fps,
            'target_fps': self.target_fps,
            'resolution': self.resolution,
            'is_running': self.is_running
        }

        if self.frame_times:
            avg_frame_time = np.mean(self.frame_times) * 1000
            stats.update({
                'avg_frame_time_ms': avg_frame_time,
                'min_frame_time_ms': min(self.frame_times) * 1000,
                'max_frame_time_ms': max(self.frame_times) * 1000
            })

        return stats

    def __del__(self):
        """Cleanup on deletion."""
        self.release()


class OptimizedCameraDataCollector:
    """
    Optimized data collector using high-speed camera for robotics applications.
    """

    def __init__(self, camera_id: int = 2, output_dir: str = "data"):
        """
        Initialize optimized camera data collector.

        Args:
            camera_id: Camera device ID
            output_dir: Directory for saving recorded data
        """
        self.camera = HighSpeedCamera(camera_id)
        self.output_dir = output_dir
        self.is_recording = False
        self.video_writer = None

    def start_recording(self, filename: str, fps: int = 60) -> bool:
        """
        Start recording video with H.264 I-frame only encoding.

        Args:
            filename: Output filename
            fps: Target recording FPS

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

        # Start capture with recording callback
        def record_frame(frame, current_fps):
            if self.is_recording:
                self.video_writer.write(frame)

        success = self.camera.start_capture(record_frame)

        if success:
            logger.info(f"Started recording {filename} at {fps} FPS")

        return success

    def stop_recording(self):
        """Stop recording and save file."""
        self.is_recording = False

        if self.video_writer:
            self.video_writer.release()
            self.video_writer = None

        self.camera.stop_capture()
        self.camera.release()

        logger.info("Recording stopped and file saved")

    def get_stats(self) -> dict:
        """Get recording statistics."""
        return self.camera.get_performance_stats()


def test_high_speed_camera():
    """Test the high-speed camera implementation."""
    print("🚀 Testing High-Speed Camera Implementation")
    print("="*50)

    # Test basic high-speed camera
    camera = HighSpeedCamera(camera_id=2, buffer_size=5)

    if not camera.initialize():
        print("❌ Failed to initialize camera")
        return

    print("✅ Camera initialized successfully")

    # Test performance stats
    def frame_callback(frame, fps):
        # Add FPS overlay
        cv2.putText(frame, f"FPS: {fps:.1f}", (10, 30),
                   cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        cv2.putText(frame, "High-Speed MJPG", (10, 70),
                   cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        cv2.imshow('High-Speed Camera Test', frame)

    # Start capture
    if camera.start_capture(frame_callback):
        print("✅ High-speed capture started")

        # Run for 10 seconds
        print("📹 Running for 10 seconds...")
        try:
            for i in range(10):
                time.sleep(1)
                stats = camera.get_performance_stats()
                print(f"   Second {i+1}: {stats['current_fps']:.1f} FPS "
                      f"(avg frame time: {stats.get('avg_frame_time_ms', 0):.1f} ms)")

                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
        except KeyboardInterrupt:
            pass

        camera.stop_capture()

        # Final stats
        final_stats = camera.get_performance_stats()
        print(f"\n📊 Final Performance:")
        print(f"   Average FPS: {final_stats['current_fps']:.1f}")
        print(f"   Frame time: {final_stats.get('avg_frame_time_ms', 0):.1f} ms")

        if final_stats['current_fps'] >= 30:
            print("✅ EXCELLENT: Camera suitable for real-time robotics!")
        elif final_stats['current_fps'] >= 15:
            print("✅ GOOD: Camera usable for moderate-speed applications")
        else:
            print("⚠️ Limited performance")

    camera.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    test_high_speed_camera()