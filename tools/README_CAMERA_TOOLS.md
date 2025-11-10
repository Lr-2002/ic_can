# Camera Capture Tools for IC_CAN

This directory contains tools for capturing camera video synchronized with robot data, optimized for machine learning applications.

## Features

- **Real-time camera capture** with configurable resolution and FPS
- **H.264 I-frame only encoding** (no future frame dependencies)
- **Synchronized robot data capture** (positions, velocities, torques)
- **Timestamp overlay** with microsecond precision
- **Performance monitoring** and statistics
- **CSV data export** for ML training
- **Multiple camera support**

## Tools Overview

### 1. `camera_capture.py` - Basic Camera Capture

Simple camera capture tool for recording video with timestamps.

```bash
# Basic usage
python camera_capture.py

# Custom settings
python camera_capture.py --camera 0 --resolution 1280x720 --fps 30 --duration 60

# High quality
python camera_capture.py --quality 10 --output high_quality_video.mp4

# Preview mode
python camera_capture.py --preview --preview-duration 15
```

### 2. `synchronized_capture.py` - Camera + Robot Data Capture

Advanced tool for synchronized camera and robot data capture, perfect for ML training.

```bash
# Basic synchronized capture
python synchronized_capture.py

# With custom settings
python synchronized_capture.py --camera 0 --resolution 640x480 --fps 30 --duration 120

# Save individual frames
python synchronized_capture.py --save-frames --output ml_dataset

# 10 minutes of training data
python synchronized_capture.py --duration 600 --output training_session_001
```

## Installation Requirements

```bash
# System packages
sudo apt update
sudo apt install python3-opencv python3-numpy

# Python packages (if using virtual environment)
pip install opencv-python numpy

# For IC_CAN integration (optional)
cd /home/lr-2002/project/InstantCreation/ic_can
mkdir build && cd build
cmake .. -DBUILD_PYTHON_BINDINGS=ON
make -j$(nproc)
export PYTHONPATH="$PWD/python:$PYTHONPATH"
```

## Quick Start

### 1. List Available Cameras

```bash
python camera_capture.py --list
```

Output:
```
Scanning for cameras...
Camera 0: 640x480 @ 30.00 FPS
Camera 1: 1280x720 @ 30.00 FPS
Found 2 camera(s)
```

### 2. Preview Camera

```bash
# 10 second preview
python camera_capture.py --preview --preview-duration 10

# Preview different camera
python camera_capture.py --camera 1 --preview
```

### 3. Record Basic Video

```bash
# 30 second recording
python camera_capture.py --duration 30

# High resolution recording
python camera_capture.py --resolution 1920x1080 --fps 60 --quality 15
```

### 4. Record Synchronized Data

```bash
# Record 2 minutes of synchronized data
python synchronized_capture.py --duration 120 --output session_001

# Record with individual frames saved
python synchronized_capture.py --save-frames --duration 60
```

## Output Files

### Basic Camera Capture

```
camera_capture_20241110_143022.mp4    # Video file
```

### Synchronized Capture

```
sync_capture_20241110_143022/
├── video.mp4                    # H.264 encoded video
├── synchronized_data.csv        # Robot state data
├── metadata.json               # Recording metadata
└── frame_XXXXXX.jpg            # Individual frames (if --save-frames)
```

## CSV Data Format

The synchronized data CSV contains the following columns:

```
timestamp,frame_number,
pos_1,pos_2,pos_3,pos_4,pos_5,pos_6,pos_7,pos_8,pos_9,
vel_1,vel_2,vel_3,vel_4,vel_5,vel_6,vel_7,vel_8,vel_9,
torque_1,torque_2,torque_3,torque_4,torque_5,torque_6,torque_7,torque_8,torque_9,
control_mode,is_safe,capture_latency,robot_latency
```

## Command Line Options

### camera_capture.py

```
--camera, -c        Camera ID (default: 0)
--output, -o        Output video file path
--resolution, -r    Resolution WIDTHxHEIGHT (default: 640x480)
--fps               Target FPS (default: 30)
--quality, -q       H.264 quality 0-51 (default: 18)
--duration, -d      Recording duration in seconds
--preview, -p       Preview mode only
--preview-duration  Preview duration in seconds (default: 10)
--list, -l          List available cameras
```

### synchronized_capture.py

```
--camera, -c        Camera ID (default: 0)
--output, -o        Output directory
--resolution, -r    Resolution WIDTHxHEIGHT (default: 640x480)
--fps               Target FPS (default: 30)
--quality, -q       H.264 quality 0-51 (default: 18)
--duration, -d      Recording duration in seconds
--save-frames       Save individual frames as images
```

## H.264 Encoding Details

The tools use H.264 encoding with **I-frame only mode** to ensure:

- **No future frame dependencies** - Each frame can be decoded independently
- **Low latency** - No need to wait for future frames
- **Real-time processing** - Suitable for live applications
- **ML compatibility** - Easy frame-by-frame processing

### Quality Settings

- **0-18**: High quality (larger files)
- **19-28**: Medium quality (balanced)
- **29-51**: Low quality (smaller files)

## Performance Considerations

### System Requirements

- **CPU**: Multi-core processor recommended for threading
- **RAM**: 4GB+ for high-resolution recording
- **Storage**: SSD recommended for high-bitrate recording
- **USB**: USB 3.0+ for high-resolution cameras

### Optimizing Performance

1. **Reduce resolution** if experiencing frame drops
2. **Lower quality setting** to reduce CPU load
3. **Use SSD storage** for smooth writing
4. **Close unnecessary applications** to free resources

### Monitoring Performance

Both tools provide real-time performance monitoring:

```
PERFORMANCE STATISTICS
==================================================
Total frames captured: 900
Total frames written:  900
Dropped frames:        0
Average FPS:           30.00
Avg encoding time:     15.2 ms
Frame drop rate:       0.00%
Output file:           video.mp4
File size:             45.2 MB
==================================================
```

## Troubleshooting

### Common Issues

1. **Camera not found**
   ```bash
   python camera_capture.py --list
   # Try different camera IDs
   ```

2. **Permission denied**
   ```bash
   # Add user to video group
   sudo usermod -a -G video $USER
   # Reboot or logout/login
   ```

3. **Low frame rate**
   - Reduce resolution
   - Lower quality setting
   - Check CPU usage
   - Use faster storage

4. **Out of memory**
   - Reduce resolution
   - Disable frame saving
   - Increase RAM

### Error Messages

- **"Failed to open camera"**: Check camera connection and permissions
- **"Failed to initialize video writer"**: Check output directory permissions
- **"High frame drop rate"**: Reduce resolution or quality settings

## Integration with ML Pipelines

### Python Integration Example

```python
import cv2
import pandas as pd
import numpy as np

# Load synchronized data
data = pd.read_csv('sync_capture_20241110_143022/synchronized_data.csv')
video_path = 'sync_capture_20241110_143022/video.mp4'

# Open video
cap = cv2.VideoCapture(video_path)

# Process frame by frame with synchronized data
for index, row in data.iterrows():
    ret, frame = cap.read()
    if ret:
        # Get synchronized robot state
        positions = [row[f'pos_{i}'] for i in range(1, 10)]
        velocities = [row[f'vel_{i}'] for i in range(1, 10)]

        # Process frame and state
        # ... your ML processing here ...

        # Display frame with data
        cv2.imshow('Frame', frame)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

cap.release()
cv2.destroyAllWindows()
```

### Dataset Creation

```python
# Create training dataset from multiple sessions
import os
import glob

def create_dataset(session_dirs):
    all_data = []

    for session_dir in session_dirs:
        csv_path = os.path.join(session_dir, 'synchronized_data.csv')
        if os.path.exists(csv_path):
            data = pd.read_csv(csv_path)
            data['session'] = os.path.basename(session_dir)
            all_data.append(data)

    # Combine all sessions
    dataset = pd.concat(all_data, ignore_index=True)
    return dataset

# Usage
sessions = glob.glob('sync_capture_*')
dataset = create_dataset(sessions)
dataset.to_csv('training_dataset.csv', index=False)
```

## Advanced Usage

### Custom Camera Configuration

```python
import cv2

# Configure camera for specific use case
cap = cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
cap.set(cv2.CAP_PROP_FPS, 60)
cap.set(cv2.CAP_PROP_AUTOFOCUS, 1)  # Enable autofocus
cap.set(cv2.CAP_PROP_EXPOSURE, -6)  # Set exposure
```

### Real-time Processing

```python
# Process frames in real-time
def process_frame(frame, robot_state):
    # Apply computer vision algorithms
    # Extract features
    # Run inference
    return processed_frame, results

# Integrate with synchronized capture
# Modify the encoding_thread_func to include custom processing
```

## Support

For issues or questions:

1. Check the troubleshooting section above
2. Verify camera compatibility
3. Ensure system requirements are met
4. Test with basic examples first

## License

These tools are part of the IC_CAN project and licensed under the Apache License 2.0.