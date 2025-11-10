#!/usr/bin/env python3
"""
V4L2 Camera Test - Direct Video4Linux2 camera control for maximum performance
"""

import cv2
import time
import numpy as np
import subprocess
import re
from typing import Dict, List, Tuple, Optional

def run_command(cmd: str) -> Tuple[int, str, str]:
    """Run shell command and return exit code, stdout, stderr."""
    try:
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=10)
        return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return -1, "", "Command timed out"
    except Exception as e:
        return -1, "", str(e)

def check_v4l2_devices() -> List[Dict]:
    """Check available V4L2 devices."""
    print("📹 Scanning V4L2 devices...")

    # Get list of video devices
    exitcode, stdout, stderr = run_command("v4l2-ctl --list-devices")

    devices = []
    current_device = {}

    if exitcode == 0:
        for line in stdout.split('\n'):
            line = line.strip()

            # Device info line
            if line.startswith('/dev/video'):
                if current_device:
                    devices.append(current_device)
                current_device = {'path': line, 'info': []}

            # Card info
            elif line and 'card' in line.lower():
                if current_device:
                    current_device['card'] = line

            # Store other info
            elif current_device:
                current_device['info'].append(line)

        if current_device:
            devices.append(current_device)

    return devices

def get_camera_capabilities(device_path: str) -> Dict:
    """Get detailed camera capabilities using v4l2-ctl."""
    print(f"\n🔍 Analyzing {device_path} capabilities...")

    capabilities = {'device_path': device_path}

    # Get device info
    exitcode, stdout, stderr = run_command(f"v4l2-ctl -d {device_path} --info")
    if exitcode == 0:
        for line in stdout.split('\n'):
            if 'Driver' in line:
                capabilities['driver'] = line.split(':')[1].strip()
            elif 'Card' in line:
                capabilities['card'] = line.split(':')[1].strip()
            elif 'Bus Info' in line:
                capabilities['bus_info'] = line.split(':')[1].strip()

    # Get supported formats
    exitcode, stdout, stderr = run_command(f"v4l2-ctl -d {device_path} --list-formats")
    if exitcode == 0:
        formats = []
        for line in stdout.split('\n'):
            if 'Pixel Format' in line or '[' in line:
                formats.append(line.strip())
        capabilities['formats'] = formats

    # Get supported frame sizes for each format
    exitcode, stdout, stderr = run_command(f"v4l2-ctl -d {device_path} --list-formats-ext")
    if exitcode == 0:
        format_sizes = {}
        current_format = None

        for line in stdout.split('\n'):
            line = line.strip()

            if 'Pixel Format' in line:
                # Extract format name
                match = re.search(r"'([^']+)'", line)
                if match:
                    current_format = match.group(1)
                    format_sizes[current_format] = []

            elif current_format and ('Size:' in line or 'Frame Size' in line):
                # Extract frame size
                match = re.search(r'(\d+)x(\d+)', line)
                if match:
                    width, height = int(match.group(1)), int(match.group(2))
                    format_sizes[current_format].append((width, height))

        capabilities['format_sizes'] = format_sizes

    # Get supported frame intervals
    exitcode, stdout, stderr = run_command(f"v4l2-ctl -d {device_path} --list-frame-sizes")
    if exitcode == 0:
        frame_sizes = []
        for line in stdout.split('\n'):
            if 'x' in line and any(char.isdigit() for char in line):
                match = re.search(r'(\d+)x(\d+)', line)
                if match:
                    frame_sizes.append((int(match.group(1)), int(match.group(2))))
        capabilities['frame_sizes'] = frame_sizes

    return capabilities

def test_v4l2_performance(device_path: str, format_str: str = 'MJPG',
                         resolution: Tuple[int, int] = (640, 480),
                         buffer_size: int = 5) -> Dict:
    """Test V4L2 performance with specific settings."""
    print(f"\n⚡ Testing V4L2 performance: {format_str} {resolution[0]}x{resolution[1]}...")

    # Convert device path to camera ID
    camera_id = int(device_path.split('video')[-1])

    # Test with OpenCV V4L2 backend
    cap = cv2.VideoCapture(camera_id, cv2.CAP_V4L)
    if not cap.isOpened():
        return {'error': f'Failed to open {device_path}'}

    # Set format
    if format_str == 'MJPG':
        fourcc = cv2.VideoWriter_fourcc(*'MJPG')
    elif format_str == 'YUY2':
        fourcc = cv2.VideoWriter_fourcc(*'YUY2')
    elif format_str == 'RGB3':
        fourcc = cv2.VideoWriter_fourcc(*'RGB3')
    else:
        fourcc = -1

    if fourcc != -1:
        cap.set(cv2.CAP_PROP_FOURCC, fourcc)

    # Configure camera
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, resolution[0])
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, resolution[1])
    cap.set(cv2.CAP_PROP_BUFFERSIZE, buffer_size)
    cap.set(cv2.CAP_PROP_FPS, 60)

    # Verify settings
    actual_width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    actual_height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    actual_fps = cap.get(cv2.CAP_PROP_FPS)
    actual_fourcc = int(cap.get(cv2.CAP_PROP_FOURCC))
    actual_format = "".join([chr((actual_fourcc >> 8 * i) & 0xFF) for i in range(4)])

    # Test performance
    start_time = time.time()
    frame_count = 0
    frame_times = []
    test_duration = 5

    print(f"   Configured: {actual_width}x{actual_height} @ {actual_fps} FPS ({actual_format})")

    while time.time() - start_time < test_duration:
        frame_start = time.time()

        ret, frame = cap.read()
        if ret:
            frame_time = time.time() - frame_start
            frame_times.append(frame_time)
            frame_count += 1
        else:
            break

    cap.release()

    if frame_count > 0:
        avg_fps = frame_count / test_duration
        avg_frame_time = np.mean(frame_times) * 1000

        result = {
            'format': actual_format,
            'resolution': (actual_width, actual_height),
            'target_fps': actual_fps,
            'achieved_fps': avg_fps,
            'frame_count': frame_count,
            'avg_frame_time_ms': avg_frame_time,
            'min_frame_time_ms': min(frame_times) * 1000,
            'max_frame_time_ms': max(frame_times) * 1000,
            'buffer_size': buffer_size
        }

        print(f"   📊 Result: {avg_fps:.1f} FPS (avg: {avg_frame_time:.1f} ms)")
        return result

    return {'error': 'No frames captured'}

def optimize_v4l2_settings(device_path: str) -> Dict:
    """Find optimal V4L2 settings for maximum performance."""
    print(f"\n🎯 Optimizing V4L2 settings for {device_path}...")

    capabilities = get_camera_capabilities(device_path)

    # Test different formats and resolutions
    test_configs = []

    # Get available formats
    formats = capabilities.get('formats', [])
    for format_line in formats:
        if 'MJPG' in format_line:
            test_configs.append(('MJPG', (640, 480)))
            test_configs.append(('MJPG', (320, 240)))
        elif 'YUY2' in format_line or 'YUYV' in format_line:
            test_configs.append(('YUY2', (640, 480)))
        elif 'RGB' in format_line:
            test_configs.append(('RGB3', (640, 480)))

    # If no specific formats found, test defaults
    if not test_configs:
        test_configs = [('MJPG', (640, 480)), ('YUY2', (640, 480))]

    results = []

    for format_str, resolution in test_configs:
        print(f"\n🔧 Testing {format_str} {resolution[0]}x{resolution[1]}...")

        for buffer_size in [1, 2, 5, 10]:
            result = test_v4l2_performance(device_path, format_str, resolution, buffer_size)

            if 'error' not in result:
                result['buffer_size'] = buffer_size
                results.append(result)

    # Find best configuration
    if results:
        best = max(results, key=lambda x: x['achieved_fps'])

        print(f"\n🏆 Best Configuration Found:")
        print(f"   Format: {best['format']}")
        print(f"   Resolution: {best['resolution'][0]}x{best['resolution'][1]}")
        print(f"   Buffer Size: {best['buffer_size']}")
        print(f"   Performance: {best['achieved_fps']:.1f} FPS")
        print(f"   Frame Time: {best['avg_frame_time_ms']:.1f} ms")

        return {
            'best_config': best,
            'all_results': results,
            'capabilities': capabilities
        }

    return {'error': 'No valid configurations found'}

def test_v4l2_direct_control(device_path: str):
    """Test direct V4L2 control for optimal settings."""
    print(f"\n🎛️ Testing direct V4L2 control for {device_path}...")

    # Get current controls
    exitcode, stdout, stderr = run_command(f"v4l2-ctl -d {device_path} --list-ctrls")

    if exitcode == 0:
        print("Available controls:")
        for line in stdout.split('\n'):
            if any(keyword in line.lower() for keyword in ['fps', 'frame', 'exposure', 'gain']):
                print(f"  {line.strip()}")

    # Try to set FPS directly
    exitcode, stdout, stderr = run_command(f"v4l2-ctl -d {device_path} --set-parm=60")
    if exitcode == 0:
        print("✅ Set frame parameter to 60 FPS")
    else:
        print(f"❌ Failed to set frame parameter: {stderr}")

    # Try to disable auto-exposure for better performance
    exitcode, stdout, stderr = run_command(f"v4l2-ctl -d {device_path} --set-ctrl=exposure_auto=1")
    if exitcode == 0:
        print("✅ Set exposure to manual mode")

    # Test performance after control changes
    result = test_v4l2_performance(device_path, 'MJPG', (640, 480), 5)

    return result

def main():
    """Main V4L2 test function."""
    print("🎬 V4L2 Camera Performance Test")
    print("="*50)
    print("Using Video4Linux2 for direct camera control")
    print()

    # Check available devices
    devices = check_v4l2_devices()

    if not devices:
        print("❌ No V4L2 devices found")
        return

    print(f"Found {len(devices)} V4L2 device(s):")
    for device in devices:
        print(f"  📹 {device['path']}")
        if 'card' in device:
            print(f"     {device['card']}")

    # Test each device
    for device in devices:
        device_path = device['path']

        # Get detailed capabilities
        capabilities = get_camera_capabilities(device_path)

        print(f"\n📊 Device Capabilities for {device_path}:")
        if 'driver' in capabilities:
            print(f"  Driver: {capabilities['driver']}")
        if 'card' in capabilities:
            print(f"  Card: {capabilities['card']}")
        if 'formats' in capabilities:
            print(f"  Supported formats: {len(capabilities['formats'])}")
            for fmt in capabilities['formats'][:3]:  # Show first 3
                print(f"    {fmt}")

        # Find optimal settings
        optimization = optimize_v4l2_settings(device_path)

        if 'best_config' in optimization:
            best = optimization['best_config']

            # Test direct V4L2 control
            direct_result = test_v4l2_direct_control(device_path)

            print(f"\n🎯 Final Results for {device_path}:")
            print(f"  Optimized Performance: {best['achieved_fps']:.1f} FPS")
            print(f"  Format: {best['format']}")
            print(f"  Resolution: {best['resolution'][0]}x{best['resolution'][1]}")
            print(f"  Buffer: {best['buffer_size']}")

            if 'achieved_fps' in direct_result:
                print(f"  Direct Control: {direct_result['achieved_fps']:.1f} FPS")

            # Performance assessment
            if best['achieved_fps'] >= 50:
                print("  ✅ EXCELLENT: Near 60Hz capability!")
                print("     Ready for high-speed robotics applications")
            elif best['achieved_fps'] >= 30:
                print("  ✅ GOOD: 30Hz+ capability")
                print("     Suitable for real-time robotics")
            elif best['achieved_fps'] >= 15:
                print("  ⚠️ FAIR: 15Hz+ capability")
                print("     Limited but usable for slow applications")
            else:
                print("  ❌ LIMITED: Under 15Hz")
                print("     Hardware limitation")

        else:
            print(f"❌ Failed to optimize {device_path}")

if __name__ == "__main__":
    main()