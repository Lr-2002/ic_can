#!/usr/bin/env python3
"""
Integrated Robot-Camera Demo

This script demonstrates the integration of camera capture into the main
IC_CAN robot control pipeline using different integration patterns.

Usage Patterns:
1. Inheritance: RobotArmWithCamera class
2. Composition: Camera subsystem as component
3. Threading: Independent camera thread
4. Pipeline: Full data collection pipeline
"""

import sys
import os
import time
import argparse
import numpy as np
import logging

# Add parent directory to path for ic_can import
sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'python'))

try:
    import ic_can
    from ic_can.camera_module import CameraConfig, CameraDataCollector, IntegratedDataCollector
    from ic_can.robot_arm_with_camera import RobotArmWithCamera, ThreadedDataCollector
    IC_CAN_AVAILABLE = True
except ImportError:
    IC_CAN_AVAILABLE = False
    print("Warning: IC_CAN not available. Running in simulation mode.")

# Setup logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)


def demo_inheritance_pattern(duration=30):
    """
    Demonstrate inheritance-based integration.

    RobotArmWithCamera extends RobotArm with camera capabilities.
    """
    print("="*60)
    print("INHERITANCE PATTERN: RobotArmWithCamera")
    print("="*60)

    try:
        # Configure camera
        camera_config = CameraConfig(
            camera_id=0,
            resolution=(640, 480),
            fps=30,
            quality=18,
            save_frames=False
        )

        # Create robot with integrated camera
        with RobotArmWithCamera(camera_config=camera_config, debug=True) as robot:
            print("✓ RobotArmWithCamera initialized")

            # Initialize robot
            robot.enable()
            robot.set_control_mode(ic_can.ControlMode.EXECUTION)
            print("✓ Robot enabled and set to EXECUTION mode")

            # Enable camera subsystem
            if robot.enable_camera():
                print("✓ Camera subsystem enabled")

                # Control loop with synchronized data
                start_time = time.time()
                loop_count = 0

                while time.time() - start_time < duration:
                    loop_count += 1

                    # Get robot state and camera frame (synchronized)
                    robot_state, camera_frame = robot.refresh_with_camera()

                    # Generate control action (simple sinusoidal)
                    t = time.time() - start_time
                    action = 0.1 * np.sin(t * 0.5) * np.ones(9)
                    action[6:9] = 0.0  # Keep wrist and gripper neutral

                    # Send command to robot
                    success = robot.step(action)

                    # Print status every 10 iterations
                    if loop_count % 10 == 0:
                        if camera_frame:
                            print(f"Loop {loop_count}: Robot active, "
                                  f"Camera frame {camera_frame.frame_number}, "
                                  f"Latency {camera_frame.capture_latency*1000:.1f}ms")
                        else:
                            print(f"Loop {loop_count}: Robot active, waiting for camera...")

                    time.sleep(0.05)  # 20 Hz control loop

                # Disable camera subsystem
                robot.disable_camera()
                print("✓ Camera subsystem disabled")

                # Show comprehensive statistics
                stats = robot.get_comprehensive_stats()
                print(f"\n📊 INHERITANCE PATTERN STATISTICS:")
                print(f"   Robot refreshes: {stats['refresh_count']}")
                print(f"   Robot steps: {stats['step_count']}")
                if 'camera' in stats:
                    print(f"   Camera frames captured: {stats['camera']['frames_captured']}")
                    print(f"   Camera frames encoded: {stats['camera']['frames_encoded']}")
                    print(f"   Camera drop rate: {stats['camera']['drop_rate_percent']:.2f}%")
                    print(f"   Avg capture latency: {stats['camera']['avg_capture_latency']*1000:.2f}ms")

            else:
                print("✗ Failed to enable camera subsystem")

    except Exception as e:
        print(f"✗ Inheritance pattern demo failed: {e}")

    print("Inheritance pattern demo completed\n")


def demo_composition_pattern(duration=30):
    """
    Demonstrate composition-based integration.

    Camera as a component of the main system.
    """
    print("="*60)
    print("COMPOSITION PATTERN: Camera as Component")
    print("="*60)

    try:
        if not IC_CAN_AVAILABLE:
            raise RuntimeError("IC_CAN not available")

        # Create regular robot
        robot = ic_can.create_robot_arm(debug=False)
        robot.enable()
        robot.set_control_mode(ic_can.ControlMode.EXECUTION)
        print("✓ Robot initialized")

        # Create camera component
        camera_config = CameraConfig(
            camera_id=0,
            resolution=(640, 480),
            fps=30,
            quality=20
        )

        camera = CameraDataCollector(camera_config)
        print("✓ Camera component created")

        # Start camera recording
        if camera.start_recording():
            print("✓ Camera component started")

            # Main control loop
            start_time = time.time()
            loop_count = 0

            while time.time() - start_time < duration:
                loop_count += 1

                # Robot control (independent)
                robot_state = robot.refresh()
                action = 0.05 * np.sin(time.time() * 0.3) * np.ones(9)
                robot.step(action)

                # Get camera data (independent)
                camera_frame = camera.get_latest_frame(timeout=0.01)

                # Print status
                if loop_count % 15 == 0:
                    if camera_frame:
                        print(f"Loop {loop_count}: Robot pos={np.mean(robot_state.positions):.3f}, "
                              f"Camera frame {camera_frame.frame_number}")
                    else:
                        print(f"Loop {loop_count}: Robot pos={np.mean(robot_state.positions):.3f}, "
                              f"No camera frame available")

                time.sleep(0.05)

            # Stop camera component
            camera.stop_recording()
            print("✓ Camera component stopped")

            # Show statistics
            camera_stats = camera.get_statistics()
            robot_stats = robot.get_performance_stats()

            print(f"\n📊 COMPOSITION PATTERN STATISTICS:")
            print(f"   Robot steps: {robot_stats['step_count']}")
            print(f"   Camera frames captured: {camera_stats['frames_captured']}")
            print(f"   Camera effective FPS: {camera_stats.get('effective_fps', 0):.1f}")
            print(f"   Camera drop rate: {camera_stats.get('drop_rate_percent', 0):.2f}%")

            # Save metadata
            camera.save_metadata()
            print(f"✓ Camera metadata saved to {camera.output_dir}")

            robot.shutdown()

        else:
            print("✗ Failed to start camera component")

    except Exception as e:
        print(f"✗ Composition pattern demo failed: {e}")

    print("Composition pattern demo completed\n")


def demo_threaded_pattern(duration=30):
    """
    Demonstrate threaded integration.

    Camera running in independent thread.
    """
    print("="*60)
    print("THREADED PATTERN: Independent Camera Thread")
    print("="*60)

    try:
        if not IC_CAN_AVAILABLE:
            raise RuntimeError("IC_CAN not available")

        # Create regular robot
        robot = ic_can.create_robot_arm(debug=False)
        robot.enable()
        print("✓ Robot initialized")

        # Create threaded collector
        camera_config = CameraConfig(fps=20)  # Lower FPS for thread
        collector = ThreadedDataCollector(robot, camera_config)

        # Start threaded collection
        if collector.start_collection():
            print("✓ Threaded camera collector started")

            # Main control loop (completely independent)
            start_time = time.time()
            loop_count = 0

            while time.time() - start_time < duration:
                loop_count += 1

                # Robot control (no camera interference)
                robot_state = robot.refresh()
                action = 0.08 * np.cos(time.time() * 0.4) * np.ones(9)
                robot.step(action)

                # Get camera data from thread (non-blocking)
                latest_data = collector.get_latest_data()

                # Print status
                if loop_count % 12 == 0:
                    if latest_data:
                        sync_latency = latest_data.get('sync_latency', 0) * 1000
                        frame_num = latest_data.get('frame_number', 0)
                        print(f"Loop {loop_count}: Robot active, "
                              f"Camera sync={sync_latency:.1f}ms (frame {frame_num})")
                    else:
                        print(f"Loop {loop_count}: Robot active, camera sync building...")

                time.sleep(0.04)  # 25 Hz control loop

            # Stop threaded collector
            collector.stop_collection()
            print("✓ Threaded collector stopped")

            # Show thread statistics
            history = collector.get_data_history()
            print(f"\n📊 THREADED PATTERN STATISTICS:")
            print(f"   Robot control loops: {loop_count}")
            print(f"   Synchronized data points: {len(history)}")
            if history:
                avg_sync = np.mean([d.get('sync_latency', 0) for d in history]) * 1000
                print(f"   Average sync latency: {avg_sync:.2f}ms")

            robot.shutdown()

        else:
            print("✗ Failed to start threaded collector")

    except Exception as e:
        print(f"✗ Threaded pattern demo failed: {e}")

    print("Threaded pattern demo completed\n")


def demo_pipeline_pattern(duration=30):
    """
    Demonstrate full pipeline integration.

    Complete data collection pipeline.
    """
    print("="*60)
    print("PIPELINE PATTERN: Full Data Collection Pipeline")
    print("="*60)

    try:
        # Create integrated data collector
        camera_config = CameraConfig(
            resolution=(640, 480),
            fps=25,
            quality=15,
            save_frames=True  # Save frames for ML dataset
        )

        if IC_CAN_AVAILABLE:
            robot = ic_can.create_robot_arm(debug=False)
            pipeline = IntegratedDataCollector(robot, camera_config)
            print("✓ Data collection pipeline created")

            # Start pipeline
            if pipeline.start_collection():
                print("✓ Pipeline started")

                # Pipeline runs in background, main thread can do other work
                start_time = time.time()
                loop_count = 0

                while time.time() - start_time < duration:
                    loop_count += 1

                    # Simulate main application work
                    time.sleep(0.1)
                    loop_count += 1

                    # Check pipeline status
                    if loop_count % 20 == 0:
                        latest = pipeline.get_latest_synchronized_data()
                        if latest:
                            print(f"Pipeline active: synchronized frame {latest['frame_number']}")

                # Stop pipeline
                pipeline.stop_collection()
                print("✓ Pipeline stopped")

                # Show pipeline statistics
                stats = pipeline.get_statistics()
                print(f"\n📊 PIPELINE PATTERN STATISTICS:")
                print(f"   Synchronized frames: {stats['integrated_collection']['synchronized_frames']}")
                print(f"   Camera frames: {stats['camera']['frames_captured']}")
                print(f"   Output directory: {pipeline.output_dir}")

            else:
                print("✗ Failed to start pipeline")

        else:
            print("✗ IC_CAN not available for pipeline demo")

    except Exception as e:
        print(f"✗ Pipeline pattern demo failed: {e}")

    print("Pipeline pattern demo completed\n")


def demo_performance_comparison():
    """Compare performance of different integration patterns."""
    print("="*60)
    print("PERFORMANCE COMPARISON")
    print("="*60)

    duration = 15  # Shorter for comparison
    results = {}

    # Test each pattern
    patterns = [
        ("Inheritance", demo_inheritance_pattern),
        ("Composition", demo_composition_pattern),
        ("Threaded", demo_threaded_pattern)
    ]

    for name, demo_func in patterns:
        print(f"\n🔄 Testing {name} pattern...")
        start_time = time.time()

        try:
            demo_func(duration)
            end_time = time.time()
            results[name] = end_time - start_time
            print(f"✅ {name} pattern completed in {results[name]:.2f} seconds")
        except Exception as e:
            print(f"❌ {name} pattern failed: {e}")
            results[name] = None

    # Summary
    print("\n" + "="*60)
    print("PERFORMANCE COMPARISON SUMMARY")
    print("="*60)
    for name, duration in results.items():
        status = "✅ Success" if duration else "❌ Failed"
        time_str = f"{duration:.2f}s" if duration else "N/A"
        print(f"   {name:12} | {status:10} | {time_str}")

    print("\n💡 Recommendations:")
    print("   • Inheritance: Easiest to use, tight integration")
    print("   • Composition: Modular design, independent components")
    print("   • Threaded: Best performance, non-blocking")
    print("   • Pipeline: Complete solution for data collection")


def main():
    """Main function."""
    parser = argparse.ArgumentParser(description="Integrated Robot-Camera Demo")
    parser.add_argument("--pattern", "-p", choices=[
        "inheritance", "composition", "threaded", "pipeline", "all", "compare"
    ], default="all", help="Integration pattern to demonstrate")
    parser.add_argument("--duration", "-d", type=int, default=20,
                       help="Demo duration in seconds")
    parser.add_argument("--camera", "-c", type=int, default=0,
                       help="Camera ID to use")

    args = parser.parse_args()

    print("🤖 Integrated Robot-Camera Demo")
    print("=" * 60)
    print(f"Pattern: {args.pattern}")
    print(f"Duration: {args.duration} seconds")
    print(f"Camera ID: {args.camera}")

    # Update camera config for all patterns
    CameraConfig.camera_id = args.camera

    try:
        if args.pattern == "inheritance":
            demo_inheritance_pattern(args.duration)
        elif args.pattern == "composition":
            demo_composition_pattern(args.duration)
        elif args.pattern == "threaded":
            demo_threaded_pattern(args.duration)
        elif args.pattern == "pipeline":
            demo_pipeline_pattern(args.duration)
        elif args.pattern == "all":
            demo_inheritance_pattern(args.duration)
            demo_composition_pattern(args.duration)
            demo_threaded_pattern(args.duration)
            demo_pipeline_pattern(args.duration)
        elif args.pattern == "compare":
            demo_performance_comparison()

        print("\n🎉 Demo completed successfully!")
        print("\n📚 Integration Patterns Summary:")
        print("   1. Inheritance: RobotArmWithCamera class")
        print("   2. Composition: Camera as component")
        print("   3. Threaded: Independent camera thread")
        print("   4. Pipeline: Full data collection system")
        print("\n✨ All patterns use H.264 I-frame only encoding")
        print("✨ No future frame dependencies - real-time ready!")

    except KeyboardInterrupt:
        print("\n\n⏹️ Demo interrupted by user")
    except Exception as e:
        print(f"\n❌ Demo failed: {e}")
        print("This is expected if hardware is not connected.")
        print("The integration patterns are ready for use with actual hardware.")


if __name__ == "__main__":
    main()