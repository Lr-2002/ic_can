#!/usr/bin/env python3
"""
Simple OpenCV Bottleneck Test - Test if OpenCV is limiting camera FPS
"""

import cv2
import time
import numpy as np

def test_pure_capture_speed(camera_id=2, duration=3):
    """Test camera capture speed without any display overhead."""
    print(f"🔬 Testing pure capture speed (no display)...")

    cap = cv2.VideoCapture(camera_id)
    if not cap.isOpened():
        print(f"❌ Failed to open camera {camera_id}")
        return 0

    # Configure for maximum performance
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)  # Minimal buffering
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    cap.set(cv2.CAP_PROP_FPS, 60)

    print(f"Camera configured: {int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))}x{int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))}")
    print(f"Target FPS: {cap.get(cv2.CAP_PROP_FPS)}")

    # Test 1: Pure capture speed (no display)
    start_time = time.time()
    frame_count = 0

    while time.time() - start_time < duration:
        ret, frame = cap.read()
        if ret:
            frame_count += 1
        else:
            break

    pure_capture_fps = frame_count / duration
    print(f"📊 Pure Capture Speed: {pure_capture_fps:.1f} FPS")

    # Test 2: Capture with simple processing (no display)
    start_time = time.time()
    frame_count = 0

    while time.time() - start_time < duration:
        ret, frame = cap.read()
        if ret:
            # Simple processing (draw rectangle)
            cv2.rectangle(frame, (50, 50), (100, 100), (0, 255, 0), 2)
            frame_count += 1
        else:
            break

    processing_fps = frame_count / duration
    print(f"📊 Capture + Simple Processing: {processing_fps:.1f} FPS")

    cap.release()
    return pure_capture_fps, processing_fps

def test_display_overhead(camera_id=2, duration=3):
    """Test display overhead by adding cv2.imshow()."""
    print(f"🖥️ Testing display overhead...")

    cap = cv2.VideoCapture(camera_id)
    if not cap.isOpened():
        print(f"❌ Failed to open camera {camera_id}")
        return 0

    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

    start_time = time.time()
    frame_count = 0

    try:
        while time.time() - start_time < duration:
            ret, frame = cap.read()
            if ret:
                cv2.putText(frame, f"Frame: {frame_count}", (10, 30),
                           cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
                cv2.imshow('Display Test', frame)

                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break

                frame_count += 1
            else:
                break
    except KeyboardInterrupt:
        pass

    display_fps = frame_count / duration
    print(f"📊 Capture + Display: {display_fps:.1f} FPS")

    cap.release()
    cv2.destroyAllWindows()
    return display_fps

def test_different_configurations(camera_id=2):
    """Test different camera configurations."""
    print(f"⚙️ Testing different configurations...")

    configs = [
        {"buffer": 1, "width": 320, "height": 240, "name": "Low Res"},
        {"buffer": 1, "width": 640, "height": 480, "name": "Medium Res"},
        {"buffer": 1, "width": 1280, "height": 720, "name": "High Res"},
        {"buffer": 5, "width": 640, "height": 480, "name": "Large Buffer"},
    ]

    results = []

    for config in configs:
        print(f"\n🔧 Testing {config['name']}...")

        cap = cv2.VideoCapture(camera_id)
        if not cap.isOpened():
            print(f"❌ Failed to open camera")
            continue

        cap.set(cv2.CAP_PROP_BUFFERSIZE, config['buffer'])
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, config['width'])
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, config['height'])

        # Test capture speed
        start_time = time.time()
        frame_count = 0
        duration = 2

        while time.time() - start_time < duration:
            ret, frame = cap.read()
            if ret:
                frame_count += 1
            else:
                break

        fps = frame_count / duration
        actual_width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        actual_height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
        actual_buffer = int(cap.get(cv2.CAP_PROP_BUFFERSIZE))

        result = {
            "name": config['name'],
            "fps": fps,
            "resolution": f"{actual_width}x{actual_height}",
            "buffer": actual_buffer
        }
        results.append(result)

        print(f"   📊 {result['name']}: {fps:.1f} FPS ({result['resolution']}, buffer={result['buffer']})")

        cap.release()

    return results

def check_opencv_backend():
    """Check OpenCV backend information."""
    print("🔍 OpenCV Backend Information")
    print("="*40)

    # Check OpenCV version
    print(f"OpenCV Version: {cv2.__version__}")

    # Check common backends that are likely to be available
    backends = {
        cv2.CAP_ANY: "ANY",
        cv2.CAP_GSTREAMER: "GStreamer",
        cv2.CAP_FFMPEG: "FFmpeg",
    }

    # Add backends that might not be available in all OpenCV versions
    try:
        backends[cv2.CAP_V4L] = "V4L"
    except AttributeError:
        pass

    try:
        backends[cv2.CAP_DSHOW] = "DirectShow"
    except AttributeError:
        pass

    try:
        backends[cv2.CAP_MSMF] = "Media Foundation"
    except AttributeError:
        pass

    try:
        backends[cv2.CAP_AVFOUNDATION] = "AVFoundation"
    except AttributeError:
        pass

    cap = cv2.VideoCapture(2)
    if cap.isOpened():
        backend = int(cap.get(cv2.CAP_PROP_BACKEND))
        backend_name = backends.get(backend, f"Unknown ({backend})")
        print(f"Current Backend: {backend_name}")

        # Check camera properties
        print(f"Camera Properties:")
        print(f"  Resolution: {int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))}x{int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))}")
        print(f"  Native FPS: {cap.get(cv2.CAP_PROP_FPS)}")
        print(f"  FourCC: {hex(int(cap.get(cv2.CAP_PROP_FOURCC)))}")
        print(f"  Buffer Size: {int(cap.get(cv2.CAP_PROP_BUFFERSIZE))}")
        print(f"  Auto Focus: {cap.get(cv2.CAP_PROP_AUTOFOCUS)}")
        print(f"  Auto Exposure: {cap.get(cv2.CAP_PROP_AUTO_EXPOSURE)}")

        cap.release()
    else:
        print("Failed to open camera for backend info")

def main():
    """Main test function."""
    print("🎯 OpenCV Camera Bottleneck Analysis")
    print("="*50)
    print("This test will help identify if OpenCV is limiting your camera FPS")
    print()

    camera_id = 2

    # Check OpenCV backend
    check_opencv_backend()
    print()

    # Test pure capture speed
    pure_fps, processing_fps = test_pure_capture_speed(camera_id)
    print()

    # Test display overhead
    display_fps = test_display_overhead(camera_id)
    print()

    # Test different configurations
    config_results = test_different_configurations(camera_id)
    print()

    # Analysis
    print("🎯 ANALYSIS RESULTS")
    print("="*50)

    print(f"Pure Capture Speed: {pure_fps:.1f} FPS")
    print(f"Processing Speed: {processing_fps:.1f} FPS")
    print(f"Display Speed: {display_fps:.1f} FPS")
    print()

    # Determine bottleneck
    if pure_fps >= 50:
        print("✅ Camera hardware CAN achieve 60Hz!")
        print("🔍 BOTTLENECK ANALYSIS:")

        overhead_loss = pure_fps - display_fps
        processing_loss = pure_fps - processing_fps

        if overhead_loss > 20:
            print(f"   ❌ Display rendering is the bottleneck (-{overhead_loss:.1f} FPS)")
            print("   💡 Solution: Use without display or use different rendering method")

        if processing_loss > 5:
            print(f"   ⚠️ OpenCV processing adds overhead (-{processing_loss:.1f} FPS)")

        print("   💡 RECOMMENDATIONS:")
        print("      1. Capture without cv2.imshow() for performance")
        print("      2. Consider GStreamer backend for better performance")
        print("      3. Use direct V4L2 if available")

    elif pure_fps >= 15:
        print("⚠️ Camera hardware supports moderate FPS")
        print("   May be limited by USB bandwidth or camera hardware")

    else:
        print("❌ Camera hardware limited to ~10 FPS")
        print("   This is a hardware limitation, not OpenCV")
        print("   💡 Solution: Need different camera for 60Hz applications")

    print()
    print("Best configuration from tests:")
    if config_results:
        best = max(config_results, key=lambda x: x['fps'])
        print(f"   {best['name']}: {best['fps']:.1f} FPS ({best['resolution']})")

if __name__ == "__main__":
    main()