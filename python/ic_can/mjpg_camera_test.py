#!/usr/bin/env python3
"""
MJPG Camera Test - Test MJPG format for higher FPS
"""

import cv2
import time
import numpy as np

def test_mjpg_format(camera_id=2, duration=3):
    """Test MJPG format for higher FPS."""
    print("🎬 Testing MJPG Format for Higher FPS")
    print("="*50)

    # MJPG FourCC code
    mjpg_fourcc = cv2.VideoWriter_fourcc(*'MJPG')
    yuy2_fourcc = cv2.VideoWriter_fourcc(*'YUY2')

    formats_to_test = [
        ("MJPG", mjpg_fourcc),
        ("YUY2", yuy2_fourcc),
        ("Default", -1),
    ]

    results = []

    for format_name, fourcc in formats_to_test:
        print(f"\n📹 Testing {format_name} format...")

        cap = cv2.VideoCapture(camera_id)
        if not cap.isOpened():
            print(f"❌ Failed to open camera {camera_id}")
            continue

        # Set format
        if fourcc != -1:
            cap.set(cv2.CAP_PROP_FOURCC, fourcc)

        # Configure for performance
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
        cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
        cap.set(cv2.CAP_PROP_FPS, 60)

        # Check actual format
        actual_fourcc = int(cap.get(cv2.CAP_PROP_FOURCC))
        actual_fourcc_str = "".join([chr((actual_fourcc >> 8 * i) & 0xFF) for i in range(4)])

        width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
        native_fps = cap.get(cv2.CAP_PROP_FPS)

        print(f"   Resolution: {width}x{height}")
        print(f"   Format: {actual_fourcc_str}")
        print(f"   Native FPS: {native_fps}")

        # Test pure capture speed
        start_time = time.time()
        frame_count = 0

        while time.time() - start_time < duration:
            ret, frame = cap.read()
            if ret:
                frame_count += 1
            else:
                break

        fps = frame_count / duration
        results.append((format_name, fps, actual_fourcc_str))
        print(f"   📊 Achieved: {fps:.1f} FPS")

        cap.release()

    return results

def test_mjpg_resolutions(camera_id=2):
    """Test MJPG format at different resolutions."""
    print("\n🎬 Testing MJPG at Different Resolutions")
    print("="*50)

    mjpg_fourcc = cv2.VideoWriter_fourcc(*'MJPG')

    resolutions = [
        (320, 240, "QVGA"),
        (640, 480, "VGA"),
        (800, 600, "SVGA"),
        (1280, 720, "HD"),
        (1920, 1080, "Full HD"),
    ]

    results = []

    for width, height, name in resolutions:
        print(f"\n📹 Testing MJPG {name} ({width}x{height})...")

        cap = cv2.VideoCapture(camera_id)
        if not cap.isOpened():
            print(f"❌ Failed to open camera")
            continue

        # Set MJPG format
        cap.set(cv2.CAP_PROP_FOURCC, mjpg_fourcc)
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
        cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
        cap.set(cv2.CAP_PROP_FPS, 60)

        # Check actual settings
        actual_width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        actual_height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
        actual_fourcc = int(cap.get(cv2.CAP_PROP_FOURCC))
        actual_fourcc_str = "".join([chr((actual_fourcc >> 8 * i) & 0xFF) for i in range(4)])

        # Test FPS
        start_time = time.time()
        frame_count = 0
        test_duration = 2

        while time.time() - start_time < test_duration:
            ret, frame = cap.read()
            if ret:
                frame_count += 1
            else:
                break

        fps = frame_count / test_duration

        result = {
            "name": name,
            "requested": f"{width}x{height}",
            "actual": f"{actual_width}x{actual_height}",
            "format": actual_fourcc_str,
            "fps": fps
        }
        results.append(result)

        print(f"   Actual: {actual_width}x{actual_height} ({actual_fourcc_str})")
        print(f"   📊 FPS: {fps:.1f}")

        cap.release()

    return results

def test_mjpg_buffer_sizes(camera_id=2):
    """Test MJPG with different buffer sizes."""
    print("\n🎬 Testing MJPG with Different Buffer Sizes")
    print("="*50)

    mjpg_fourcc = cv2.VideoWriter_fourcc(*'MJPG')
    buffer_sizes = [1, 2, 5, 10]

    results = []

    for buf_size in buffer_sizes:
        print(f"\n📹 Testing MJPG with buffer size {buf_size}...")

        cap = cv2.VideoCapture(camera_id)
        if not cap.isOpened():
            print(f"❌ Failed to open camera")
            continue

        cap.set(cv2.CAP_PROP_FOURCC, mjpg_fourcc)
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
        cap.set(cv2.CAP_PROP_BUFFERSIZE, buf_size)
        cap.set(cv2.CAP_PROP_FPS, 60)

        # Test FPS
        start_time = time.time()
        frame_count = 0
        test_duration = 2

        while time.time() - start_time < test_duration:
            ret, frame = cap.read()
            if ret:
                frame_count += 1
            else:
                break

        fps = frame_count / test_duration
        results.append((buf_size, fps))
        print(f"   📊 Buffer {buf_size}: {fps:.1f} FPS")

        cap.release()

    return results

def test_mjpg_direct_pipeline(camera_id=2, duration=5):
    """Test MJPG with direct pipeline (no display)."""
    print("\n🎬 Testing MJPG Direct Pipeline (No Display)")
    print("="*50)

    mjpg_fourcc = cv2.VideoWriter_fourcc(*'MJPG')

    cap = cv2.VideoCapture(camera_id)
    if not cap.isOpened():
        print(f"❌ Failed to open camera {camera_id}")
        return 0

    # Configure for maximum performance
    cap.set(cv2.CAP_PROP_FOURCC, mjpg_fourcc)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
    cap.set(cv2.CAP_PROP_FPS, 60)

    # Check actual format
    actual_fourcc = int(cap.get(cv2.CAP_PROP_FOURCC))
    actual_fourcc_str = "".join([chr((actual_fourcc >> 8 * i) & 0xFF) for i in range(4)])

    print(f"Camera configured with MJPG format: {actual_fourcc_str}")
    print(f"Resolution: {int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))}x{int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))}")
    print(f"Native FPS: {cap.get(cv2.CAP_PROP_FPS)}")

    print(f"\n⚡ Testing pure MJPG capture speed for {duration} seconds...")

    start_time = time.time()
    frame_count = 0
    frame_times = []

    while time.time() - start_time < duration:
        frame_start = time.time()

        ret, frame = cap.read()
        if ret:
            frame_count += 1
            frame_time = time.time() - frame_start
            frame_times.append(frame_time)
        else:
            break

    if frame_count > 0:
        avg_fps = frame_count / duration
        avg_frame_time = np.mean(frame_times) * 1000  # Convert to ms

        print(f"\n📊 MJPG Pipeline Results:")
        print(f"   Total frames: {frame_count}")
        print(f"   Average FPS: {avg_fps:.1f}")
        print(f"   Average frame time: {avg_frame_time:.2f} ms")
        print(f"   Min frame time: {min(frame_times)*1000:.2f} ms")
        print(f"   Max frame time: {max(frame_times)*1000:.2f} ms")

        if avg_fps >= 30:
            print("✅ EXCELLENT: MJPG provides 30+ FPS!")
            print("   Suitable for real-time robotics applications")
        elif avg_fps >= 15:
            print("✅ GOOD: MJPG provides 15+ FPS")
            print("   Usable for moderate-speed applications")
        else:
            print("⚠️ LIMITED: Still under 15 FPS")
            print("   Hardware limitation remains")

    cap.release()
    return avg_fps if frame_count > 0 else 0

def main():
    """Main test function."""
    print("🎬 MJPG Camera Format Test")
    print("="*50)
    print("Testing MJPG format to potentially achieve higher FPS")
    print()

    camera_id = 2

    # Test different formats
    format_results = test_mjpg_format(camera_id)

    # Test MJPG at different resolutions
    resolution_results = test_mjpg_resolutions(camera_id)

    # Test MJPG with different buffer sizes
    buffer_results = test_mjpg_buffer_sizes(camera_id)

    # Test MJPG direct pipeline
    mjpg_fps = test_mjpg_direct_pipeline(camera_id)

    print("\n🎯 MJPG ANALYSIS SUMMARY")
    print("="*50)

    print("Format Comparison:")
    for format_name, fps, actual_format in format_results:
        print(f"  {format_name} ({actual_format}): {fps:.1f} FPS")

    print("\nBest Resolution Performance:")
    if resolution_results:
        best = max(resolution_results, key=lambda x: x['fps'])
        print(f"  {best['name']} ({best['actual']}): {best['fps']:.1f} FPS")

    print("\nBest Buffer Size:")
    if buffer_results:
        best_buffer = max(buffer_results, key=lambda x: x[1])
        print(f"  Buffer size {best_buffer[0]}: {best_buffer[1]:.1f} FPS")

    print(f"\nFinal MJPG Performance: {mjpg_fps:.1f} FPS")

    if mjpg_fps > 15:
        print("✅ MJPG format improved performance!")
        print("💡 Use MJPG format for camera capture in robot applications")
    else:
        print("❌ MJPG format did not significantly improve performance")
        print("💡 Camera hardware limitation cannot be overcome with format changes")

if __name__ == "__main__":
    main()