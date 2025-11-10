#!/usr/bin/env python3
"""
Simple Camera Test - Basic camera functionality without advanced features
"""

import cv2
import time
import sys
import os

def test_camera_basic(camera_id=0, duration=10):
    """Basic camera test without any threading or complex features."""
    print(f"🎥 Testing camera {camera_id} for {duration} seconds...")

    # Initialize camera
    cap = cv2.VideoCapture(camera_id)

    if not cap.isOpened():
        print(f"❌ Failed to open camera {camera_id}")
        return False

    # Configure camera
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)

    # Get actual settings
    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))

    print(f"✅ Camera {camera_id} initialized: {width}x{height}")

    # Test loop
    start_time = time.time()
    frame_count = 0

    try:
        while time.time() - start_time < duration:
            ret, frame = cap.read()

            if not ret:
                print("❌ Failed to read frame")
                break

            frame_count += 1

            # Add simple timestamp
            timestamp = time.time() - start_time
            cv2.putText(frame, f"Time: {timestamp:.1f}s", (10, 30),
                       cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
            cv2.putText(frame, f"Frame: {frame_count}", (10, 70),
                       cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

            # Show window
            cv2.imshow(f'Camera {camera_id} Test', frame)

            # Check for quit key
            if cv2.waitKey(1) & 0xFF == ord('q'):
                print("User quit early")
                break

            # Small delay
            time.sleep(0.03)  # ~30 FPS

    except KeyboardInterrupt:
        print("Interrupted by user")

    finally:
        # Cleanup
        cap.release()
        cv2.destroyAllWindows()

    # Calculate stats
    actual_duration = time.time() - start_time
    actual_fps = frame_count / actual_duration if actual_duration > 0 else 0

    print(f"✅ Camera test completed!")
    print(f"   Frames captured: {frame_count}")
    print(f"   Duration: {actual_duration:.2f}s")
    print(f"   FPS: {actual_fps:.2f}")

    return True

def list_cameras_simple():
    """Simple camera listing."""
    print("📷 Scanning for cameras...")
    found = []

    for i in range(5):
        try:
            cap = cv2.VideoCapture(i)
            if cap.isOpened():
                ret, _ = cap.read()
                if ret:
                    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
                    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
                    found.append((i, width, height))
                    print(f"  Camera {i}: {width}x{height}")
                cap.release()
        except:
            continue

    if not found:
        print("  No cameras found")
    else:
        print(f"Found {len(found)} camera(s)")

    return found

def main():
    """Main function."""
    import argparse

    parser = argparse.ArgumentParser(description="Simple Camera Test")
    parser.add_argument("--camera", "-c", type=int, default=0,
                       help="Camera ID (default: 0)")
    parser.add_argument("--duration", "-d", type=int, default=5,
                       help="Test duration in seconds (default: 5)")
    parser.add_argument("--list", "-l", action="store_true",
                       help="List available cameras")

    args = parser.parse_args()

    if args.list:
        list_cameras_simple()
        return

    print("🚀 Simple Camera Test")
    print("="*30)
    print(f"Camera ID: {args.camera}")
    print(f"Duration: {args.duration}s")
    print("="*30)

    success = test_camera_basic(args.camera, args.duration)

    if success:
        print("🎉 Test successful!")
    else:
        print("❌ Test failed!")

if __name__ == "__main__":
    main()