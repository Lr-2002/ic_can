"""
Camera Manager for Dual-Arm System

This module provides camera management and integration for the dual-arm robotic system,
supporting 2 hand-eye cameras (one per arm) and 1 fixed overhead camera.

Features:
    - Multi-camera synchronization
    - Arm-specific camera mapping
    - Calibration management
    - Real-time capture with arm state correlation
    - Depth sensing support (optional)
"""

import cv2
import numpy as np
import time
import logging
import json
from typing import Dict, List, Optional, Tuple, Any, Callable
from threading import Lock, Thread, Event
from dataclasses import dataclass, asdict
from pathlib import Path
import queue

from .dual_arm_manager import DualArmManager, DualArmState

# Setup logging
logger = logging.getLogger(__name__)

@dataclass
class CameraFrame:
    """Container for camera frame data"""
    camera_id: int
    timestamp: float
    image: np.ndarray
    depth_image: Optional[np.ndarray] = None
    arm_state: Optional[DualArmState] = None

@dataclass
class CameraCalibration:
    """Camera calibration parameters"""
    camera_matrix: np.ndarray
    distortion_coeffs: np.ndarray
    rotation_matrix: Optional[np.ndarray] = None
    translation_vector: Optional[np.ndarray] = None
    hand_eye_matrix: Optional[np.ndarray] = None  # For hand-eye cameras

class CameraManager:
    """
    Manager for multi-camera system in dual-arm setup.

    This class handles camera initialization, capture, synchronization, and integration
    with the dual-arm system state.

    Camera Configuration:
        - Camera 0: Left hand-eye camera
        - Camera 1: Right hand-eye camera
        - Camera 2: Fixed overhead camera
    """

    def __init__(self,
                 camera_config_file: str = "config/dual_camera_config.json",
                 dual_arm_manager: Optional[DualArmManager] = None,
                 debug: bool = False):
        """
        Initialize camera manager.

        Args:
            camera_config_file: Path to camera configuration JSON file
            dual_arm_manager: Optional dual-arm manager for state correlation
            debug: Enable debug logging
        """
        self.debug = debug
        self.config_dir = Path(__file__).parent.parent.parent / "config"
        self.camera_config_file = self.config_dir / "dual_camera_config.json"

        # Dual-arm manager reference
        self.dual_arm_manager = dual_arm_manager

        # Camera configuration
        self.camera_config = {}
        self.calibrations: Dict[int, CameraCalibration] = {}

        # Camera instances
        self.cameras: Dict[int, cv2.VideoCapture] = {}
        self.camera_properties: Dict[int, Dict[str, Any]] = {}

        # Capture management
        self.capture_threads: Dict[int, Thread] = {}
        self.stop_events: Dict[int, Event] = {}
        self.frame_queues: Dict[int, queue.Queue] = {}
        self.is_capturing = False

        # Synchronization
        self.enable_synchronized_capture = True
        self.sync_timeout = 0.1  # seconds

        # Frame callbacks
        self.frame_callbacks: List[Callable[[CameraFrame], None]] = []

        # Statistics
        self.capture_stats: Dict[int, Dict[str, Any]] = {}
        self.last_frame_time: Dict[int, float] = {}

        logger.info("CameraManager initialized")

    def load_configuration(self) -> bool:
        """Load camera configuration from JSON file."""
        try:
            with open(self.camera_config_file, 'r') as f:
                self.camera_config = json.load(f)

            self.enable_synchronized_capture = self.camera_config.get('enable_synchronized_capture', True)

            # Initialize camera properties
            for camera_data in self.camera_config['cameras']:
                camera_id = camera_data['camera_id']
                self.camera_properties[camera_id] = camera_data
                self.capture_stats[camera_id] = {
                    'frames_captured': 0,
                    'capture_rate': 0.0,
                    'last_capture_time': 0.0,
                    'errors': 0
                }
                self.last_frame_time[camera_id] = 0.0

            logger.info(f"Camera configuration loaded: {len(self.camera_config['cameras'])} cameras")
            return True

        except Exception as e:
            logger.error(f"Failed to load camera configuration: {e}")
            return False

    def load_calibrations(self, calibration_dir: str = "calibration") -> bool:
        """Load camera calibration files."""
        try:
            cal_dir = Path(calibration_dir)
            if not cal_dir.exists():
                logger.warning(f"Calibration directory not found: {cal_dir}")
                return False

            for camera_data in self.camera_config['cameras']:
                camera_id = camera_data['camera_id']
                calibration_file = cal_dir / camera_data.get('calibration_file', f"camera_{camera_id}.json")

                if calibration_file.exists():
                    calibration = self._load_calibration(calibration_file)
                    if calibration:
                        self.calibrations[camera_id] = calibration
                        logger.info(f"Loaded calibration for camera {camera_id}")
                else:
                    logger.warning(f"Calibration file not found: {calibration_file}")

            return True

        except Exception as e:
            logger.error(f"Failed to load calibrations: {e}")
            return False

    def _load_calibration(self, calibration_file: Path) -> Optional[CameraCalibration]:
        """Load calibration from JSON file."""
        try:
            with open(calibration_file, 'r') as f:
                cal_data = json.load(f)

            return CameraCalibration(
                camera_matrix=np.array(cal_data['camera_matrix']),
                distortion_coeffs=np.array(cal_data['distortion_coeffs']),
                rotation_matrix=np.array(cal_data['rotation_matrix']) if 'rotation_matrix' in cal_data else None,
                translation_vector=np.array(cal_data['translation_vector']) if 'translation_vector' in cal_data else None,
                hand_eye_matrix=np.array(cal_data['hand_eye_matrix']) if 'hand_eye_matrix' in cal_data else None
            )

        except Exception as e:
            logger.error(f"Failed to load calibration from {calibration_file}: {e}")
            return None

    def initialize_cameras(self) -> bool:
        """Initialize all cameras according to configuration."""
        if not self.load_configuration():
            return False

        success = True

        for camera_data in self.camera_config['cameras']:
            camera_id = camera_data['camera_id']

            if not camera_data.get('enabled', True):
                logger.info(f"Camera {camera_id} disabled in configuration")
                continue

            try:
                # Initialize camera
                cap = cv2.VideoCapture(camera_data['device_path'])

                if not cap.isOpened():
                    logger.error(f"Failed to open camera {camera_id} at {camera_data['device_path']}")
                    success = False
                    continue

                # Set camera properties
                cap.set(cv2.CAP_PROP_FRAME_WIDTH, camera_data.get('resolution_width', 640))
                cap.set(cv2.CAP_PROP_FRAME_HEIGHT, camera_data.get('resolution_height', 480))
                cap.set(cv2.CAP_PROP_FPS, camera_data.get('framerate', 30.0))

                self.cameras[camera_id] = cap
                self.frame_queues[camera_id] = queue.Queue(maxsize=10)
                self.stop_events[camera_id] = Event()

                logger.info(f"Camera {camera_id} initialized: {camera_data['camera_name']}")

            except Exception as e:
                logger.error(f"Failed to initialize camera {camera_id}: {e}")
                success = False

        # Load calibrations
        self.load_calibrations()

        return success

    def start_capture(self) -> bool:
        """Start capturing from all enabled cameras."""
        if self.is_capturing:
            logger.warning("Capture already started")
            return True

        if not self.cameras:
            logger.error("No cameras initialized")
            return False

        self.is_capturing = True

        for camera_id, camera in self.cameras.items():
            self.stop_events[camera_id].clear()

            # Start capture thread for each camera
            capture_thread = Thread(target=self._capture_loop, args=(camera_id,))
            capture_thread.daemon = True
            capture_thread.start()
            self.capture_threads[camera_id] = capture_thread

        logger.info(f"Started capture from {len(self.cameras)} cameras")
        return True

    def stop_capture(self):
        """Stop capturing from all cameras."""
        if not self.is_capturing:
            return

        logger.info("Stopping camera capture...")
        self.is_capturing = False

        # Stop all capture threads
        for camera_id in self.cameras:
            if camera_id in self.stop_events:
                self.stop_events[camera_id].set()

        # Wait for threads to finish
        for camera_id, thread in self.capture_threads.items():
            if thread.is_alive():
                thread.join(timeout=1.0)

        logger.info("Camera capture stopped")

    def _capture_loop(self, camera_id: int):
        """Capture loop for individual camera."""
        camera = self.cameras[camera_id]
        stop_event = self.stop_events[camera_id]
        frame_queue = self.frame_queues[camera_id]
        camera_props = self.camera_properties[camera_id]

        last_frame_time = 0.0
        target_interval = 1.0 / camera_props.get('framerate', 30.0)

        while not stop_event.is_set():
            try:
                current_time = time.time()

                # Frame rate limiting
                if current_time - last_frame_time < target_interval:
                    time.sleep(0.001)
                    continue

                # Capture frame
                ret, frame = camera.read()
                if not ret:
                    logger.warning(f"Failed to capture frame from camera {camera_id}")
                    self.capture_stats[camera_id]['errors'] += 1
                    continue

                # Create frame object
                camera_frame = CameraFrame(
                    camera_id=camera_id,
                    timestamp=current_time,
                    image=frame
                )

                # Add arm state if available
                if self.dual_arm_manager:
                    camera_frame.arm_state = self.dual_arm_manager.get_current_state()

                # Add to queue (non-blocking)
                try:
                    frame_queue.put_nowait(camera_frame)
                except queue.Full:
                    # Remove oldest frame if queue is full
                    try:
                        frame_queue.get_nowait()
                        frame_queue.put_nowait(camera_frame)
                    except queue.Empty:
                        pass

                # Update statistics
                self.capture_stats[camera_id]['frames_captured'] += 1
                self.capture_stats[camera_id]['last_capture_time'] = current_time

                if last_frame_time > 0:
                    interval = current_time - last_frame_time
                    self.capture_stats[camera_id]['capture_rate'] = 1.0 / interval

                last_frame_time = current_time

                # Trigger callbacks
                for callback in self.frame_callbacks:
                    try:
                        callback(camera_frame)
                    except Exception as e:
                        logger.error(f"Frame callback error: {e}")

            except Exception as e:
                logger.error(f"Capture loop error for camera {camera_id}: {e}")
                self.capture_stats[camera_id]['errors'] += 1
                time.sleep(0.01)  # Brief pause on error

    def get_latest_frame(self, camera_id: int, timeout: float = 0.1) -> Optional[CameraFrame]:
        """Get the latest frame from specified camera."""
        if camera_id not in self.frame_queues:
            logger.warning(f"Camera {camera_id} not available")
            return None

        try:
            return self.frame_queues[camera_id].get(timeout=timeout)
        except queue.Empty:
            return None

    def get_synchronized_frames(self, timeout: float = 0.1) -> Dict[int, CameraFrame]:
        """Get synchronized frames from all cameras."""
        if not self.enable_synchronized_capture:
            # Return latest frames from each camera
            frames = {}
            for camera_id in self.cameras:
                frame = self.get_latest_frame(camera_id, timeout=0.01)
                if frame:
                    frames[camera_id] = frame
            return frames

        # Synchronized capture logic
        frames = {}
        reference_time = None
        sync_tolerance = 0.05  # 50ms tolerance

        # Get latest frame from each camera
        for camera_id in self.cameras:
            frame = self.get_latest_frame(camera_id, timeout=0.01)
            if frame:
                frames[camera_id] = frame
                if reference_time is None:
                    reference_time = frame.timestamp

        # Check synchronization
        if reference_time and len(frames) > 1:
            synchronized_frames = {}
            for camera_id, frame in frames.items():
                time_diff = abs(frame.timestamp - reference_time)
                if time_diff <= sync_tolerance:
                    synchronized_frames[camera_id] = frame
                else:
                    logger.debug(f"Camera {camera_id} frame not synchronized: {time_diff:.3f}s difference")

            return synchronized_frames

        return frames

    def add_frame_callback(self, callback: Callable[[CameraFrame], None]):
        """Add callback function for new frames."""
        self.frame_callbacks.append(callback)

    def remove_frame_callback(self, callback: Callable[[CameraFrame], None]):
        """Remove frame callback."""
        if callback in self.frame_callbacks:
            self.frame_callbacks.remove(callback)

    def get_camera_info(self, camera_id: int) -> Optional[Dict[str, Any]]:
        """Get camera information and statistics."""
        if camera_id not in self.camera_properties:
            return None

        info = self.camera_properties[camera_id].copy()
        info.update(self.capture_stats.get(camera_id, {}))
        info['is_initialized'] = camera_id in self.cameras
        info['is_capturing'] = self.is_capturing
        info['has_calibration'] = camera_id in self.calibrations

        return info

    def get_all_camera_info(self) -> Dict[int, Dict[str, Any]]:
        """Get information for all cameras."""
        info = {}
        for camera_id in self.camera_properties:
            info[camera_id] = self.get_camera_info(camera_id)
        return info

    def capture_synchronized_image_set(self) -> Optional[Dict[int, np.ndarray]]:
        """Capture a synchronized set of images from all cameras."""
        frames = self.get_synchronized_frames()

        if not frames:
            return None

        # Check if we have frames from all enabled cameras
        enabled_cameras = [cid for cid, props in self.camera_properties.items()
                          if props.get('enabled', True)]

        if len(frames) != len(enabled_cameras):
            logger.warning(f"Got frames from {len(frames)} cameras, expected {len(enabled_cameras)}")

        # Extract images
        images = {camera_id: frame.image for camera_id, frame in frames.items()}
        return images

    def save_frame(self, frame: CameraFrame, filename: str) -> bool:
        """Save a camera frame to file."""
        try:
            cv2.imwrite(filename, frame.image)
            logger.debug(f"Saved frame from camera {frame.camera_id} to {filename}")
            return True
        except Exception as e:
            logger.error(f"Failed to save frame: {e}")
            return False

    def print_status(self):
        """Print camera system status."""
        print("\n📷 Camera System Status:")
        print("=" * 50)

        for camera_id, props in self.camera_properties.items():
            stats = self.capture_stats.get(camera_id, {})
            is_capturing = camera_id in self.cameras and self.is_capturing

            print(f"Camera {camera_id} ({props['camera_name']}):")
            print(f"  Status: {'✅ Capturing' if is_capturing else '❌ Not capturing'}")
            print(f"  Device: {props['device_path']}")
            print(f"  Type: {'Hand-eye' if props['is_hand_eye_camera'] else 'Fixed'}")
            print(f"  Associated arm: {props['associated_arm']}")
            print(f"  Resolution: {props['resolution_width']}x{props['resolution_height']}")
            print(f"  Framerate: {props['framerate']} Hz")
            print(f"  Capture rate: {stats.get('capture_rate', 0):.1f} Hz")
            print(f"  Frames captured: {stats.get('frames_captured', 0)}")
            print(f"  Calibration: {'✅ Loaded' if camera_id in self.calibrations else '❌ Not loaded'}")
            print()

        print(f"Synchronized capture: {'✅ Enabled' if self.enable_synchronized_capture else '❌ Disabled'}")
        print("=" * 50)

    def shutdown(self):
        """Shutdown camera system."""
        logger.info("Shutting down camera system...")

        # Stop capture
        self.stop_capture()

        # Release cameras
        for camera_id, camera in self.cameras.items():
            camera.release()
            logger.info(f"Camera {camera_id} released")

        self.cameras.clear()
        self.frame_queues.clear()
        self.capture_threads.clear()
        self.stop_events.clear()
        self.frame_callbacks.clear()

        logger.info("Camera system shutdown complete")

# Convenience function for easy usage
def create_camera_manager(dual_arm_manager: Optional[DualArmManager] = None,
                        debug: bool = False) -> CameraManager:
    """Create a camera manager with default configuration."""
    return CameraManager(dual_arm_manager=dual_arm_manager, debug=debug)