#!/bin/bash
# Final Camera Configuration
# Brightness: 40/64, Contrast: 20/100, Saturation: 61/100, Gamma: 40/500, White Balance: 4000K

echo "Applying Final Camera Configuration..."

# Disable auto white balance first
v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_automatic=0

# Apply final settings
v4l2-ctl -d /dev/video2 --set-ctrl=brightness=40
v4l2-ctl -d /dev/video2 --set-ctrl=contrast=20
v4l2-ctl -d /dev/video2 --set-ctrl=saturation=61
v4l2-ctl -d /dev/video2 --set-ctrl=gamma=40
v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_temperature=4000

echo "✅ Final camera configuration applied!"
echo "Expected performance: 60+ FPS with optimized settings"