#!/usr/bin/env python3
"""
Color Optimized 60Hz Camera - Better colors while maintaining high speed
"""

import cv2
import time
import numpy as np
import subprocess

def set_optimal_camera_settings(camera_id: int = 2):
    """Set optimal camera settings for better color quality."""
    device_path = f'/dev/video{camera_id}'

    print("🎨 Optimizing camera settings for better color quality...")

    # Camera control settings for better colors
    settings = [
        # Basic image adjustments
        ("brightness", 140),      # Slightly brighter
        ("contrast", 130),        # Better contrast
        ("saturation", 140),      # More vibrant colors
        ("sharpness", 140),       # Sharper image

        # White balance controls
        ("white_balance_auto", 1),   # Auto white balance
        ("white_balance_temperature", 4500),  # Warm white balance

        # Exposure settings
        ("exposure_auto", 1),         # Manual exposure mode
        ("exposure_absolute", 100),   # Balanced exposure

        # Other enhancements
        ("power_line_frequency", 1),  # 50Hz for less flicker
        ("gain", 50),                 # Moderate gain
        ("gamma", 120),               # Better gamma curve
    ]

    success_count = 0
    for setting_name, value in settings:
        try:
            cmd = f"v4l2-ctl -d {device_path} --set-ctrl={setting_name}={value}"
            result = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=5)

            if result.returncode == 0:
                print(f"   ✅ {setting_name}: {value}")
                success_count += 1
            else:
                print(f"   ❌ {setting_name}: {value} - {result.stderr.strip()}")

        except Exception as e:
            print(f"   ⚠️ {setting_name}: {e}")

    print(f"   📊 Applied {success_count}/{len(settings)} settings successfully")
    return success_count > 0

def apply_color_correction(frame: np.ndarray) -> np.ndarray:
    """Apply software color correction to improve MJPG colors."""
    # Convert to different color space for better manipulation
    hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

    # Increase saturation for more vibrant colors
    hsv[:, :, 1] = np.clip(hsv[:, :, 1] * 1.2, 0, 255)

    # Adjust brightness slightly
    hsv[:, :, 2] = np.clip(hsv[:, :, 2] * 1.05, 0, 255)

    # Convert back to BGR
    corrected_frame = cv2.cvtColor(hsv, cv2.COLOR_HSV2BGR)

    # Apply slight contrast enhancement
    alpha = 1.1  # Contrast control (1.0-3.0)
    beta = 5     # Brightness control (0-100)

    corrected_frame = cv2.convertScaleAbs(corrected_frame, alpha=alpha, beta=beta)

    return corrected_frame

def test_color_optimized_camera():
    """Test color optimized 60Hz camera."""
    print("🎬 Color Optimized 60Hz Camera Demo")
    print("="*50)

    camera_id = 2

    # Set optimal camera settings first
    set_optimal_camera_settings(camera_id)

    # Initialize camera
    cap = cv2.VideoCapture(camera_id, cv2.CAP_V4L)
    if not cap.isOpened():
        print("❌ Failed to open camera")
        return

    # Configure for 60Hz performance with MJPG
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

    # Color correction mode
    color_correction_enabled = True

    print("📹 Starting color-optimized demo (press 'c' to toggle color correction, 'q' to quit)...")

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

                if len(fps_history) > 10:
                    fps_history.pop(0)

                avg_fps = np.mean(fps_history) if fps_history else 0
                print(f"📊 Current: {instant_fps:.1f} FPS, Average: {avg_fps:.1f} FPS")

            # Apply color correction if enabled
            display_frame = frame.copy()
            if color_correction_enabled:
                display_frame = apply_color_correction(display_frame)

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
            cv2.putText(display_frame, f"FPS: {current_fps:.1f}", (10, 30),
                       cv2.FONT_HERSHEY_SIMPLEX, 1, fps_color, 2)
            cv2.putText(display_frame, status, (10, 70),
                       cv2.FONT_HERSHEY_SIMPLEX, 1, fps_color, 2)

            # Draw settings info
            cv2.putText(display_frame, f"{width}x{height} MJPG", (10, 110),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

            # Color correction status
            correction_text = f"Color Correction: {'ON' if color_correction_enabled else 'OFF'}"
            correction_color = (0, 255, 0) if color_correction_enabled else (0, 0, 255)
            cv2.putText(display_frame, correction_text, (10, 140),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, correction_color, 2)

            # Draw timestamp
            elapsed = current_time - start_time
            cv2.putText(display_frame, f"Time: {elapsed:.1f}s", (10, 170),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

            # Split screen comparison (original vs corrected)
            if color_correction_enabled:
                # Create side-by-side comparison
                comparison_frame = np.zeros((height, width * 2, 3), dtype=np.uint8)
                comparison_frame[:, :width] = frame
                comparison_frame[:, width:] = display_frame

                # Add labels
                cv2.putText(comparison_frame, "Original", (10, height - 10),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
                cv2.putText(comparison_frame, "Corrected", (width + 10, height - 10),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

                display_frame = comparison_frame

            # Show frame
            cv2.imshow('Color Optimized 60Hz Camera', display_frame)

            # Check for key presses
            key = cv2.waitKey(1) & 0xFF
            if key == ord('q'):
                break
            elif key == ord('c'):
                # Toggle color correction
                color_correction_enabled = not color_correction_enabled
                print(f"🎨 Color correction {'enabled' if color_correction_enabled else 'disabled'}")
            elif key == ord('s'):
                # Save screenshot
                screenshot_name = f"color_optimized_{int(time.time())}.jpg"
                cv2.imwrite(screenshot_name, display_frame)
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
                print("🎉 EXCELLENT: Camera ready for 60Hz robotics with optimized colors!")
            elif avg_fps >= 30:
                print("✅ GOOD: Camera suitable for real-time robotics")
            else:
                print("⚠️ Limited performance")

def show_available_controls(camera_id: int = 2):
    """Show available camera controls."""
    device_path = f'/dev/video{camera_id}'

    print(f"🎛️ Available controls for {device_path}:")

    try:
        result = subprocess.run(f"v4l2-ctl -d {device_path} --list-ctrls",
                              shell=True, capture_output=True, text=True, timeout=10)

        if result.returncode == 0:
            lines = result.stdout.split('\n')

            # Group controls by category
            for line in lines:
                if any(keyword in line.lower() for keyword in [
                    'brightness', 'contrast', 'saturation', 'sharpness',
                    'white_balance', 'exposure', 'gain', 'gamma', 'hue'
                ]):
                    print(f"   {line.strip()}")
        else:
            print(f"   Error: {result.stderr}")

    except Exception as e:
        print(f"   Error: {e}")

if __name__ == "__main__":
    # Show available controls first
    show_available_controls()
    print()

    # Run color optimized demo
    test_color_optimized_camera()