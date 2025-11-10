#!/usr/bin/env python3
"""
Simple Color Test - No threading, just basic camera with optimized settings
"""

import cv2
import time

def simple_color_test():
    """Test simple camera with color optimization."""
    print("🎬 Simple Color Test (No Threading)")
    print("="*40)

    camera_id = 2

    # Initialize camera
    cap = cv2.VideoCapture(camera_id, cv2.CAP_V4L)
    if not cap.isOpened():
        print("❌ Failed to open camera")
        return

    # Configure for MJPG 60Hz
    mjpg_fourcc = cv2.VideoWriter_fourcc(*'MJPG')
    cap.set(cv2.CAP_PROP_FOURCC, mjpg_fourcc)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
    cap.set(cv2.CAP_PROP_FPS, 60)
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 5)

    # Check settings
    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    fps = cap.get(cv2.CAP_PROP_FPS)
    fourcc = int(cap.get(cv2.CAP_PROP_FOURCC))
    format_str = "".join([chr((fourcc >> 8 * i) & 0xFF) for i in range(4)])

    print(f"Camera: {width}x{height} @ {fps} FPS ({format_str})")

    print("📹 Testing color quality (press 'q' to quit)...")

    frame_count = 0
    start_time = time.time()

    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                print("❌ Failed to read frame")
                break

            frame_count += 1

            # Simple FPS calculation
            if frame_count % 30 == 0:
                elapsed = time.time() - start_time
                current_fps = frame_count / elapsed if elapsed > 0 else 0
                print(f"📊 FPS: {current_fps:.1f}")

            # Add simple info overlay
            elapsed = time.time() - start_time
            cv2.putText(frame, f"FPS: {frame_count/(elapsed+0.001):.1f}", (10, 30),
                       cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
            cv2.putText(frame, f"Frame: {frame_count}", (10, 70),
                       cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
            cv2.putText(frame, f"Time: {elapsed:.1f}s", (10, 110),
                       cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

            # Show the frame
            cv2.imshow('Simple Color Test', frame)

            # Check for quit
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

    except KeyboardInterrupt:
        print("\n⚠️ Interrupted")

    finally:
        cap.release()
        cv2.destroyAllWindows()

        # Final stats
        total_time = time.time() - start_time
        avg_fps = frame_count / total_time if total_time > 0 else 0

        print(f"\n📊 Results:")
        print(f"   Total frames: {frame_count}")
        print(f"   Duration: {total_time:.1f}s")
        print(f"   Average FPS: {avg_fps:.1f}")

        if avg_fps >= 50:
            print("🎉 EXCELLENT: 60Hz performance with optimized colors!")
        elif avg_fps >= 30:
            print("✅ GOOD: Real-time performance")
        else:
            print("⚠️ Limited performance")

if __name__ == "__main__":
    simple_color_test()