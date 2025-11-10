#!/usr/bin/env python3
"""
USB Camera Analyzer - Check USB version and camera capabilities

This script helps identify:
- USB version for connected devices
- Camera capabilities and limitations
- Bandwidth and connection speed information
"""

import cv2
import subprocess
import re
import sys
import os
from typing import Dict, List, Tuple

def run_command(cmd: str) -> Tuple[int, str, str]:
    """Run shell command and return exit code, stdout, stderr."""
    try:
        result = subprocess.run(
            cmd, shell=True, capture_output=True, text=True, timeout=10
        )
        return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return -1, "", "Command timed out"
    except Exception as e:
        return -1, "", str(e)

def check_usb_devices() -> List[Dict]:
    """Check connected USB devices and their capabilities."""
    print("🔍 Scanning USB devices...")

    devices = []

    # Method 1: lsusb (Linux)
    if os.path.exists("/usr/bin/lsusb"):
        print("Using lsusb to scan USB devices...")
        exitcode, stdout, stderr = run_command("lsusb -v")

        if exitcode == 0:
            devices = parse_lsubusb_output(stdout)
        else:
            print(f"lsusb error: {stderr}")

    # Method 2: usb-devices (if available)
    if not devices and os.path.exists("/sys/bus/usb/devices/"):
        print("Using sysfs to scan USB devices...")
        devices = parse_sysfs_usb_devices()

    # Method 3: udevadm (if available)
    if not devices:
        exitcode, stdout, stderr = run_command("udevadm info --export")
        if exitcode == 0:
            devices = parse_udev_output(stdout)

    return devices

def parse_lsubusb_output(output: str) -> List[Dict]:
    """Parse lsusb -v output to extract USB device information."""
    devices = []
    current_device = {}

    for line in output.split('\n'):
        line = line.strip()

        # Start of new device
        if line.startswith('Bus ') and 'Device ' in line:
            if current_device:
                devices.append(current_device)
            current_device = {'type': 'usb', 'details': []}

            # Extract bus and device info
            match = re.search(r'Bus (\d+).*Device (\d+):', line)
            if match:
                current_device['bus'] = int(match.group(1))
                current_device['device'] = int(match.group(2))

        # USB version
        elif 'bcdUSB' in line:
            match = re.search(r'bcdUSB\s+(\d+\.\d+)', line)
            if match:
                current_device['usb_version'] = float(match.group(1))

        # Device description
        elif 'iProduct' in line:
            match = re.search(r'iProduct\s+\d+ (.+)', line)
            if match:
                current_device['product'] = match.group(1).strip()

        # Max power
        elif 'MaxPower' in line:
            match = re.search(r'MaxPower\s+(\d+)mA', line)
            if match:
                current_device['max_power_ma'] = int(match.group(1))

        # Speed
        elif 'Speed' in line:
            match = re.search(r'Speed\s+(\d+)Mb/s', line)
            if match:
                current_device['speed_mbps'] = int(match.group(1))

        # Store details for debugging
        if 'current_device' in locals() and current_device:
            current_device['details'].append(line)

    # Add last device
    if current_device:
        devices.append(current_device)

    return devices

def parse_sysfs_usb_devices() -> List[Dict]:
    """Parse /sys/bus/usb/devices/ for USB device information."""
    devices = []

    try:
        # Find USB device directories
        result = subprocess.run(
            ["find", "/sys/bus/usb/devices/", "-name", "usb*", "-print"],
            capture_output=True, text=True, timeout=5
        )

        if result.returncode == 0:
            for device_path in result.stdout.split('\n'):
                if device_path.strip():
                    device_info = parse_sysfs_device(device_path)
                    if device_info:
                        devices.append(device_info)

    except Exception as e:
        print(f"sysfs scan error: {e}")

    return devices

def parse_sysfs_device(device_path: str) -> Dict:
    """Parse individual sysfs device path."""
    device = {'type': 'usb', 'path': device_path}

    try:
        # USB version
        version_file = os.path.join(device_path, "version")
        if os.path.exists(version_file):
            with open(version_file, 'r') as f:
                version = f.read().strip()
                if version.startswith('USB '):
                    device['usb_version'] = float(version.split(' ')[1])

        # Product name
        product_file = os.path.join(device_path, "product")
        if os.path.exists(product_file):
            with open(product_file, 'r') as f:
                device['product'] = f.read().strip()

        # Speed
        speed_file = os.path.join(device_path, "speed")
        if os.path.exists(speed_file):
            with open(speed_file, 'r') as f:
                speed = f.read().strip()
                if speed.endswith('Mbps'):
                    device['speed_mbps'] = int(speed.replace('Mbps', ''))

        # Max power
        power_file = os.path.join(device_path, "maxpower")
        if os.path.exists(power_file):
            with open(power_file, 'r') as f:
                power = f.read().strip()
                if power.endswith('mA'):
                    device['max_power_ma'] = int(power.replace('mA', ''))

    except Exception as e:
        print(f"Error parsing {device_path}: {e}")

    return device

def parse_udev_output(output: str) -> List[Dict]:
    """Parse udevadm output for USB devices."""
    devices = []
    current_device = {}

    for line in output.split('\n'):
        line = line.strip()

        if line.startswith('ID_BUS=usb'):
            if current_device:
                devices.append(current_device)
            current_device = {'type': 'usb'}

        elif line.startswith('ID_VENDOR_ID='):
            current_device['vendor_id'] = line.split('=')[1]
        elif line.startswith('ID_MODEL_ID='):
            current_device['model_id'] = line.split('=')[1]
        elif line.startswith('ID_SERIAL='):
            current_device['serial'] = line.split('=')[1]

    if current_device:
        devices.append(current_device)

    return devices

def check_camera_capabilities(camera_id: int) -> Dict:
    """Check specific camera capabilities."""
    print(f"\n📷 Testing Camera {camera_id} capabilities...")

    cap = cv2.VideoCapture(camera_id)
    if not cap.isOpened():
        return {'error': f"Cannot open camera {camera_id}"}

    capabilities = {
        'camera_id': camera_id,
        'supported': True,
        'resolutions': [],
        'fps_values': []
    }

    # Get basic properties
    capabilities['width'] = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    capabilities['height'] = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    capabilities['native_fps'] = cap.get(cv2.CAP_PROP_FPS)

    # Test common resolutions
    common_resolutions = [
        (320, 240), (640, 480), (800, 600),
        (1024, 768), (1280, 720), (1920, 1080)
    ]

    print("Testing supported resolutions...")
    for width, height in common_resolutions:
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)

        actual_width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        actual_height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))

        if actual_width == width and actual_height == height:
            capabilities['resolutions'].append((width, height))
            print(f"  ✅ {width}x{height}")
        else:
            print(f"  ❌ {width}x{height} (got {actual_width}x{actual_height})")

    # Test FPS values
    print("Testing FPS capabilities...")
    test_fps_values = [15, 30, 60, 120]

    for fps in test_fps_values:
        cap.set(cv2.CAP_PROP_FPS, fps)
        actual_fps = cap.get(cv2.CAP_PROP_FPS)

        if abs(actual_fps - fps) < 2:  # Allow some tolerance
            capabilities['fps_values'].append(fps)
            print(f"  ✅ {fps} FPS (actual: {actual_fps:.1f})")
        else:
            print(f"  ❌ {fps} FPS (actual: {actual_fps:.1f})")

    # Test backend
    backends = {
        cv2.CAP_V4L: 'V4L2',
        cv2.CAP_DSHOW: 'DirectShow',
        cv2.CAP_MSMF: 'Media Foundation',
        cv2.CAP_AVFOUNDATION: 'AVFoundation'
    }

    for backend_const, backend_name in backends.items():
        if int(cap.get(cv2.CAP_PROP_BACKEND)) == backend_const:
            capabilities['backend'] = backend_name
            break

    cap.release()
    return capabilities

def analyze_usb_version_for_camera(device_info: Dict, camera_capabilities: Dict) -> Dict:
    """Analyze if USB version is limiting camera performance."""
    analysis = {
        'usb_version': device_info.get('usb_version', 'Unknown'),
        'usb_speed_mbps': device_info.get('speed_mbps', 'Unknown'),
        'limitations': [],
        'recommendations': []
    }

    # Check USB version
    usb_version = device_info.get('usb_version', 0)
    if usb_version:
        if usb_version >= 3.0:
            analysis['usb_version_name'] = 'USB 3.0+'
            analysis['theoretical_max_mbps'] = 5000  # USB 3.0/3.1/3.2
        elif usb_version >= 2.0:
            if usb_version >= 2.5:
                analysis['usb_version_name'] = 'USB 2.0 High-Speed'
                analysis['theoretical_max_mbps'] = 480
            else:
                analysis['usb_version_name'] = 'USB 2.0'
                analysis['theoretical_max_mbps'] = 12
        else:
            analysis['usb_version_name'] = 'USB 1.x'
            analysis['theoretical_max_mbps'] = 1.5

    # Check actual speed
    speed = device_info.get('speed_mbps', 0)
    if speed > 0:
        analysis['actual_speed_mbps'] = speed

    # Check camera limitations
    max_fps = max(camera_capabilities.get('fps_values', [0]))
    max_resolution = max(camera_capabilities.get('resolutions', [(0, 0)]))
    pixel_count = max_resolution[0] * max_resolution[1]

    # Calculate bandwidth requirement
    if pixel_count > 0 and max_fps > 0:
        # Rough estimate: 24-bit color * 3 channels (RGB)
        bandwidth_mbps = (pixel_count * 3 * max_fps * 8) / (1024 * 1024)
        analysis['estimated_bandwidth_mbps'] = bandwidth_mbps

        if analysis.get('actual_speed_mbps', 0) > 0:
            if bandwidth_mbps > analysis['actual_speed_mbps'] * 0.8:
                analysis['limitations'].append(
                    f"Camera requires {bandwidth_mbps:.1f} Mbps but USB only provides {analysis['actual_speed_mbps']} Mbps"
                )

    # Add recommendations
    if usb_version < 3.0 and max_fps < 30:
        analysis['recommendations'].append(
            "Camera FPS limited by USB 2.0. Consider USB 3.0 camera for higher frame rates."
        )

    if max_resolution[0] < 1280:
        analysis['recommendations'].append(
            "Limited resolution. Consider higher resolution camera."
        )

    return analysis

def print_system_info():
    """Print system USB information."""
    print("🖥️ System USB Information")
    print("="*50)

    # Check USB controllers
    if os.path.exists("/usr/bin/lsusb"):
        exitcode, stdout, stderr = run_command("lsusb")
        if exitcode == 0:
            print("USB Controllers:")
            for line in stdout.split('\n'):
                if 'Bus ' in line and 'Hub' in line:
                    print(f"  {line}")

    # Check PCI USB controllers
    exitcode, stdout, stderr = run_command("lspci | grep -i usb")
    if exitcode == 0 and stdout.strip():
        print("\nUSB PCI Controllers:")
        print(stdout)

    # Check kernel USB modules
    exitcode, stdout, stderr = run_command("lsmod | grep -E 'usb|uvc|v4l'")
    if exitcode == 0 and stdout.strip():
        print("\nLoaded USB Modules:")
        for line in stdout.split('\n'):
            print(f"  {line}")

def main():
    """Main function."""
    print("🔌 USB Camera Analyzer")
    print("="*50)

    # Print system info
    print_system_info()

    # Scan USB devices
    usb_devices = check_usb_devices()

    print(f"\n📊 Found {len(usb_devices)} USB device(s):")
    print("="*50)

    camera_devices = []
    for i, device in enumerate(usb_devices):
        print(f"\nDevice {i+1}:")
        print(f"  Product: {device.get('product', 'Unknown')}")
        print(f"  USB Version: {device.get('usb_version', 'Unknown')}")
        print(f"  Speed: {device.get('speed_mbps', 'Unknown')} Mbps")
        print(f"  Max Power: {device.get('max_power_ma', 'Unknown')} mA")

        # Look for camera indicators
        product_name = device.get('product', '').lower()
        if any(keyword in product_name for keyword in ['camera', 'webcam', 'video', 'cam']):
            camera_devices.append(device)
            print(f"  📷 Likely camera device!")

    # Check camera capabilities
    camera_id = 2  # Based on previous tests
    print(f"\n📷 Testing Camera {camera_id}")
    print("="*50)

    camera_caps = check_camera_capabilities(camera_id)

    if 'error' in camera_caps:
        print(f"❌ Camera test failed: {camera_caps['error']}")
        return

    print(f"Camera Capabilities:")
    print(f"  Resolution: {camera_caps['width']}x{camera_caps['height']}")
    print(f"  Native FPS: {camera_caps['native_fps']}")
    print(f"  Backend: {camera_caps.get('backend', 'Unknown')}")
    print(f"  Supported Resolutions: {len(camera_caps['resolutions'])}")
    print(f"  Supported FPS: {camera_caps['fps_values']}")

    # Find USB device for camera
    camera_usb_device = None
    for device in usb_devices:
        if device.get('product', '').lower() in ['camera', 'webcam']:
            camera_usb_device = device
            break

    # Analyze limitations
    if camera_usb_device:
        print(f"\n🔍 Analysis for Camera {camera_id}")
        print("="*50)

        analysis = analyze_usb_version_for_camera(camera_usb_device, camera_caps)

        print(f"USB Version: {analysis['usb_version_name']} (v{analysis['usb_version']})")
        print(f"USB Speed: {analysis.get('actual_speed_mbps', 'Unknown')} Mbps")
        print(f"Theoretical Max: {analysis['theoretical_max_mbps']} Mbps")

        if 'estimated_bandwidth_mbps' in analysis:
            print(f"Camera Bandwidth: {analysis['estimated_bandwidth_mbps']:.1f} Mbps")

        if analysis['limitations']:
            print("\n⚠️ Limitations Detected:")
            for limitation in analysis['limitations']:
                print(f"  - {limitation}")

        if analysis['recommendations']:
            print("\n💡 Recommendations:")
            for rec in analysis['recommendations']:
                print(f"  - {rec}")

    print(f"\n🎯 Summary:")
    if camera_caps['fps_values']:
        max_fps = max(camera_caps['fps_values'])
        print(f"  Camera supports up to {max_fps} FPS")
        if max_fps >= 30:
            print("  ✅ Good for real-time applications")
        if max_fps >= 60:
            print("  ✅ Supports high-speed (60Hz)")
        else:
            print("  ⚠️ Limited frame rate for high-speed applications")

    if camera_usb_device:
        usb_ver = camera_usb_device.get('usb_version', 0)
        if usb_ver >= 3.0:
            print("  ✅ USB 3.0+ connection (high bandwidth)")
        else:
            print("  ⚠️ USB 2.0 connection (may limit performance)")

if __name__ == "__main__":
    main()