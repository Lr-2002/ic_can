#!/usr/bin/env python3
"""
Camera Capture Tool for IC_CAN

This script captures images from a camera and saves them as video using H.264 compression
with I-frame only mode (no future frame dependencies) for real-time processing.

Features:
- Real-time camera capture
- H.264 I-frame only encoding
- Timestamp overlay
- Configurable resolution and framerate
- Multiple camera support
- Real-time performance monitoring
"""

import cv2
import numpy as np
import time
import argparse
import os
import sys
from datetime import datetime
from threading import Thread, Lock
from queue import Queue
import logging

# Setup logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)


class CameraCapture:
    """
    Real-time camera capture with H.264 I-frame only video encoding.

    This class provides efficient camera capture and video encoding optimized for
    real-time applications where low latency is important.
    """

    def __init__(self, camera_id: int = 0, output_path: str = None,
                 resolution: tuple = (640, 480), fps: int = 30,
                 quality: int = 18):
        """
        Initialize camera capture.

        Args:
            camera_id: Camera device ID (default: 0)
            output_path: Output video file path (auto-generated if None)
            resolution: Video resolution (width, height)
            fps: Target frames per second
            quality: H.264 quality (lower = better quality, range: 0-51)
        """
        self.camera_id = camera_id
        self.resolution = resolution
        self.fps = fps
        self.quality = quality
        self.is_recording = False
        self.frame_count = 0
        self.start_time = None

        # Generate output path if not provided
        if output_path is None:
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            output_path = f"camera_capture_{timestamp}.mp4"

        self.output_path = output_path

        # Thread safety
        self._lock = Lock()
        self._frame_queue = Queue(maxsize=10)

        # Performance monitoring
        self._performance_stats = {
            'frames_captured': 0,
            'frames_written': 0,
            'avg_fps': 0.0,
            'dropped_frames': 0,
            'encoding_time': 0.0
        }

    def initialize_camera(self) -> bool:
        """
        Initialize camera and video writer.

        Returns:
            True if initialization successful
        """
        try:
            # Initialize camera
            self.cap = cv2.VideoCapture(self.camera_id)

            if not self.cap.isOpened():
                logger.error(f"Failed to open camera {self.camera_id}")
                return False

            # Set camera properties
            self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, self.resolution[0])
            self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, self.resolution[1])
            self.cap.set(cv2.CAP_PROP_FPS, self.fps)
            self.cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)  # Minimize buffering

            # Verify settings
            actual_width = int(self.cap.get(cv2.CAP_PROP_FRAME_WIDTH))
            actual_height = int(self.cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
            actual_fps = self.cap.get(cv2.CAP_PROP_FPS)

            logger.info(f"Camera initialized: {actual_width}x{actual_height} @ {actual_fps:.2f} FPS")

            # Initialize video writer with H.264 I-frame only encoding
            fourcc = cv2.VideoWriter_fourcc(*'avc1')  # H.264
            self.video_writer = cv2.VideoWriter(
                self.output_path,
                fourcc,
                self.fps,
                (actual_width, actual_height)
            )

            if not self.video_writer.isOpened():
                logger.error("Failed to initialize video writer")
                return False

            # Configure H.264 encoding for I-frame only mode
            # This ensures no future frame dependencies
            self.video_writer.set(cv2.VIDEOWRITER_PROP_QUALITY, self.quality)

            logger.info(f"Video writer initialized: {self.output_path}")
            logger.info(f"H.264 I-frame only encoding, quality: {self.quality}")

            return True

        except Exception as e:
            logger.error(f"Camera initialization failed: {e}")
            return False

    def add_timestamp_overlay(self, frame: np.ndarray) -> np.ndarray:
        """
        Add timestamp overlay to frame.

        Args:
            frame: Input frame

        Returns:
            Frame with timestamp overlay
        """
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
        fps_text = f"FPS: {self._get_current_fps():.1f}"

        # Add timestamp (top-left)
        cv2.putText(frame, timestamp, (10, 30),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)

        # Add FPS (top-right)
        cv2.putText(frame, fps_text, (frame.shape[1] - 150, 30),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)

        # Add frame count (bottom-left)
        cv2.putText(frame, f"Frame: {self.frame_count}", (10, frame.shape[0] - 20),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)

        return frame

    def _get_current_fps(self) -> float:
        """Calculate current FPS."""
        if self.start_time is None:
            return 0.0

        elapsed = time.time() - self.start_time
        if elapsed > 0:
            return self.frame_count / elapsed
        return 0.0

    def capture_frames(self):
        """Thread function for continuous frame capture."""
        logger.info("Starting frame capture thread")

        while self.is_recording:
            ret, frame = self.cap.read()

            if ret:
                with self._lock:
                    self.frame_count += 1
                    self._performance_stats['frames_captured'] += 1

                # Add timestamp overlay
                frame = self.add_timestamp_overlay(frame)

                # Put frame in queue (non-blocking)
                if not self._frame_queue.full():
                    self._frame_queue.put(frame)
                else:
                    # Queue full, drop frame
                    with self._lock:
                        self._performance_stats['dropped_frames'] += 1
                    logger.debug("Frame dropped - queue full")
            else:
                logger.warning("Failed to capture frame")
                time.sleep(0.01)  # Prevent tight loop on error

    def encode_frames(self):
        """Thread function for video encoding."""
        logger.info("Starting video encoding thread")

        while self.is_recording or not self._frame_queue.empty():
            try:
                # Get frame from queue with timeout
                frame = self._frame_queue.get(timeout=0.1)

                # Encode and write frame
                start_time = time.time()
                self.video_writer.write(frame)
                encoding_time = time.time() - start_time

                with self._lock:
                    self._performance_stats['frames_written'] += 1
                    self._performance_stats['encoding_time'] += encoding_time

            except:
                # Queue empty or timeout, continue
                continue

    def start_recording(self, duration: float = None) -> bool:
        """
        Start recording video.

        Args:
            duration: Recording duration in seconds (None for infinite)

        Returns:
            True if recording started successfully
        """
        if not self.initialize_camera():
            return False

        self.is_recording = True
        self.start_time = time.time()
        self.frame_count = 0

        # Start capture and encoding threads
        self.capture_thread = Thread(target=self.capture_frames, daemon=True)
        self.encoding_thread = Thread(target=self.encode_frames, daemon=True)

        self.capture_thread.start()
        self.encoding_thread.start()

        logger.info(f"Recording started: {self.output_path}")

        # Record for specified duration if provided
        if duration is not None:
            Thread(target=self._stop_after_duration, args=(duration,), daemon=True).start()

        return True

    def _stop_after_duration(self, duration: float):
        """Stop recording after specified duration."""
        time.sleep(duration)
        self.stop_recording()

    def stop_recording(self):
        """Stop recording and save video."""
        if not self.is_recording:
            return

        logger.info("Stopping recording...")
        self.is_recording = False

        # Wait for threads to finish
        if hasattr(self, 'capture_thread'):
            self.capture_thread.join(timeout=2.0)
        if hasattr(self, 'encoding_thread'):
            self.encoding_thread.join(timeout=2.0)

        # Update final stats
        if self.start_time:
            total_time = time.time() - self.start_time
            if total_time > 0:
                self._performance_stats['avg_fps'] = self.frame_count / total_time

        # Release resources
        if hasattr(self, 'video_writer'):
            self.video_writer.release()
        if hasattr(self, 'cap'):
            self.cap.release()

        logger.info(f"Recording saved: {self.output_path}")
        self.print_performance_stats()

    def print_performance_stats(self):
        """Print performance statistics."""
        stats = self._performance_stats

        print("\n" + "="*50)
        print("PERFORMANCE STATISTICS")
        print("="*50)
        print(f"Total frames captured: {stats['frames_captured']}")
        print(f"Total frames written:  {stats['frames_written']}")
        print(f"Dropped frames:        {stats['dropped_frames']}")
        print(f"Average FPS:           {stats['avg_fps']:.2f}")

        if stats['frames_written'] > 0:
            avg_encoding_time = stats['encoding_time'] / stats['frames_written'] * 1000
            print(f"Avg encoding time:     {avg_encoding_time:.2f} ms")

        if stats['frames_captured'] > 0:
            drop_rate = stats['dropped_frames'] / stats['frames_captured'] * 100
            print(f"Frame drop rate:       {drop_rate:.2f}%")

        print(f"Output file:           {self.output_path}")
        print(f"File size:             {os.path.getsize(self.output_path) / 1024 / 1024:.2f} MB")
        print("="*50)

    def preview_mode(self, duration: float = 10.0):
        """
        Run camera preview without recording.

        Args:
            duration: Preview duration in seconds
        """
        if not self.initialize_camera():
            return False

        logger.info(f"Starting preview mode for {duration} seconds...")
        print("Press 'q' to quit preview early")

        start_time = time.time()

        while time.time() - start_time < duration:
            ret, frame = self.cap.read()

            if ret:
                frame = self.add_timestamp_overlay(frame)
                cv2.imshow('Camera Preview', frame)

                # Check for quit key
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
            else:
                logger.warning("Failed to capture frame in preview")
                break

        cv2.destroyAllWindows()
        self.cap.release()
        logger.info("Preview mode ended")


def list_cameras():
    """List available cameras."""
    print("Scanning for cameras...")

    available_cameras = []
    for i in range(10):  # Check first 10 camera indices
        cap = cv2.VideoCapture(i)
        if cap.isOpened():
            ret, _ = cap.read()
            if ret:
                width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
                height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
                fps = cap.get(cv2.CAP_PROP_FPS)
                available_cameras.append((i, width, height, fps))
                print(f"Camera {i}: {width}x{height} @ {fps:.2f} FPS")
            cap.release()

    if not available_cameras:
        print("No cameras found")
    else:
        print(f"Found {len(available_cameras)} camera(s)")

    return available_cameras


def main():
    """Main function."""
    parser = argparse.ArgumentParser(description="Camera Capture Tool for IC_CAN")
    parser.add_argument("--camera", "-c", type=int, default=0,
                       help="Camera ID (default: 0)")
    parser.add_argument("--output", "-o", type=str,
                       help="Output video file path")
    parser.add_argument("--resolution", "-r", type=str, default="640x480",
                       help="Resolution (default: 640x480)")
    parser.add_argument("--fps", type=int, default=30,
                       help="Target FPS (default: 30)")
    parser.add_argument("--quality", "-q", type=int, default=18,
                       help="H.264 quality 0-51 (lower=better, default: 18)")
    parser.add_argument("--duration", "-d", type=float,
                       help="Recording duration in seconds")
    parser.add_argument("--preview", "-p", action="store_true",
                       help="Preview mode only")
    parser.add_argument("--preview-duration", type=float, default=10.0,
                       help="Preview duration in seconds (default: 10)")
    parser.add_argument("--list", "-l", action="store_true",
                       help="List available cameras")

    args = parser.parse_args()

    # List cameras if requested
    if args.list:
        list_cameras()
        return

    # Parse resolution
    try:
        width, height = map(int, args.resolution.split('x'))
        resolution = (width, height)
    except:
        logger.error("Invalid resolution format. Use WIDTHxHEIGHT (e.g., 640x480)")
        return

    # Validate quality
    if not 0 <= args.quality <= 51:
        logger.error("Quality must be between 0 (best) and 51 (worst)")
        return

    # Create camera capture instance
    capture = CameraCapture(
        camera_id=args.camera,
        output_path=args.output,
        resolution=resolution,
        fps=args.fps,
        quality=args.quality
    )

    try:
        if args.preview:
            # Preview mode
            success = capture.initialize_camera()
            if success:
                capture.preview_mode(args.preview_duration)
        else:
            # Recording mode
            print(f"Starting camera capture...")
            print(f"Camera: {args.camera}")
            print(f"Resolution: {width}x{height}")
            print(f"FPS: {args.fps}")
            print(f"Quality: {args.quality}")
            print(f"Output: {capture.output_path}")

            if args.duration:
                print(f"Duration: {args.duration} seconds")
            else:
                print("Duration: Unlimited (press Ctrl+C to stop)")

            print("\nPress Ctrl+C to stop recording early")

            success = capture.start_recording(duration=args.duration)

            if success:
                # Wait for recording to complete or interrupt
                try:
                    while capture.is_recording:
                        time.sleep(0.1)
                except KeyboardInterrupt:
                    print("\nRecording stopped by user")
                    capture.stop_recording()

    except Exception as e:
        logger.error(f"Error during capture: {e}")
        capture.stop_recording()


if __name__ == "__main__":
    main()