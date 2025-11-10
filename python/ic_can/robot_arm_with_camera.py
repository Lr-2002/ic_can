"""
RobotArm with Integrated Camera

This module extends the RobotArm class with integrated camera capabilities,
showing how to embed camera collection as a subsystem of the main robot control.

Features:
- Camera as a subsystem of RobotArm
- Thread-safe integration
- Synchronized data collection
- Minimal impact on robot control performance
- Easy to use interface
"""

import threading
import time
import logging
from typing import Optional, Dict, Any

from .robot_arm import RobotArm
from .camera_module import CameraDataCollector, CameraConfig, IntegratedDataCollector
from .motor_state import MotorState

logger = logging.getLogger(__name__)


class RobotArmWithCamera(RobotArm):
    """
    Extended RobotArm class with integrated camera subsystem.

    This class demonstrates inheritance-based integration of camera
    functionality into the main robot control pipeline.
    """

    def __init__(self, device_sn: str = "F561E08C892274DB09496BCC1102DBC5",
                 camera_config: CameraConfig = None,
                 debug: bool = False,
                 safety_limits: bool = True):
        """
        Initialize RobotArm with integrated camera.

        Args:
            device_sn: Device serial number for CAN communication
            camera_config: Camera configuration
            debug: Enable debug logging
            safety_limits: Enable automatic position/velocity/torque limiting
        """
        # Initialize parent RobotArm
        super().__init__(device_sn, debug, safety_limits)

        # Initialize camera subsystem
        self.camera_config = camera_config or CameraConfig()
        self.camera: Optional[CameraDataCollector] = None
        self.camera_enabled = False

        # Integrated data collector
        self.integrated_collector: Optional[IntegratedDataCollector] = None

        # Thread safety
        self._camera_lock = threading.Lock()

        logger.info("RobotArmWithCamera initialized")

    def initialize_camera(self) -> bool:
        """
        Initialize camera subsystem.

        Returns:
            True if camera initialized successfully
        """
        with self._camera_lock:
            try:
                # Create camera collector
                output_dir = f"robot_camera_{int(time.time())}"
                self.camera = CameraDataCollector(self.camera_config, output_dir)

                # Set callback for robot synchronization
                self.camera.set_frame_callback(self._on_camera_frame)

                logger.info("Camera subsystem initialized")
                return True

            except Exception as e:
                logger.error(f"Camera initialization failed: {e}")
                return False

    def _on_camera_frame(self, camera_frame):
        """
        Handle new camera frame (internal integration point).

        Args:
            camera_frame: New camera frame from camera subsystem
        """
        try:
            # This is where you can add custom processing
            # that runs in parallel with robot control
            pass
        except Exception as e:
            logger.warning(f"Camera frame processing error: {e}")

    def enable_camera(self) -> bool:
        """
        Enable camera subsystem.

        Returns:
            True if camera enabled successfully
        """
        if not self.camera:
            if not self.initialize_camera():
                return False

        with self._camera_lock:
            if self.camera.start_recording():
                self.camera_enabled = True
                logger.info("Camera subsystem enabled")
                return True
            else:
                logger.error("Failed to start camera recording")
                return False

    def disable_camera(self):
        """Disable camera subsystem."""
        with self._camera_lock:
            if self.camera and self.camera_enabled:
                self.camera.stop_recording()
                self.camera_enabled = False
                logger.info("Camera subsystem disabled")

    def start_integrated_collection(self) -> bool:
        """
        Start synchronized robot-camera data collection.

        Returns:
            True if collection started successfully
        """
        if not self._initialized:
            logger.error("Robot not initialized")
            return False

        try:
            # Create integrated collector
            self.integrated_collector = IntegratedDataCollector(
                robot=self,
                camera_config=self.camera_config
            )

            # Start collection
            if self.integrated_collector.start_collection():
                logger.info("Integrated data collection started")
                return True
            else:
                logger.error("Failed to start integrated collection")
                return False

        except Exception as e:
            logger.error(f"Integrated collection error: {e}")
            return False

    def stop_integrated_collection(self):
        """Stop integrated data collection."""
        if self.integrated_collector:
            self.integrated_collector.stop_collection()
            self.integrated_collector = None
            logger.info("Integrated data collection stopped")

    def refresh_with_camera(self) -> tuple:
        """
        Refresh robot state and get latest camera frame.

        Returns:
            Tuple of (robot_state, camera_frame)
        """
        # Refresh robot state (parent method)
        robot_state = self.refresh()

        # Get latest camera frame if available
        camera_frame = None
        if self.camera:
            camera_frame = self.camera.get_latest_frame(timeout=0.01)

        return robot_state, camera_frame

    def get_comprehensive_stats(self) -> Dict[str, Any]:
        """Get comprehensive statistics including camera data."""
        stats = self.get_performance_stats()

        # Add camera statistics
        if self.camera:
            stats['camera'] = self.camera.get_statistics()

        # Add integrated collection statistics
        if self.integrated_collector:
            stats['integrated_collection'] = self.integrated_collector.get_statistics()

        return stats

    def shutdown(self) -> bool:
        """
        Shutdown robot and camera subsystems.

        Returns:
            True if shutdown successful
        """
        logger.info("Shutting down RobotArmWithCamera...")

        # Stop camera subsystem
        self.disable_camera()
        self.stop_integrated_collection()

        # Shutdown parent RobotArm
        return super().shutdown()

    def __enter__(self):
        """Context manager entry with camera."""
        if self.initialize():
            return self
        else:
            raise RuntimeError("Failed to initialize RobotArmWithCamera")

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit with camera cleanup."""
        self.shutdown()


class ThreadedDataCollector:
    """
    Standalone threaded data collector for pipeline integration.

    This class shows how to run camera collection as a completely
    independent thread that can be integrated into any pipeline.
    """

    def __init__(self, robot, camera_config: CameraConfig = None):
        """
        Initialize threaded data collector.

        Args:
            robot: RobotArm instance
            camera_config: Camera configuration
        """
        self.robot = robot
        self.camera_config = camera_config or CameraConfig()

        # Threading
        self._collection_thread = None
        self._is_running = False
        self._lock = threading.Lock()

        # Data storage
        self.latest_data = None
        self.data_history = []

    def _collection_loop(self):
        """Main collection loop running in separate thread."""
        logger.info("Starting threaded data collection")

        # Initialize integrated collector
        collector = IntegratedDataCollector(self.robot, self.camera_config)

        if not collector.start_collection():
            logger.error("Failed to start integrated collection")
            return

        try:
            while self._is_running:
                # Get latest synchronized data
                latest = collector.get_latest_synchronized_data()
                if latest:
                    with self._lock:
                        self.latest_data = latest
                        self.data_history.append(latest)

                        # Limit history size
                        if len(self.data_history) > 1000:
                            self.data_history = self.data_history[-500:]

                time.sleep(0.01)  # Small sleep to prevent CPU spinning

        except Exception as e:
            logger.error(f"Collection loop error: {e}")
        finally:
            collector.stop_collection()
            logger.info("Threaded data collection stopped")

    def start_collection(self) -> bool:
        """
        Start threaded data collection.

        Returns:
            True if started successfully
        """
        if self._is_running:
            logger.warning("Collection already running")
            return True

        self._is_running = True
        self._collection_thread = threading.Thread(target=self._collection_loop, daemon=True)
        self._collection_thread.start()

        logger.info("Threaded data collection started")
        return True

    def stop_collection(self):
        """Stop threaded data collection."""
        if not self._is_running:
            return

        logger.info("Stopping threaded data collection...")
        self._is_running = False

        if self._collection_thread:
            self._collection_thread.join(timeout=5.0)

        logger.info("Threaded data collection stopped")

    def get_latest_data(self):
        """Get the latest synchronized data."""
        with self._lock:
            return self.latest_data

    def get_data_history(self, limit: int = None):
        """Get data history."""
        with self._lock:
            if limit:
                return self.data_history[-limit:]
            return self.data_history.copy()


# Example usage functions
def example_inheritance_integration():
    """Example of inheritance-based integration."""
    print("=== Inheritance Integration Example ===")

    # Create robot with camera
    camera_config = CameraConfig(
        camera_id=0,
        resolution=(640, 480),
        fps=30,
        save_frames=False
    )

    with RobotArmWithCamera(camera_config=camera_config, debug=True) as robot:
        # Initialize robot systems
        robot.enable()
        robot.set_control_mode("EXECUTION")

        # Enable camera subsystem
        if robot.enable_camera():
            print("Camera subsystem enabled")

            # Control loop with camera
            for i in range(50):
                # Get robot state and camera frame
                robot_state, camera_frame = robot.refresh_with_camera()

                # Robot control
                action = [0.1 * (i % 10) / 10] * 9
                robot.step(action)

                # Print status
                if camera_frame:
                    print(f"Step {i}: Robot pos={np.mean(robot_state.positions):.3f}, "
                          f"Camera frame={camera_frame.frame_number}, "
                          f"Latency={camera_frame.capture_latency*1000:.1f}ms")
                else:
                    print(f"Step {i}: Robot pos={np.mean(robot_state.positions):.3f}, "
                          f"No camera frame")

                time.sleep(0.1)

            # Disable camera
            robot.disable_camera()

        # Show comprehensive statistics
        stats = robot.get_comprehensive_stats()
        print(f"\nComprehensive Statistics:")
        print(f"Robot refreshes: {stats['refresh_count']}")
        if 'camera' in stats:
            print(f"Camera frames: {stats['camera']['frames_captured']}")
            print(f"Camera drop rate: {stats['camera']['drop_rate_percent']:.2f}%")

    print("Inheritance integration example completed\n")


def example_threaded_integration():
    """Example of threaded integration."""
    print("=== Threaded Integration Example ===")

    # Create regular robot
    import ic_can
    robot = ic_can.create_robot_arm(debug=False)
    robot.enable()

    # Create threaded collector
    collector = ThreadedDataCollector(robot)
    collector.start_collection()

    try:
        print("Threaded collector started")

        # Main control loop (independent of camera)
        for i in range(50):
            # Regular robot control
            state = robot.refresh()
            action = [0.1 * np.sin(i * 0.1)] * 9
            robot.step(action)

            # Get camera data independently
            latest_data = collector.get_latest_data()
            if latest_data:
                print(f"Step {i}: Robot pos={np.mean(state.positions):.3f}, "
                      f"Camera frame={latest_data['frame_number']}, "
                      f"Robot-Camera sync={latest_data['sync_latency']*1000:.1f}ms")
            else:
                print(f"Step {i}: Robot pos={np.mean(state.positions):.3f}, "
                      f"Waiting for camera sync...")

            time.sleep(0.1)

    finally:
        # Stop collector
        collector.stop_collection()
        robot.shutdown()

    print("Threaded integration example completed\n")


def example_pipeline_integration():
    """Example of pipeline integration pattern."""
    print("=== Pipeline Integration Example ===")

    class DataPipeline:
        """Example data pipeline with integrated camera."""

        def __init__(self):
            # Initialize components
            self.robot = None
            self.camera_collector = None
            self.is_running = False

        def initialize(self):
            """Initialize pipeline components."""
            import ic_can
            self.robot = ic_can.create_robot_arm(debug=False)
            self.robot.enable()

            # Initialize camera as separate subsystem
            camera_config = CameraConfig(fps=15)  # Lower FPS for pipeline
            self.camera_collector = IntegratedDataCollector(self.robot, camera_config)

            return True

        def start(self):
            """Start pipeline."""
            if self.camera_collector.start_collection():
                self.is_running = True
                print("Pipeline started with camera integration")
                return True
            return False

        def run_control_loop(self, duration=10):
            """Run main control loop."""
            start_time = time.time()

            while time.time() - start_time < duration and self.is_running:
                # Robot control (independent of camera)
                state = self.robot.refresh()
                action = [0.05 * np.sin(time.time())] * 9
                robot.step(action)

                # Pipeline can access camera data independently
                latest_sync = self.camera_collector.get_latest_synchronized_data()
                if latest_sync and time.time() - latest_sync['timestamp'] < 0.1:
                    print(f"Pipeline: Robot active, Camera sync available "
                          f"(frame {latest_sync['frame_number']})")

                time.sleep(0.05)

        def stop(self):
            """Stop pipeline."""
            if self.camera_collector:
                self.camera_collector.stop_collection()
            if self.robot:
                self.robot.shutdown()
            self.is_running = False
            print("Pipeline stopped")

    # Run pipeline example
    pipeline = DataPipeline()

    if pipeline.initialize():
        pipeline.start()
        pipeline.run_control_loop(duration=5)
        pipeline.stop()

    print("Pipeline integration example completed\n")


if __name__ == "__main__":
    import numpy as np
    import logging

    # Setup logging
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

    # Run examples
    try:
        example_inheritance_integration()
        example_threaded_integration()
        example_pipeline_integration()
    except Exception as e:
        print(f"Example failed (expected if hardware not available): {e}")
        print("The integration patterns are ready for use with actual hardware.")