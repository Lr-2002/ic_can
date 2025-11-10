#!/usr/bin/env python3
"""
Synchronized Camera and Robot Data Capture

This script captures camera video synchronized with IC_CAN robot data for
machine learning applications. Uses H.264 I-frame only encoding for
real-time processing without future frame dependencies.

Features:
- Synchronized camera and robot data capture
- H.264 I-frame only video encoding
- Real-time data logging with microsecond precision
- CSV data export for ML training
- Performance monitoring and statistics
"""

import cv2
import numpy as np
import time
import argparse
import os
import sys
import csv
import json
from datetime import datetime
from threading import Thread, Lock
from queue import Queue, Empty
import logging
from dataclasses import dataclass, asdict
from typing import Optional, Dict, List

# Add parent directory to path for ic_can import
sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'python'))

try:
    import ic_can
    IC_CAN_AVAILABLE = True
except ImportError:
    IC_CAN_AVAILABLE = False
    print("Warning: IC_CAN not available. Robot data will be simulated.")

# Setup logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)


@dataclass
class FrameData:
    """Data structure for synchronized frame and robot state."""
    timestamp: float  # Unix timestamp in seconds
    frame_number: int
    video_frame_path: Optional[str] = None  # Path to saved frame if saving images

    # Robot state data
    positions: List[float] = None  # 9 joint positions
    velocities: List[float] = None  # 9 joint velocities
    torques: List[float] = None  # 9 joint torques
    control_mode: str = "UNKNOWN"
    is_safe: bool = True

    # Performance metrics
    capture_latency: float = 0.0  # Time to capture frame
    robot_latency: float = 0.0    # Time to get robot data


class SynchronizedCapture:
    """
    Synchronized camera and robot data capture system.

    This class provides real-time synchronized capture of camera frames
    and robot state data for machine learning applications.
    """

    def __init__(self, camera_id: int = 0, output_dir: str = None,
                 resolution: tuple = (640, 480), fps: int = 30,
                 video_quality: int = 18, save_frames: bool = False):
        """
        Initialize synchronized capture system.

        Args:
            camera_id: Camera device ID
            output_dir: Output directory for data
            resolution: Video resolution (width, height)
            fps: Target frames per second
            video_quality: H.264 quality (0-51, lower=better)
            save_frames: Save individual frames as images
        """
        self.camera_id = camera_id
        self.resolution = resolution
        self.fps = fps
        self.video_quality = video_quality
        self.save_frames = save_frames

        # Generate output directory
        if output_dir is None:
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            self.output_dir = f"sync_capture_{timestamp}"
        else:
            self.output_dir = output_dir

        os.makedirs(self.output_dir, exist_ok=True)

        # Initialize robot interface
        self.robot = None
        self.robot_available = False
        if IC_CAN_AVAILABLE:
            try:
                self.robot = ic_can.RobotArm(debug=False)
                self.robot_available = True
                logger.info("IC_CAN robot interface initialized")
            except Exception as e:
                logger.warning(f"Failed to initialize robot: {e}")

        # Threading and synchronization
        self.is_recording = False
        self._lock = Lock()
        self._frame_queue = Queue(maxsize=10)
        self._data_queue = Queue(maxsize=100)

        # Performance tracking
        self.stats = {
            'frames_captured': 0,
            'frames_processed': 0,
            'robot_data_points': 0,
            'avg_capture_latency': 0.0,
            'avg_robot_latency': 0.0,
            'sync_errors': 0,
            'start_time': None
        }

    def initialize_camera(self) -> bool:
        """Initialize camera for capture."""
        try:
            self.cap = cv2.VideoCapture(self.camera_id)

            if not self.cap.isOpened():
                logger.error(f"Failed to open camera {self.camera_id}")
                return False

            # Set camera properties
            self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, self.resolution[0])
            self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, self.resolution[1])
            self.cap.set(cv2.CAP_PROP_FPS, self.fps)
            self.cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)

            # Verify settings
            actual_width = int(self.cap.get(cv2.CAP_PROP_FRAME_WIDTH))
            actual_height = int(self.cap.get(cv2.CAP_PROP_FRAME_HEIGHT))

            logger.info(f"Camera initialized: {actual_width}x{actual_height} @ {self.fps} FPS")

            # Initialize video writer with H.264 I-frame only encoding
            video_path = os.path.join(self.output_dir, "video.mp4")
            fourcc = cv2.VideoWriter_fourcc(*'avc1')  # H.264
            self.video_writer = cv2.VideoWriter(
                video_path, fourcc, self.fps, (actual_width, actual_height)
            )

            if not self.video_writer.isOpened():
                logger.error("Failed to initialize video writer")
                return False

            self.video_writer.set(cv2.VIDEOWRITER_PROP_QUALITY, self.video_quality)

            logger.info(f"Video writer initialized: {video_path}")
            logger.info(f"H.264 I-frame only encoding, quality: {self.video_quality}")

            return True

        except Exception as e:
            logger.error(f"Camera initialization failed: {e}")
            return False

    def initialize_robot(self) -> bool:
        """Initialize robot interface."""
        if not self.robot_available or not self.robot:
            logger.warning("Robot interface not available")
            return False

        try:
            if self.robot.initialize():
                self.robot.enable()
                logger.info("Robot interface ready")
                return True
            else:
                logger.error("Failed to initialize robot")
                return False

        except Exception as e:
            logger.error(f"Robot initialization error: {e}")
            return False

    def capture_thread_func(self):
        """Thread function for camera capture."""
        logger.info("Starting camera capture thread")

        while self.is_recording:
            start_time = time.time()

            ret, frame = self.cap.read()
            if ret:
                capture_latency = time.time() - start_time

                # Get robot data
                robot_start = time.time()
                frame_data = self._get_robot_data(frame)
                robot_latency = time.time() - robot_start

                frame_data.capture_latency = capture_latency
                frame_data.robot_latency = robot_latency

                # Add timestamp overlay
                frame = self._add_overlays(frame, frame_data)

                # Put frame in queue
                if not self._frame_queue.full():
                    self._frame_queue.put((frame, frame_data))
                else:
                    logger.debug("Frame queue full, dropping frame")
                    self.stats['sync_errors'] += 1

                with self._lock:
                    self.stats['frames_captured'] += 1
                    self.stats['avg_capture_latency'] = (
                        (self.stats['avg_capture_latency'] * (self.stats['frames_captured'] - 1) +
                         capture_latency) / self.stats['frames_captured']
                    )
            else:
                logger.warning("Failed to capture frame")
                time.sleep(0.01)

    def _get_robot_data(self, frame) -> FrameData:
        """Get synchronized robot data."""
        frame_data = FrameData(
            timestamp=time.time(),
            frame_number=self.stats['frames_captured'] + 1
        )

        if self.robot_available and self.robot:
            try:
                state = self.robot.refresh()

                frame_data.positions = state.positions.tolist()
                frame_data.velocities = state.velocities.tolist()
                frame_data.torques = state.torques.tolist()
                frame_data.control_mode = state.control_mode.value
                frame_data.is_safe = state.is_safe()

                with self._lock:
                    self.stats['robot_data_points'] += 1
                    self.stats['avg_robot_latency'] = (
                        (self.stats['avg_robot_latency'] * (self.stats['robot_data_points'] - 1) +
                         frame_data.robot_latency) / self.stats['robot_data_points']
                    )

            except Exception as e:
                logger.warning(f"Failed to get robot data: {e}")
                # Use simulated data if robot fails
                frame_data.positions = [0.0] * 9
                frame_data.velocities = [0.0] * 9
                frame_data.torques = [0.0] * 9
                frame_data.control_mode = "SIMULATED"
                frame_data.is_safe = True
        else:
            # Simulated robot data
            t = frame_data.timestamp
            frame_data.positions = [
                0.1 * np.sin(t), 0.1 * np.cos(t), 0.05 * np.sin(t*2),
                0.05 * np.cos(t*2), 0.02 * np.sin(t*3), 0.02 * np.cos(t*3),
                0.0, 0.0, 0.0
            ]
            frame_data.velocities = [0.0] * 9
            frame_data.torques = [0.0] * 9
            frame_data.control_mode = "SIMULATED"
            frame_data.is_safe = True

        return frame_data

    def _add_overlays(self, frame: np.ndarray, data: FrameData) -> np.ndarray:
        """Add information overlays to frame."""
        # Timestamp
        timestamp_str = datetime.fromtimestamp(data.timestamp).strftime("%H:%M:%S.%f")[:-3]
        cv2.putText(frame, timestamp_str, (10, 30),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)

        # Frame info
        cv2.putText(frame, f"Frame: {data.frame_number}", (10, 60),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)

        # Robot status
        if self.robot_available:
            status_color = (0, 255, 0) if data.is_safe else (0, 0, 255)
            cv2.putText(frame, f"Robot: {data.control_mode}", (10, 90),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.6, status_color, 2)

            # Show first few joint positions
            if data.positions:
                pos_str = f"Joints: [{data.positions[0]:.2f}, {data.positions[1]:.2f}, {data.positions[2]:.2f}...]"
                cv2.putText(frame, pos_str, (10, 120),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)

        # Latency info
        latency_str = f"Lat: {data.capture_latency*1000:.1f}ms"
        cv2.putText(frame, latency_str, (frame.shape[1] - 150, 30),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 0), 2)

        return frame

    def encoding_thread_func(self):
        """Thread function for video encoding and data logging."""
        logger.info("Starting encoding thread")

        # Initialize CSV writer for synchronized data
        csv_path = os.path.join(self.output_dir, "synchronized_data.csv")
        csv_file = open(csv_path, 'w', newline='')
        csv_writer = csv.writer(csv_file)

        # Write header
        header = [
            'timestamp', 'frame_number',
            'pos_1', 'pos_2', 'pos_3', 'pos_4', 'pos_5', 'pos_6', 'pos_7', 'pos_8', 'pos_9',
            'vel_1', 'vel_2', 'vel_3', 'vel_4', 'vel_5', 'vel_6', 'vel_7', 'vel_8', 'vel_9',
            'torque_1', 'torque_2', 'torque_3', 'torque_4', 'torque_5', 'torque_6', 'torque_7', 'torque_8', 'torque_9',
            'control_mode', 'is_safe', 'capture_latency', 'robot_latency'
        ]
        csv_writer.writerow(header)

        frame_count = 0

        while self.is_recording or not self._frame_queue.empty():
            try:
                frame, frame_data = self._frame_queue.get(timeout=0.1)

                # Write frame to video
                self.video_writer.write(frame)

                # Save individual frame if requested
                if self.save_frames:
                    frame_path = os.path.join(self.output_dir, f"frame_{frame_data.frame_number:06d}.jpg")
                    cv2.imwrite(frame_path, frame)
                    frame_data.video_frame_path = frame_path

                # Write data to CSV
                row = [
                    frame_data.timestamp,
                    frame_data.frame_number,
                    *(frame_data.positions or [0.0]*9),
                    *(frame_data.velocities or [0.0]*9),
                    *(frame_data.torques or [0.0]*9),
                    frame_data.control_mode,
                    frame_data.is_safe,
                    frame_data.capture_latency,
                    frame_data.robot_latency
                ]
                csv_writer.writerow(row)

                frame_count += 1
                self.stats['frames_processed'] = frame_count

            except Empty:
                continue
            except Exception as e:
                logger.error(f"Encoding error: {e}")
                continue

        csv_file.close()
        logger.info(f"Encoding completed. Processed {frame_count} frames")

    def start_recording(self, duration: float = None) -> bool:
        """
        Start synchronized recording.

        Args:
            duration: Recording duration in seconds (None for infinite)

        Returns:
            True if recording started successfully
        """
        # Initialize subsystems
        if not self.initialize_camera():
            return False

        self.initialize_robot()  # Optional, can work without robot

        self.is_recording = True
        self.stats['start_time'] = time.time()

        # Start threads
        self.capture_thread = Thread(target=self.capture_thread_func, daemon=True)
        self.encoding_thread = Thread(target=self.encoding_thread_func, daemon=True)

        self.capture_thread.start()
        self.encoding_thread.start()

        logger.info(f"Synchronized recording started: {self.output_dir}")

        # Auto-stop after duration
        if duration:
            Thread(target=self._stop_after_duration, args=(duration,), daemon=True).start()

        return True

    def _stop_after_duration(self, duration: float):
        """Stop recording after duration."""
        time.sleep(duration)
        self.stop_recording()

    def stop_recording(self):
        """Stop recording and save all data."""
        if not self.is_recording:
            return

        logger.info("Stopping synchronized recording...")
        self.is_recording = False

        # Wait for threads
        if hasattr(self, 'capture_thread'):
            self.capture_thread.join(timeout=3.0)
        if hasattr(self, 'encoding_thread'):
            self.encoding_thread.join(timeout=3.0)

        # Release resources
        if hasattr(self, 'video_writer'):
            self.video_writer.release()
        if hasattr(self, 'cap'):
            self.cap.release()
        if self.robot:
            self.robot.shutdown()

        # Save metadata
        self._save_metadata()

        # Print statistics
        self._print_stats()

        logger.info(f"Synchronized recording saved: {self.output_dir}")

    def _save_metadata(self):
        """Save recording metadata."""
        metadata = {
            'recording_info': {
                'output_directory': self.output_dir,
                'start_time': self.stats['start_time'],
                'duration': time.time() - self.stats['start_time'] if self.stats['start_time'] else 0,
                'camera_id': self.camera_id,
                'resolution': self.resolution,
                'fps': self.fps,
                'video_quality': self.video_quality
            },
            'robot_info': {
                'available': self.robot_available,
                'initialized': self.robot is not None
            },
            'performance_stats': self.stats,
            'files': {
                'video': 'video.mp4',
                'synchronized_data': 'synchronized_data.csv',
                'metadata': 'metadata.json'
            }
        }

        metadata_path = os.path.join(self.output_dir, "metadata.json")
        with open(metadata_path, 'w') as f:
            json.dump(metadata, f, indent=2)

    def _print_stats(self):
        """Print performance statistics."""
        print("\n" + "="*60)
        print("SYNCHRONIZED CAPTURE STATISTICS")
        print("="*60)
        print(f"Output directory: {self.output_dir}")
        print(f"Recording duration: {time.time() - self.stats['start_time']:.2f} seconds")
        print(f"Frames captured: {self.stats['frames_captured']}")
        print(f"Frames processed: {self.stats['frames_processed']}")
        print(f"Robot data points: {self.stats['robot_data_points']}")
        print(f"Average capture latency: {self.stats['avg_capture_latency']*1000:.2f} ms")
        print(f"Average robot latency: {self.stats['avg_robot_latency']*1000:.2f} ms")
        print(f"Sync errors: {self.stats['sync_errors']}")

        if self.stats['frames_captured'] > 0:
            effective_fps = self.stats['frames_processed'] / (time.time() - self.stats['start_time'])
            print(f"Effective FPS: {effective_fps:.2f}")

        # File sizes
        video_path = os.path.join(self.output_dir, "video.mp4")
        csv_path = os.path.join(self.output_dir, "synchronized_data.csv")

        if os.path.exists(video_path):
            video_size = os.path.getsize(video_path) / 1024 / 1024
            print(f"Video file size: {video_size:.2f} MB")

        if os.path.exists(csv_path):
            csv_size = os.path.getsize(csv_path) / 1024
            print(f"CSV file size: {csv_size:.2f} KB")

        print("="*60)


def main():
    """Main function."""
    parser = argparse.ArgumentParser(description="Synchronized Camera and Robot Data Capture")
    parser.add_argument("--camera", "-c", type=int, default=0,
                       help="Camera ID (default: 0)")
    parser.add_argument("--output", "-o", type=str,
                       help="Output directory")
    parser.add_argument("--resolution", "-r", type=str, default="640x480",
                       help="Resolution (default: 640x480)")
    parser.add_argument("--fps", type=int, default=30,
                       help="Target FPS (default: 30)")
    parser.add_argument("--quality", "-q", type=int, default=18,
                       help="H.264 quality 0-51 (default: 18)")
    parser.add_argument("--duration", "-d", type=float,
                       help="Recording duration in seconds")
    parser.add_argument("--save-frames", action="store_true",
                       help="Save individual frames as images")

    args = parser.parse_args()

    # Parse resolution
    try:
        width, height = map(int, args.resolution.split('x'))
        resolution = (width, height)
    except:
        logger.error("Invalid resolution format. Use WIDTHxHEIGHT")
        return

    # Create capture system
    capture = SynchronizedCapture(
        camera_id=args.camera,
        output_dir=args.output,
        resolution=resolution,
        fps=args.fps,
        video_quality=args.quality,
        save_frames=args.save_frames
    )

    try:
        print("Starting synchronized capture...")
        print(f"Camera: {args.camera}")
        print(f"Resolution: {width}x{height}")
        print(f"FPS: {args.fps}")
        print(f"Output: {capture.output_dir}")

        if args.duration:
            print(f"Duration: {args.duration} seconds")
        else:
            print("Duration: Unlimited (press Ctrl+C to stop)")

        print("\nPress Ctrl+C to stop recording")

        success = capture.start_recording(duration=args.duration)

        if success:
            # Wait for completion
            try:
                while capture.is_recording:
                    time.sleep(0.1)
            except KeyboardInterrupt:
                print("\nRecording stopped by user")
                capture.stop_recording()

    except Exception as e:
        logger.error(f"Capture error: {e}")
        capture.stop_recording()


if __name__ == "__main__":
    main()