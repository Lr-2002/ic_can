#!/usr/bin/env python3
"""
Simple 60Hz Camera Demo - Visualization of high-speed camera performance
"""

import cv2
import time
import numpy as np

def simple_60hz_demo():
    """Simple demonstration of 60Hz camera with visualization."""
    print("🎬 60Hz Camera Demo with Visualization")
    print("="*50)

    camera_id = 2

    # Initialize camera with V4L2 backend and MJPG format
    cap = cv2.VideoCapture(camera_id, cv2.CAP_V4L)
    if not cap.isOpened():
        print("❌ Failed to open camera")
        return

    # Configure for 60Hz performance
    mjpg_fourcc = cv2.VideoWriter_fourcc(*'MJPG')
    cap.set(cv2.CAP_PROP_FOURCC, mjpg_fourcc)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
    cap.set(cv2.CAP_PROP_FPS, 60)
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 5)

    # Verify settings
    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    fps = cap.get(cv2.CAP_PROP_FPS)
    fourcc = int(cap.get(cv2.CAP_PROP_FOURCC))
    format_str = "".join([chr((fourcc >> 8 * i) & 0xFF) for i in range(4)])

    print(f"Camera: {width}x{height} @ {fps} FPS ({format_str})")

    # Performance tracking
    frame_count = 0
    start_time = time.time()
    last_time = start_time
    fps_history = []

    print("📹 Starting visualization (press 'q' to quit)...")

    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                print("❌ Failed to read frame")
                break

            frame_count += 1
            current_time = time.time()

            # Calculate FPS every second
            if current_time - last_time >= 1.0:
                instant_fps = 1.0 / (current_time - last_time) if frame_count > 0 else 0
                fps_history.append(instant_fps)
                last_time = current_time
                frame_count = 0

                # Keep only last 10 seconds of history
                if len(fps_history) > 10:
                    fps_history.pop(0)

                avg_fps = np.mean(fps_history) if fps_history else 0
                print(f"📊 Current: {instant_fps:.1f} FPS, Average: {avg_fps:.1f} FPS")

            # Add visualization overlay
            current_fps = fps_history[-1] if fps_history else 0

            # FPS indicator
            if current_fps >= 50:
                fps_color = (0, 255, 0)  # Green
                status = "EXCELLENT 60Hz"
            elif current_fps >= 30:
                fps_color = (0, 255, 255)  # Yellow
                status = "GOOD 30Hz+"
            else:
                fps_color = (0, 0, 255)  # Red
                status = "LIMITED"

            # Draw FPS info
            cv2.putText(frame, f"FPS: {current_fps:.1f}", (10, 30),
                       cv2.FONT_HERSHEY_SIMPLEX, 1, fps_color, 2)
            cv2.putText(frame, status, (10, 70),
                       cv2.FONT_HERSHEY_SIMPLEX, 1, fps_color, 2)

            # Draw resolution
            cv2.putText(frame, f"{width}x{height} MJPG", (10, 110),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

            # Draw timestamp
            elapsed = current_time - start_time
            cv2.putText(frame, f"Time: {elapsed:.1f}s", (10, 150),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

            # Draw FPS graph (simple)
            if len(fps_history) > 1:
                graph_x = width - 250
                graph_y = 30
                graph_width = 200
                graph_height = 100

                # Graph background
                cv2.rectangle(frame, (graph_x, graph_y),
                             (graph_x + graph_width, graph_y + graph_height),
                             (0, 0, 0), -1)
                cv2.rectangle(frame, (graph_x, graph_y),
                             (graph_x + graph_width, graph_y + graph_height),
                             (255, 255, 255), 1)

                # Plot FPS history
                for i in range(1, len(fps_history)):
                    x1 = graph_x + (i-1) * graph_width // 10
                    x2 = graph_x + i * graph_width // 10
                    y1 = graph_y + graph_height - int(fps_history[i-1] * graph_height // 60)
                    y2 = graph_y + graph_height - int(fps_history[i] * graph_height // 60)
                    cv2.line(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)

                cv2.putText(frame, "FPS", (graph_x + 5, graph_y + 20),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)

            # Show frame
            cv2.imshow('60Hz Camera Demo', frame)

            # Check for quit
            key = cv2.waitKey(1) & 0xFF
            if key == ord('q'):
                break
            elif key == ord('s'):
                # Save screenshot
                screenshot_name = f"camera_demo_{int(time.time())}.jpg"
                cv2.imwrite(screenshot_name, frame)
                print(f"📸 Screenshot saved: {screenshot_name}")

    except KeyboardInterrupt:
        print("\n⚠️ Demo interrupted")

    finally:
        cap.release()
        cv2.destroyAllWindows()

        # Final statistics
        total_time = time.time() - start_time
        if fps_history:
            avg_fps = np.mean(fps_history)
            min_fps = np.min(fps_history)
            max_fps = np.max(fps_history)

            print(f"\n📊 Final Statistics:")
            print(f"   Total time: {total_time:.1f}s")
            print(f"   Average FPS: {avg_fps:.1f}")
            print(f"   Min FPS: {min_fps:.1f}")
            print(f"   Max FPS: {max_fps:.1f}")

            if avg_fps >= 50:
                print("🎉 EXCELLENT: Camera ready for 60Hz robotics!")
            elif avg_fps >= 30:
                print("✅ GOOD: Camera suitable for real-time robotics")
            else:
                print("⚠️ Limited performance")

if __name__ == "__main__":
    simple_60hz_demo()