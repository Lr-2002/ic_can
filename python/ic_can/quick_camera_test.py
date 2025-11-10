#!/usr/bin/env python3
"""
Quick Camera Test

Simple script to quickly test camera functionality without complex setup.
"""

import cv2
import time
import argparse

def test_camera(camera_id=0, duration=10):
    """
    Test camera with basic preview.

    Args:
        camera_id: Camera device ID
        duration: Test duration in seconds
    """
    print(f"🎥 Testing camera {camera_id} for {duration} seconds...")

    # Initialize camera
    cap = cv2.VideoCapture(camera_id)

    if not cap.isOpened():
        print(f"❌ Failed to open camera {camera_id}")
        return False

    # Configure camera
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    cap.set(cv2.CAP_PROP_FPS, 30)

    # Get actual settings
    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    fps = cap.get(cv2.CAP_PROP_FPS)

    print(f"✅ Camera {camera_id} initialized: {width}x{height} @ {fps:.2f} FPS")

    # Test loop
    start_time = time.time()
    frame_count = 0

    print("📹 Showing preview... Press 'q' to quit early")

    while time.time() - start_time < duration:
        ret, frame = cap.read()

        if not ret:
            print("❌ Failed to read frame")
            break

        frame_count += 1

        # Add timestamp
        timestamp = time.time() - start_time
        cv2.putText(frame, f"Time: {timestamp:.1f}s", (10, 30),
                   cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        cv2.putText(frame, f"Frame: {frame_count}", (10, 70),
                   cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

        cv2.imshow(f'Camera {camera_id} Test', frame)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    # Cleanup
    cap.release()
    cv2.destroyAllWindows()

    # Calculate actual FPS
    actual_duration = time.time() - start_time
    actual_fps = frame_count / actual_duration if actual_duration > 0 else 0

    print(f"✅ Camera test completed!")
    print(f"   Frames captured: {frame_count}")
    print(f"   Actual duration: {actual_duration:.2f}s")
    print(f"   Actual FPS: {actual_fps:.2f}")

    return True

def list_cameras():
    """List available cameras."""
    print("📷 Scanning for cameras...")

    found_cameras = []
    for i in range(5):
        cap = cv2.VideoCapture(i)
        if cap.isOpened():
            ret, _ = cap.read()
            if ret:
                width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
                height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
                fps = cap.get(cv2.CAP_PROP_FPS)
                found_cameras.append((i, width, height, fps))
                print(f"  Camera {i}: {width}x{height} @ {fps:.2f} FPS")
            cap.release()

    if not found_cameras:
        print("  No cameras found")

    return found_cameras

def main():
    """Main function."""
    parser = argparse.ArgumentParser(description="Quick Camera Test")
    parser.add_argument("--camera", "-c", type=int, default=0,
                       help="Camera ID (default: 0)")
    parser.add_argument("--duration", "-d", type=int, default=10,
                       help="Test duration in seconds (default: 10)")
    parser.add_argument("--list", "-l", action="store_true",
                       help="List available cameras")

    args = parser.parse_args()

    if args.list:
        list_cameras()
        return

    print("🚀 Quick Camera Test")
    print("="*30)
    print(f"Camera ID: {args.camera}")
    print(f"Duration: {args.duration}s")
    print("="*30)

    # Test camera
    success = test_camera(args.camera, args.duration)

    if success:
        print("🎉 Camera test successful!")
    else:
        print("❌ Camera test failed!")

if __name__ == "__main__":
    main()