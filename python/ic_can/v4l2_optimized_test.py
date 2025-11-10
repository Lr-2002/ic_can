#!/usr/bin/env python3
"""
V4L2 Optimized Camera Test - Based on actual device capabilities
"""

import cv2
import time
import numpy as np
import subprocess

def test_specific_v4l2_config(camera_id: int = 2,
                              format_str: str = 'MJPG',
                              resolution: tuple = (640, 480),
                              target_fps: int = 60,
                              buffer_size: int = 5,
                              duration: int = 5) -> dict:
    """Test specific V4L2 configuration."""

    print(f"\n🎬 Testing V4L2 Configuration:")
    print(f"   Device: /dev/video{camera_id}")
    print(f"   Format: {format_str}")
    print(f"   Resolution: {resolution[0]}x{resolution[1]}")
    print(f"   Target FPS: {target_fps}")
    print(f"   Buffer Size: {buffer_size}")
    print(f"   Duration: {duration}s")

    # Configure camera using V4L2 backend
    cap = cv2.VideoCapture(camera_id, cv2.CAP_V4L)
    if not cap.isOpened():
        return {'error': f'Failed to open /dev/video{camera_id}'}

    # Set format
    if format_str == 'MJPG':
        fourcc = cv2.VideoWriter_fourcc(*'MJPG')
    elif format_str == 'YUYV':
        fourcc = cv2.VideoWriter_fourcc(*'YUY2')
    else:
        fourcc = -1

    if fourcc != -1:
        success = cap.set(cv2.CAP_PROP_FOURCC, fourcc)
        print(f"   Format set: {success}")

    # Configure parameters
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, resolution[0])
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, resolution[1])
    cap.set(cv2.CAP_PROP_FPS, target_fps)
    cap.set(cv2.CAP_PROP_BUFFERSIZE, buffer_size)

    # Verify actual settings
    actual_width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    actual_height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    actual_fps = cap.get(cv2.CAP_PROP_FPS)
    actual_fourcc = int(cap.get(cv2.CAP_PROP_FOURCC))
    actual_format = "".join([chr((actual_fourcc >> 8 * i) & 0xFF) for i in range(4)])
    actual_buffer = int(cap.get(cv2.CAP_PROP_BUFFERSIZE))

    print(f"   Actual: {actual_width}x{actual_height} @ {actual_fps} FPS ({actual_format}, buffer={actual_buffer})")

    # Test performance
    frame_times = []
    frame_count = 0
    start_time = time.time()

    print(f"   ⚡ Starting performance test...")

    try:
        while time.time() - start_time < duration:
            frame_start = time.time()

            ret, frame = cap.read()
            if not ret:
                print(f"   ❌ Failed to read frame at frame {frame_count}")
                break

            frame_time = time.time() - frame_start
            frame_times.append(frame_time)
            frame_count += 1

            # Show progress every second
            if frame_count % target_fps == 0:
                elapsed = time.time() - start_time
                current_fps = frame_count / elapsed if elapsed > 0 else 0
                print(f"   📊 {elapsed:.1f}s: {current_fps:.1f} FPS (avg: {np.mean(frame_times)*1000:.1f}ms)")

    except KeyboardInterrupt:
        print("   ⚠️ Test interrupted by user")

    cap.release()

    if frame_count > 0:
        total_time = time.time() - start_time
        achieved_fps = frame_count / total_time
        avg_frame_time = np.mean(frame_times) * 1000

        result = {
            'device': f'/dev/video{camera_id}',
            'format': actual_format,
            'resolution': (actual_width, actual_height),
            'target_fps': target_fps,
            'achieved_fps': achieved_fps,
            'frame_count': frame_count,
            'total_time': total_time,
            'avg_frame_time_ms': avg_frame_time,
            'min_frame_time_ms': min(frame_times) * 1000,
            'max_frame_time_ms': max(frame_times) * 1000,
            'buffer_size': actual_buffer,
            'success': True
        }

        print(f"\n   ✅ Test completed:")
        print(f"      Total frames: {frame_count}")
        print(f"      Duration: {total_time:.2f}s")
        print(f"      Achieved FPS: {achieved_fps:.1f}")
        print(f"      Frame time: {avg_frame_time:.1f}ms (min: {min(frame_times)*1000:.1f}ms, max: {max(frame_times)*1000:.1f}ms)")

        return result
    else:
        return {'error': 'No frames captured'}

def set_v4l2_controls(camera_id: int = 2):
    """Set optimal V4L2 controls for performance."""
    print(f"\n🎛️ Setting V4L2 controls for /dev/video{camera_id}...")

    device_path = f'/dev/video{camera_id}'

    # Try to set frame rate directly
    commands = [
        f"v4l2-ctl -d {device_path} --set-parm=60",
        f"v4l2-ctl -d {device_path} --set-ctrl=exposure_auto=1",  # Manual exposure
        f"v4l2-ctl -d {device_path} --set-ctrl=exposure_absolute=50",  # Fixed exposure
        f"v4l2-ctl -d {device_path} --set-ctrl=power_line_frequency=1",  # 50Hz
        f"v4l2-ctl -d {device_path} --set-ctrl=sharpness=128",  # Default sharpness
        f"v4l2-ctl -d {device_path} --set-ctrl=contrast=128",  # Default contrast
    ]

    for cmd in commands:
        try:
            result = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=5)
            if result.returncode == 0:
                print(f"   ✅ {cmd.split(' --')[-1]}")
            else:
                print(f"   ❌ {cmd.split(' --')[-1]}: {result.stderr.strip()}")
        except Exception as e:
            print(f"   ⚠️ {cmd.split(' --')[-1]}: {e}")

def main():
    """Main test function."""
    print("🎬 V4L2 Optimized Camera Test")
    print("="*50)
    print("Testing camera based on actual V4L2 capabilities")
    print()

    camera_id = 2

    # Set V4L2 controls for optimal performance
    set_v4l2_controls(camera_id)

    print(f"\n🎯 Testing Optimal Configurations for 60 FPS Performance")
    print("="*60)

    # Test configurations based on v4l2-ctl output
    test_configs = [
        # MJPG configurations (should achieve 60 FPS)
        ('MJPG', (640, 480), 60, 5),
        ('MJPG', (640, 480), 60, 2),
        ('MJPG', (640, 480), 60, 1),
        ('MJPG', (320, 240), 60, 5),
        ('MJPG', (1280, 720), 60, 5),

        # YUYV for comparison (should be 10 FPS)
        ('YUYV', (640, 480), 10, 5),
    ]

    results = []

    for format_str, resolution, target_fps, buffer_size in test_configs:
        result = test_specific_v4l2_config(
            camera_id, format_str, resolution, target_fps, buffer_size, duration=3
        )

        if 'error' not in result:
            results.append(result)

    print(f"\n🏆 Performance Analysis")
    print("="*50)

    if results:
        # Sort by achieved FPS
        results.sort(key=lambda x: x['achieved_fps'], reverse=True)

        print("All Results (sorted by performance):")
        for i, result in enumerate(results):
            print(f"{i+1}. {result['format']} {result['resolution'][0]}x{result['resolution'][1]} "
                  f"(buffer={result['buffer_size']}): {result['achieved_fps']:.1f} FPS")

        # Best result
        best = results[0]
        print(f"\n🥇 Best Configuration:")
        print(f"   Format: {best['format']}")
        print(f"   Resolution: {best['resolution'][0]}x{best['resolution'][1]}")
        print(f"   Buffer Size: {best['buffer_size']}")
        print(f"   Performance: {best['achieved_fps']:.1f} FPS")
        print(f"   Frame Time: {best['avg_frame_time_ms']:.1f} ms")

        # Performance assessment
        if best['achieved_fps'] >= 50:
            print(f"\n🎉 EXCELLENT! Camera achieves {best['achieved_fps']:.1f} FPS")
            print("   ✅ Ready for high-speed robotics (60Hz capability)")
            print("   ✅ Suitable for real-time control applications")
        elif best['achieved_fps'] >= 30:
            print(f"\n✅ GOOD! Camera achieves {best['achieved_fps']:.1f} FPS")
            print("   ✅ Suitable for real-time robotics")
            print("   ✅ Can sync with 500Hz robot control")
        else:
            print(f"\n⚠️ Limited performance: {best['achieved_fps']:.1f} FPS")
            print("   ❌ Not ideal for high-speed applications")

        # Show MJPG vs YUYV comparison
        mjpg_results = [r for r in results if r['format'] == 'MJPG']
        yuyv_results = [r for r in results if r['format'] == 'YUYV']

        if mjpg_results and yuyv_results:
            best_mjpg = max(mjpg_results, key=lambda x: x['achieved_fps'])
            best_yuyv = max(yuyv_results, key=lambda x: x['achieved_fps'])

            print(f"\n📊 Format Comparison:")
            print(f"   MJPG: {best_mjpg['achieved_fps']:.1f} FPS")
            print(f"   YUYV: {best_yuyv['achieved_fps']:.1f} FPS")
            print(f"   Improvement: {best_mjpg['achieved_fps']/best_yuyv['achieved_fps']:.1f}x")

    else:
        print("❌ No successful tests")

if __name__ == "__main__":
    main()