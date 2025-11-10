#!/bin/bash
# Best camera configuration - 2025-11-10T22:15:19.536284
# Performance: 61.6 FPS
# Score: 118.5

echo 'Applying BEST camera configuration...'

v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_automatic=0
v4l2-ctl -d /dev/video2 --set-ctrl=brightness=5
v4l2-ctl -d /dev/video2 --set-ctrl=contrast=90
v4l2-ctl -d /dev/video2 --set-ctrl=saturation=100
v4l2-ctl -d /dev/video2 --set-ctrl=gamma=130
v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_temperature=5500

echo '✅ Best camera configuration applied!'
echo 'Expected performance: 61.6 FPS'
