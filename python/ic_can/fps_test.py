#!/usr/bin/env python3
"""
Camera FPS Test - Test maximum camera frame rate capability
"""

import cv2
import time
import sys


def test_max_fps(camera_id=0, duration=5):
    """Test maximum FPS capability of camera."""
    print(f"🎥 Testing camera {camera_id} maximum FPS for {duration} seconds...")

    cap = cv2.VideoCapture(camera_id)
    if not cap.isOpened():
        print(f"❌ Failed to open camera {camera_id}")
        return False

    # Configure camera for maximum performance
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 720)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
    cap.set(cv2.CAP_PROP_FPS, 60)  # Try to set to 60 FPS

    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    native_fps = cap.get(cv2.CAP_PROP_FPS)

    print(f"✅ Camera {camera_id}: {width}x{height}, Native FPS: {native_fps}")

    # Test maximum FPS
    start_time = time.time()
    frame_count = 0
    last_time = start_time

    print("⚡ Measuring maximum FPS... (no artificial delays)")

    try:
        while time.time() - start_time < duration:
            ret, frame = cap.read()
            if ret:
                frame_count += 1

                # Show FPS every second
                current_time = time.time()
                if current_time - last_time >= 1.0:
                    instant_fps = frame_count / (current_time - start_time)
                    print(f"   Current FPS: {instant_fps:.1f}")
                    last_time = current_time

                # Add simple overlay
                fps_text = (
                    f"FPS: {frame_count / (time.time() - start_time):.1f}"
                    if frame_count > 0
                    else "FPS: --"
                )
                cv2.putText(
                    frame,
                    fps_text,
                    (10, 30),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    1,
                    (0, 255, 0),
                    2,
                )
                cv2.putText(
                    frame,
                    f"Frame: {frame_count}",
                    (10, 70),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    1,
                    (0, 255, 0),
                    2,
                )

                cv2.imshow(f"Camera {camera_id} FPS Test", frame)

                if cv2.waitKey(1) & 0xFF == ord("q"):
                    break

    except KeyboardInterrupt:
        print("Interrupted by user")

    finally:
        cap.release()
        cv2.destroyAllWindows()

    # Calculate results
    total_time = time.time() - start_time
    max_fps = frame_count / total_time if total_time > 0 else 0

    print(f"\n📊 FPS TEST RESULTS:")
    print(f"   Total frames: {frame_count}")
    print(f"   Duration: {total_time:.2f}s")
    print(f"   Maximum FPS: {max_fps:.2f}")
    print(f"   Native FPS: {native_fps:.2f}")

    # Analysis
    if max_fps >= 55:
        print(f"✅ EXCELLENT: Camera can do 60Hz! ({max_fps:.1f} FPS)")
    elif max_fps >= 30:
        print(f"✅ GOOD: Camera can do 30Hz+ ({max_fps:.1f} FPS)")
    elif max_fps >= 15:
        print(f"⚠️ FAIR: Camera can do 15Hz+ ({max_fps:.1f} FPS)")
    else:
        print(f"❌ LIMITED: Camera only {max_fps:.1f} FPS")

    return max_fps


if __name__ == "__main__":
    camera_id = 2  # Use camera 2 based on previous test
    print("🚀 Camera FPS Capability Test")
    print("=" * 40)

    max_fps = test_max_fps(camera_id, duration=5)

    print(f"\n💡 Recommendation:")
    if max_fps >= 30:
        print("   - Camera suitable for real-time robotics control")
        print("   - Can sync with 500Hz robot control (30Hz visual, 500Hz motor)")
    if max_fps >= 60:
        print("   - Camera can do 60Hz for high-speed applications")
        print("   - Perfect for fast motion capture")

