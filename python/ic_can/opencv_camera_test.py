#!/usr/bin/env python3
"""
OpenCV Camera Test - Test different OpenCV backends and configurations
"""

import cv2
import time
import sys

def test_opencv_backends(camera_id=2):
    """Test different OpenCV backends to see if any provide better FPS."""
    print("🔍 Testing OpenCV Backends and Configurations")
    print("="*50)

    # Test different buffer sizes
    buffer_sizes = [1, 2, 5, 10]

    for buf_size in buffer_sizes:
        print(f"\n📷 Testing with buffer size {buf_size}...")

        cap = cv2.VideoCapture(camera_id)
        if not cap.isOpened():
            print(f"❌ Failed to open camera {camera_id}")
            continue

        # Configure for maximum performance
        cap.set(cv2.CAP_PROP_BUFFERSIZE, buf_size)
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
        cap.set(cv2.CAP_PROP_FPS, 60)

        # Test for 3 seconds
        start_time = time.time()
        frame_count = 0
        last_time = start_time

        try:
            while time.time() - start_time < 3.0:
                ret, frame = cap.read()
                if ret:
                    frame_count += 1

                    # Show FPS every second
                    current_time = time.time()
                    if current_time - last_time >= 1.0:
                        instant_fps = frame_count / (current_time - start_time)
                        print(f"   Buffer {buf_size}: {instant_fps:.1f} FPS")
                        last_time = current_time

                    # Add simple overlay
                    fps_text = f"FPS: {frame_count / (time.time() - start_time):.1f}" if frame_count > 0 else "FPS: --"
                    cv2.putText(frame, fps_text, (10, 30),
                               cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
                    cv2.putText(frame, f"Buffer: {buf_size}", (10, 70),
                               cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
                    cv2.putText(frame, f"Frame: {frame_count}", (10, 110),
                               cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

                    cv2.imshow('OpenCV Backend Test', frame)

                    if cv2.waitKey(1) & 0xFF == ord('q'):
                        break

        except KeyboardInterrupt:
            break

        finally:
            cap.release()
            cv2.destroyAllWindows()

        # Calculate results
        total_time = time.time() - start_time
        if total_time > 0:
            avg_fps = frame_count / total_time
            print(f"   Result: {avg_fps:.1f} FPS average")

    print(f"\n💡 Buffer Size Analysis:")
    print(f"   Try smaller buffers for lower latency")
    print(f"   Larger buffers provide smoother video but higher latency")

def test_gstreamer_backend(camera_id=2):
    """Test GStreamer backend if available."""
    print("\n🌊 Testing GStreamer Backend")
    print("="*50)

    # Test different GStreamer pipelines
    pipelines = [
        "v4l2src device=/dev/video2 ! videoconvert ! video/x-raw,format=BGR ! appsink",
        "v4l2src device=/dev/video2 ! video/x-raw,format=BGR ! videoconvert ! video/x-raw,format=BGR ! appsink",
        "v4l2src device=/dev/video2 ! video/x-raw,width=640,height=480,framerate=30,format=BGR ! appsink"
    ]

    for i, pipeline in enumerate(pipelines):
        print(f"\n📹 Pipeline {i+1}: {pipeline[:60]}...")

        try:
            cap = cv2.VideoCapture(pipeline, cv2.CAP_GSTREAMER)
            if not cap.is_opened():
                print(f"❌ Failed to open GStreamer pipeline")
                continue

            # Test for 3 seconds
            start_time = time.time()
            frame_count = 0

            try:
                while time.time() - start_time < 3.0:
                    ret, frame = cap.read()
                    if ret:
                        frame_count += 1
                        cv2.putText(frame, f"GStreamer - Frame {frame_count}", (10, 30),
                                   cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
                        cv2.imshow('GStreamer Test', frame)

                        if cv2.waitKey(1) & 0xFF == ord('q'):
                            break

            except KeyboardInterrupt:
                break

            finally:
                cap.release()
                cv2.destroyAllWindows()

            # Calculate results
            total_time = time.time() - start_time
            if total_time > 0:
                avg_fps = frame_count / total_time
                print(f"   Result: {avg_fps:.1f} FPS average")

        except Exception as e:
            print(f"   Error: {e}")

def test_ffmpeg_backend(camera_id=2):
    """Test FFmpeg backend if available."""
    print("\n🎬 Testing FFmpeg Backend")
    print("="*50)

    try:
        pipeline = "rtsp://localhost:8554/test"  # Would need RTSP server
        print("Note: FFmpeg backend requires RTSP streaming")
        print("This is not available for direct USB cameras")

    except Exception as e:
        print(f"FFmpeg not available or not configured: {e}")

def test_camera_direct_access(camera_id=2):
    """Test direct camera access methods."""
    print("\n🔧 Testing Direct Camera Access Methods")
    print("="*50)

    cap = cv2.VideoCapture(camera_id)
    if not cap.is_opened():
        print(f"❌ Failed to open camera {camera_id}")
        return

    # Get camera properties
    print("Camera Properties:")
    print(f"  Backend: {cap.get(cv2.CAP_PROP_BACKEND)}")
    print(f"  Native FPS: {cap.get(cv2.CAP_PROP_FPS)}")
    print(f"  Width: {cap.get(cv2.CAP_PROP_FRAME_WIDTH)}")
    print(f"  Height: {cap.get(cv2.CAP_PROP_FRAME_HEIGHT)}")
    print(f"  FourCC: {hex(cap.get(cv2.CAP_PROP_FOURCC))}")
    print(f"  Buffer Size: {cap.get(cv2.CAP_PROP_BUFFERSIZE)}")

    # Test if camera reports different FPS when we change settings
    print("\nTesting FPS settings:")
    test_fps = [15, 25, 30, 60, 120]

    for fps in test_fps:
        cap.set(cv2.CAP_PROP_FPS, fps)
        actual_fps = cap.get(cv2.CAP_PROP_FPS)
        print(f"  Set {fps} FPS -> Actual: {actual_fps} FPS")

    cap.release()

def test_high_performance_mode(camera_id=2):
    """Test high performance mode settings."""
    print("\n⚡ Testing High Performance Mode")
    print("="*50)

    cap = cv2.VideoCapture(camera_id)
    if not cap.is_opened():
        print(f"❌ Failed to open camera {camera_id}")
        return

    # Optimize for performance
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)  # Minimal buffering
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    cap.set(cv2.CAP_PROP_FPS, 60)

    # Test without display (just reading)
    print("Testing without display (pure capture)...")

    start_time = time.time()
    frame_count = 0

    try:
        while time.time() - start_time < 5.0:
            ret, frame = cap.read()
            if ret:
                frame_count += 1
            else:
                break

    except KeyboardInterrupt:
        break

    finally:
        cap.release()

    if frame_count > 0:
        pure_fps = frame_count / 5.0
        print(f"Pure capture speed: {pure_fps:.1f} FPS")

        if pure_fps > 10:
            print("✅ Camera CAN achieve higher FPS!")
            print("   The limitation is in OpenCV display/rendering")
            print("   Try running without cv2.imshow() or use a different approach")
        else:
            print("❌ Camera itself limited to 10 FPS")

def main():
    """Main function."""
    camera_id = 2  # Use camera 2 based on previous tests

    print("🔍 OpenCV Camera Backend Analysis")
    print("="*50)
    print(f"Testing camera {camera_id} with different Open configurations")
    print("This will help identify if OpenCV is the bottleneck")
    print()

    # Run tests
    test_opencv_backends(camera_id)
    test_gstreamer_backend(camera_id)
    test_ffmpeg_backend(camera_id)
    test_camera_direct_access(camera_id)
    test_high_performance_mode(camera_id)

    print(f"\n🎯 Analysis Summary:")
    print("If 'pure capture speed' is higher than 10 FPS:")
    print("   ✅ Camera hardware supports higher FPS")
    print("   ❌ OpenCV is the bottleneck")
    print("   💡 Solutions: Use GStreamer, FFmpeg, or direct V4L2")
    print()
    print("If 'pure capture speed' is also ~10 FPS:")
    print("   ❌ Camera hardware limited to 10 FPS")
    print("   💡 Need a different camera for 60Hz capability")

if __name__ == "__main__":
    main()