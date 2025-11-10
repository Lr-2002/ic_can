#!/bin/bash
# Config #3 - 2025-11-10T22:22:05.859683
# FPS: 60.5 - Changed contrast to 80
echo 'Applying camera configuration...'

v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_automatic=0
v4l2-ctl -d /dev/video2 --set-ctrl=brightness=0
v4l2-ctl -d /dev/video2 --set-ctrl=contrast=80
v4l2-ctl -d /dev/video2 --set-ctrl=saturation=80
v4l2-ctl -d /dev/video2 --set-ctrl=gamma=150
v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_automatic=0
v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_temperature=5000
