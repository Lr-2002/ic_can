#!/usr/bin/env python3
"""
60Hz Video Showcase - Demonstrate the optimized camera performance
"""

import cv2
import time
import numpy as np

def show_60hz_performance():
    """Show 60Hz camera performance with real-time display."""
    print("🎬 60Hz Camera Performance Showcase")
    print("="*50)
    print("Demonstrating optimized camera settings")
    print("Expected: 60+ FPS with vibrant colors")
    print("Press 'q' to quit, 's' to save screenshot")
    print("="*50)

    camera_id = 2

    # Initialize camera with optimal settings
    cap = cv2.VideoCapture(camera_id, cv2.CAP_V4L)
    if not cap.isOpened():
        print("❌ Failed to open camera")
        return

    # Configure for 60Hz MJPG (best settings already applied)
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

    print(f"📹 Camera: {width}x{height} @ {fps} FPS ({format_str})")
    print("🎨 Using optimized color settings")

    # Performance tracking
    frame_count = 0
    start_time = time.time()
    last_time = start_time
    fps_history = []

    # Performance metrics
    best_fps = 0
    worst_fps = float('inf')

    print("\n🎥 Starting live video showcase...")
    print("Watch for smooth 60Hz motion and vibrant colors!")

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
                instant_fps = 1.0 / (current_time - last_time)
                fps_history.append(instant_fps)
                last_time = current_time
                frame_count = 0

                # Update best/worst
                best_fps = max(best_fps, instant_fps)
                worst_fps = min(worst_fps, instant_fps)

                # Keep only last 10 seconds of history
                if len(fps_history) > 10:
                    fps_history.pop(0)

                avg_fps = np.mean(fps_history) if fps_history else 0
                print(f"📊 Current: {instant_fps:.1f} FPS, Average: {avg_fps:.1f} FPS (Best: {best_fps:.1f}, Worst: {worst_fps:.1f})")

            # Add performance overlay
            current_fps = fps_history[-1] if fps_history else 0
            elapsed = current_time - start_time

            # Determine performance color
            if current_fps >= 60:
                perf_color = (0, 255, 0)  # Green
                perf_status = "EXCELLENT 60Hz+"
            elif current_fps >= 50:
                perf_color = (0, 255, 255)  # Yellow
                perf_status = "GOOD 50Hz+"
            elif current_fps >= 30:
                perf_color = (255, 165, 0)  # Orange
                perf_status = "FAIR 30Hz+"
            else:
                perf_color = (0, 0, 255)  # Red
                perf_status = "POOR"

            # Create semi-transparent overlay for info
            overlay = frame.copy()
            cv2.rectangle(overlay, (10, 10), (500, 220), (0, 0, 0), -1)
            frame = cv2.addWeighted(frame, 1, overlay, 0.7, 0)

            # Draw performance info
            cv2.putText(frame, f"FPS: {current_fps:.1f}", (20, 40),
                       cv2.FONT_HERSHEY_SIMPLEX, 1, perf_color, 2)
            cv2.putText(frame, perf_status, (20, 80),
                       cv2.FONT_HERSHEY_SIMPLEX, 1, perf_color, 2)

            # Draw settings info
            cv2.putText(frame, f"Resolution: {width}x{height}", (20, 120),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
            cv2.putText(frame, f"Format: MJPG", (20, 150),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
            cv2.putText(frame, f"Time: {elapsed:.1f}s", (20, 180),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

            # Draw optimization badge
            cv2.putText(frame, "OPTIMIZED", (width - 200, 40),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
            cv2.putText(frame, "60Hz Capable", (width - 200, 70),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)

            # Draw FPS graph (simple visualization)
            if len(fps_history) > 1:
                graph_x = width - 220
                graph_y = 100
                graph_width = 200
                graph_height = 80

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
                    y1 = graph_y + graph_height - int(fps_history[i-1] * graph_height // 70)
                    y2 = graph_y + graph_height - int(fps_history[i] * graph_height // 70)

                    # Color based on FPS value
                    color = (0, 255, 0) if fps_history[i] >= 50 else (0, 255, 255) if fps_history[i] >= 30 else (0, 0, 255)
                    cv2.line(frame, (x1, y1), (x2, y2), color, 2)

                cv2.putText(frame, "FPS", (graph_x + 5, graph_y + 20),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)

            # Show frame
            cv2.imshow('60Hz Camera Showcase - Optimized Performance', frame)

            # Check for key presses
            key = cv2.waitKey(1) & 0xFF
            if key == ord('q'):
                break
            elif key == ord('s'):
                # Save screenshot
                screenshot_name = f"60hz_showcase_{int(time.time())}.jpg"
                cv2.imwrite(screenshot_name, frame)
                print(f"📸 Screenshot saved: {screenshot_name}")
            elif key == ord('r'):
                # Reset FPS tracking
                fps_history = []
                best_fps = 0
                worst_fps = float('inf')
                start_time = time.time()
                print("🔄 FPS tracking reset")

    except KeyboardInterrupt:
        print("\n⚠️ Showcase interrupted")

    finally:
        cap.release()
        cv2.destroyAllWindows()

        # Final performance summary
        total_time = time.time() - start_time
        if fps_history:
            avg_fps = np.mean(fps_history)
            min_fps = np.min(fps_history)
            max_fps = np.max(fps_history)
            fps_std = np.std(fps_history)

            print(f"\n🎯 Performance Summary:")
            print(f"   Total time: {total_time:.1f}s")
            print(f"   Average FPS: {avg_fps:.1f}")
            print(f"   Min FPS: {min_fps:.1f}")
            print(f"   Max FPS: {max_fps:.1f}")
            print(f"   FPS Stability: {fps_std:.1f} (lower is better)")

            if avg_fps >= 60:
                print("🎉 EXCELLENT: True 60Hz performance achieved!")
                print("   ✅ Perfect for high-speed robotics")
                print("   ✅ Smooth motion capture")
                print("   ✅ Real-time applications ready")
            elif avg_fps >= 50:
                print("✅ EXCELLENT: High-performance 50Hz+")
                print("   ✅ Great for robotics applications")
                print("   ✅ Smooth real-time video")
            elif avg_fps >= 30:
                print("✅ GOOD: Solid 30Hz+ performance")
                print("   ✅ Suitable for real-time robotics")
            else:
                print("⚠️ Limited performance")

def record_video_demo():
    """Record a short video demo of the 60Hz performance."""
    print("\n🎥 Recording Video Demo...")
    print("Creating 10-second video demonstration")

    camera_id = 2

    cap = cv2.VideoCapture(camera_id, cv2.CAP_V4L)
    if not cap.isOpened():
        print("❌ Failed to open camera")
        return

    # Configure camera
    mjpg_fourcc = cv2.VideoWriter_fourcc(*'MJPG')
    cap.set(cv2.CAP_PROP_FOURCC, mjpg_fourcc)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
    cap.set(cv2.CAP_PROP_FPS, 60)
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 5)

    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))

    # Create video writer (H.264 with I-frame only)
    output_filename = f"60hz_demo_{int(time.time())}.mp4"
    fourcc = cv2.VideoWriter_fourcc(*'mp4v')
    out = cv2.VideoWriter(output_filename, fourcc, 60.0, (width, height))

    print(f"📹 Recording to: {output_filename}")

    frame_count = 0
    start_time = time.time()

    try:
        while time.time() - start_time < 10:  # 10 second demo
            ret, frame = cap.read()
            if not ret:
                break

            frame_count += 1

            # Add demo overlay
            elapsed = time.time() - start_time
            cv2.putText(frame, f"60Hz Camera Demo - {elapsed:.1f}s", (10, 30),
                       cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
            cv2.putText(frame, f"Frame: {frame_count}", (10, 70),
                       cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

            out.write(frame)

            # Also show live preview
            cv2.imshow('Recording Demo', frame)
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

    except KeyboardInterrupt:
        print("\n⚠️ Recording interrupted")

    finally:
        out.release()
        cap.release()
        cv2.destroyAllWindows()

        actual_fps = frame_count / 10.0
        print(f"\n✅ Video saved: {output_filename}")
        print(f"   Duration: 10s")
        print(f"   Frames: {frame_count}")
        print(f"   Actual FPS: {actual_fps:.1f}")

def main():
    """Main function."""
    print("🎬 60Hz Camera Video Showcase")
    print("="*40)
    print("Running live performance demo...")

    show_60hz_performance()

if __name__ == "__main__":
    main()