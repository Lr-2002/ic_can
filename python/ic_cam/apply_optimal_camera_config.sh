#!/bin/bash
# Optimal Camera Configuration
# Brightness: 13/64, Contrast: 14/100, Saturation: 61/100, Gamma: 29/500, White Balance: 4600K

echo "Applying Optimal Camera Configuration..."

# Disable auto white balance first
v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_automatic=0

# Apply optimal settings
v4l2-ctl -d /dev/video2 --set-ctrl=brightness=13
v4l2-ctl -d /dev/video2 --set-ctrl=contrast=14
v4l2-ctl -d /dev/video2 --set-ctrl=saturation=61
v4l2-ctl -d /dev/video2 --set-ctrl=gamma=29
v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_temperature=4600

echo "✅ Optimal camera configuration applied!"
echo "Expected performance: 60+ FPS with balanced colors"
