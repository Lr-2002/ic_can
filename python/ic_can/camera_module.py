"""
Camera Module for IC_CAN Data Collection Pipeline

This module provides a CameraDataCollector class that can be integrated
as a subsystem into the main IC_CAN data collection pipeline.

Features:
- Seamless integration with RobotArm class
- Thread-safe data collection
- Synchronized camera-robot timestamps
- Configurable video encoding
- Real-time performance monitoring
"""

import cv2
import numpy as np
import time
import os
import json
import threading
from queue import Queue, Empty
from dataclasses import dataclass, field
from typing import Optional, List, Dict, Callable
from datetime import datetime
import logging

# Setup logging
logger = logging.getLogger(__name__)


@dataclass
class CameraFrame:
    """Single camera frame with metadata."""
    timestamp: float                    # Unix timestamp (seconds)
    frame_number: int                   # Frame sequence number
    image: np.ndarray                  # Image data
    capture_latency: float = 0.0       # Time to capture frame
    encoding_latency: float = 0.0      # Time to encode frame
    file_path: Optional[str] = None    # Path to saved file (if saved)


@dataclass
class CameraConfig:
    """Camera configuration settings."""
    camera_id: int = 0
    resolution: tuple = (640, 480)
    fps: int = 30
    quality: int = 18                  # H.264 quality (0-51)
    fourcc: str = 'avc1'               # H.264 encoder
    save_frames: bool = False
    buffer_size: int = 10
    auto_exposure: float = -6.0
    focus_mode: int = 0                # 0=auto, 1=manual


class CameraDataCollector:
    """
    Camera data collection subsystem for IC_CAN pipeline.

    This class is designed to be integrated as a component in the main
    data collection system, providing synchronized camera capture
    with minimal interference to robot control.
    """

    def __init__(self, config: CameraConfig = None, output_dir: str = None):
        """
        Initialize camera data collector.

        Args:
            config: Camera configuration
            output_dir: Output directory for recordings
        """
        self.config = config or CameraConfig()
        self.output_dir = output_dir or f"camera_data_{datetime.now().strftime('%Y%m%d_%H%M%S')}"

        # Create output directory
        os.makedirs(self.output_dir, exist_ok=True)

        # Threading and synchronization
        self._lock = threading.Lock()
        self._capture_thread = None
        self._encoding_thread = None
        self._is_recording = False
        self._frame_queue = Queue(maxsize=self.config.buffer_size)
        self._data_queue = Queue(maxsize=100)

        # Camera and video writer
        self._camera = None
        self._video_writer = None
        self._video_path = None

        # Frame tracking
        self._frame_count = 0
        self._start_time = None

        # Performance monitoring
        self._stats = {
            'frames_captured': 0,
            'frames_encoded': 0,
            'frames_saved': 0,
            'avg_capture_latency': 0.0,
            'avg_encoding_latency': 0.0,
            'dropped_frames': 0,
            'total_size_mb': 0.0
        }

        # Callback for integration with main pipeline
        self._frame_callback: Optional[Callable[[CameraFrame], None]] = None

    def set_frame_callback(self, callback: Callable[[CameraFrame], None]):
        """
        Set callback function for new frames (integration point).

        Args:
            callback: Function called with each new CameraFrame
        """
        self._frame_callback = callback

    def initialize(self) -> bool:
        """
        Initialize camera and video writer.

        Returns:
            True if initialization successful
        """
        try:
            # Initialize camera
            self._camera = cv2.VideoCapture(self.config.camera_id)

            if not self._camera.isOpened():
                logger.error(f"Failed to open camera {self.config.camera_id}")
                return False

            # Configure camera
            self._camera.set(cv2.CAP_PROP_FRAME_WIDTH, self.config.resolution[0])
            self._camera.set(cv2.CAP_PROP_FRAME_HEIGHT, self.config.resolution[1])
            self._camera.set(cv2.CAP_PROP_FPS, self.config.fps)
            self._camera.set(cv2.CAP_PROP_BUFFERSIZE, 1)
            self._camera.set(cv2.CAP_PROP_AUTOFOCUS, self.config.focus_mode)

            # Try to set exposure if supported
            try:
                self._camera.set(cv2.CAP_PROP_EXPOSURE, self.config.auto_exposure)
            except:
                pass  # Not all cameras support this

            # Verify settings
            actual_width = int(self._camera.get(cv2.CAP_PROP_FRAME_WIDTH))
            actual_height = int(self._camera.get(cv2.CAP_PROP_FRAME_HEIGHT))
            actual_fps = self._camera.get(cv2.CAP_PROP_FPS)

            logger.info(f"Camera initialized: {actual_width}x{actual_height} @ {actual_fps:.2f} FPS")

            # Initialize video writer
            self._video_path = os.path.join(self.output_dir, "camera_video.mp4")
            fourcc = cv2.VideoWriter_fourcc(*self.config.fourcc)

            self._video_writer = cv2.VideoWriter(
                self._video_path, fourcc, self.config.fps, (actual_width, actual_height)
            )

            if not self._video_writer.isOpened():
                logger.error("Failed to initialize video writer")
                return False

            self._video_writer.set(cv2.VIDEOWRITER_PROP_QUALITY, self.config.quality)

            logger.info(f"Video writer initialized: {self._video_path}")
            logger.info(f"H.264 I-frame only encoding, quality: {self.config.quality}")

            return True

        except Exception as e:
            logger.error(f"Camera initialization failed: {e}")
            return False

    def _capture_loop(self):
        """Main capture loop running in separate thread."""
        logger.info("Starting camera capture loop")

        while self._is_recording:
            start_time = time.time()

            ret, frame = self._camera.read()
            if ret:
                capture_latency = time.time() - start_time

                # Create camera frame object
                camera_frame = CameraFrame(
                    timestamp=time.time(),
                    frame_number=self._frame_count,
                    image=frame.copy(),
                    capture_latency=capture_latency
                )

                # Add to queue (non-blocking)
                if not self._frame_queue.full():
                    self._frame_queue.put(camera_frame)
                else:
                    with self._lock:
                        self._stats['dropped_frames'] += 1
                    logger.debug("Frame dropped - queue full")

                # Update statistics
                with self._lock:
                    self._frame_count += 1
                    self._stats['frames_captured'] += 1
                    self._stats['avg_capture_latency'] = (
                        (self._stats['avg_capture_latency'] * (self._stats['frames_captured'] - 1) +
                         capture_latency) / self._stats['frames_captured']
                    )

                # Call integration callback if set
                if self._frame_callback:
                    try:
                        self._frame_callback(camera_frame)
                    except Exception as e:
                        logger.warning(f"Frame callback error: {e}")

            else:
                logger.warning("Failed to capture frame")
                time.sleep(0.01)  # Prevent tight loop on error

    def _encoding_loop(self):
        """Video encoding loop running in separate thread."""
        logger.info("Starting video encoding loop")

        while self._is_recording or not self._frame_queue.empty():
            try:
                camera_frame = self._frame_queue.get(timeout=0.1)
                start_time = time.time()

                # Encode and write frame
                self._video_writer.write(camera_frame.image)

                encoding_latency = time.time() - start_time
                camera_frame.encoding_latency = encoding_latency

                # Save individual frame if requested
                if self.config.save_frames:
                    frame_path = os.path.join(self.output_dir, f"frame_{camera_frame.frame_number:06d}.jpg")
                    cv2.imwrite(frame_path, camera_frame.image)
                    camera_frame.file_path = frame_path

                # Put in data queue for pipeline integration
                if not self._data_queue.full():
                    self._data_queue.put(camera_frame)

                # Update statistics
                with self._lock:
                    self._stats['frames_encoded'] += 1
                    self._stats['frames_saved'] += self.config.save_frames
                    self._stats['avg_encoding_latency'] = (
                        (self._stats['avg_encoding_latency'] * (self._stats['frames_encoded'] - 1) +
                         encoding_latency) / self._stats['frames_encoded']
                    )

            except Empty:
                continue
            except Exception as e:
                logger.error(f"Encoding error: {e}")
                continue

    def start_recording(self) -> bool:
        """
        Start camera recording.

        Returns:
            True if recording started successfully
        """
        if not self.initialize():
            return False

        self._is_recording = True
        self._start_time = time.time()
        self._frame_count = 0

        # Start threads
        self._capture_thread = threading.Thread(target=self._capture_loop, daemon=True)
        self._encoding_thread = threading.Thread(target=self._encoding_loop, daemon=True)

        self._capture_thread.start()
        self._encoding_thread.start()

        logger.info(f"Camera recording started: {self.output_dir}")
        return True

    def stop_recording(self):
        """Stop camera recording."""
        if not self._is_recording:
            return

        logger.info("Stopping camera recording...")
        self._is_recording = False

        # Wait for threads to finish
        if self._capture_thread:
            self._capture_thread.join(timeout=3.0)
        if self._encoding_thread:
            self._encoding_thread.join(timeout=3.0)

        # Release resources
        if self._video_writer:
            self._video_writer.release()
        if self._camera:
            self._camera.release()

        # Update final statistics
        if os.path.exists(self._video_path):
            self._stats['total_size_mb'] = os.path.getsize(self._video_path) / 1024 / 1024

        logger.info(f"Camera recording saved: {self._video_path}")

    def get_latest_frame(self, timeout: float = 0.1) -> Optional[CameraFrame]:
        """
        Get the latest frame from the data queue.

        Args:
            timeout: Timeout in seconds

        Returns:
            Latest CameraFrame or None if no frame available
        """
        try:
            return self._data_queue.get(timeout=timeout)
        except Empty:
            return None

    def get_statistics(self) -> Dict:
        """Get current performance statistics."""
        stats = self._stats.copy()

        if self._start_time:
            duration = time.time() - self._start_time
            if duration > 0:
                stats['duration'] = duration
                stats['effective_fps'] = self._frame_count / duration
            else:
                stats['duration'] = 0.0
                stats['effective_fps'] = 0.0

        if stats['frames_captured'] > 0:
            stats['drop_rate_percent'] = (stats['dropped_frames'] / stats['frames_captured']) * 100
        else:
            stats['drop_rate_percent'] = 0.0

        return stats

    def save_metadata(self):
        """Save recording metadata."""
        metadata = {
            'camera_config': {
                'camera_id': self.config.camera_id,
                'resolution': self.config.resolution,
                'fps': self.config.fps,
                'quality': self.config.quality,
                'fourcc': self.config.fourcc,
                'save_frames': self.config.save_frames
            },
            'recording_info': {
                'output_dir': self.output_dir,
                'start_time': self._start_time,
                'video_path': self._video_path,
                'total_frames': self._frame_count
            },
            'performance': self.get_statistics()
        }

        metadata_path = os.path.join(self.output_dir, "camera_metadata.json")
        with open(metadata_path, 'w') as f:
            json.dump(metadata, f, indent=2)


class IntegratedDataCollector:
    """
    Integrated data collector combining robot and camera data.

    This class demonstrates how to integrate the CameraDataCollector
    with the main IC_CAN RobotArm class for synchronized data collection.
    """

    def __init__(self, robot, camera_config: CameraConfig = None, output_dir: str = None):
        """
        Initialize integrated data collector.

        Args:
            robot: IC_CAN RobotArm instance
            camera_config: Camera configuration
            output_dir: Output directory
        """
        self.robot = robot
        self.output_dir = output_dir or f"integrated_data_{datetime.now().strftime('%Y%m%d_%H%M%S')}"

        # Initialize camera subsystem
        self.camera = CameraDataCollector(camera_config, self.output_dir)
        self.camera.set_frame_callback(self._on_camera_frame)

        # Data synchronization
        self._synchronized_data = []
        self._lock = threading.Lock()
        self._is_collecting = False

    def _on_camera_frame(self, camera_frame: CameraFrame):
        """
        Callback for new camera frames (integration point).

        Args:
            camera_frame: New camera frame
        """
        try:
            # Get robot state synchronized with camera frame
            robot_state = self.robot.refresh()

            # Create synchronized data entry
            synchronized_entry = {
                'timestamp': camera_frame.timestamp,
                'frame_number': camera_frame.frame_number,
                'camera_frame': camera_frame,
                'robot_state': robot_state,
                'sync_latency': time.time() - camera_frame.timestamp
            }

            # Store synchronized data
            with self._lock:
                self._synchronized_data.append(synchronized_entry)

        except Exception as e:
            logger.warning(f"Synchronization error: {e}")

    def start_collection(self) -> bool:
        """Start integrated data collection."""
        logger.info("Starting integrated data collection")

        # Enable robot
        if not self.robot.enable():
            logger.error("Failed to enable robot")
            return False

        # Start camera recording
        if not self.camera.start_recording():
            logger.error("Failed to start camera recording")
            return False

        self._is_collecting = True
        logger.info(f"Integrated data collection started: {self.output_dir}")
        return True

    def stop_collection(self):
        """Stop integrated data collection."""
        if not self._is_collecting:
            return

        logger.info("Stopping integrated data collection...")
        self._is_collecting = False

        # Stop camera recording
        self.camera.stop_recording()

        # Save synchronized data
        self._save_synchronized_data()

        # Save metadata
        self._save_metadata()

        logger.info(f"Integrated data collection saved: {self.output_dir}")

    def _save_synchronized_data(self):
        """Save synchronized robot-camera data to CSV."""
        import csv

        csv_path = os.path.join(self.output_dir, "synchronized_data.csv")

        with open(csv_path, 'w', newline='') as csvfile:
            fieldnames = [
                'timestamp', 'frame_number',
                'pos_1', 'pos_2', 'pos_3', 'pos_4', 'pos_5', 'pos_6', 'pos_7', 'pos_8', 'pos_9',
                'vel_1', 'vel_2', 'vel_3', 'vel_4', 'vel_5', 'vel_6', 'vel_7', 'vel_8', 'vel_9',
                'torque_1', 'torque_2', 'torque_3', 'torque_4', 'torque_5', 'torque_6', 'torque_7', 'torque_8', 'torque_9',
                'control_mode', 'is_safe', 'sync_latency'
            ]
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            writer.writeheader()

            with self._lock:
                for entry in self._synchronized_data:
                    robot_state = entry['robot_state']
                    row = {
                        'timestamp': entry['timestamp'],
                        'frame_number': entry['frame_number'],
                        **{f'pos_{i+1}': robot_state.positions[i] for i in range(min(9, len(robot_state.positions)))},
                        **{f'vel_{i+1}': robot_state.velocities[i] for i in range(min(9, len(robot_state.velocities)))},
                        **{f'torque_{i+1}': robot_state.torques[i] for i in range(min(9, len(robot_state.torques)))},
                        'control_mode': robot_state.control_mode.value,
                        'is_safe': robot_state.is_safe,
                        'sync_latency': entry['sync_latency']
                    }
                    writer.writerow(row)

    def _save_metadata(self):
        """Save collection metadata."""
        metadata = {
            'collection_info': {
                'output_dir': self.output_dir,
                'total_frames': len(self._synchronized_data),
                'robot_available': True
            },
            'camera_stats': self.camera.get_statistics(),
            'files': {
                'video': 'camera_video.mp4',
                'synchronized_data': 'synchronized_data.csv',
                'camera_metadata': 'camera_metadata.json'
            }
        }

        metadata_path = os.path.join(self.output_dir, "collection_metadata.json")
        with open(metadata_path, 'w') as f:
            json.dump(metadata, f, indent=2)

    def get_latest_synchronized_data(self):
        """Get the latest synchronized data entry."""
        with self._lock:
            return self._synchronized_data[-1] if self._synchronized_data else None

    def get_statistics(self):
        """Get comprehensive statistics."""
        stats = {
            'integrated_collection': {
                'synchronized_frames': len(self._synchronized_data),
                'is_collecting': self._is_collecting
            },
            'camera': self.camera.get_statistics()
        }
        return stats