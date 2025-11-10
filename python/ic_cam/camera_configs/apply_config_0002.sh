#!/bin/bash
# Config #2 - 2025-11-10T22:22:03.839600
# FPS: 62.0 - Changed brightness to 0
echo 'Applying camera configuration...'

v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_automatic=0
v4l2-ctl -d /dev/video2 --set-ctrl=brightness=0
v4l2-ctl -d /dev/video2 --set-ctrl=contrast=80
v4l2-ctl -d /dev/video2 --set-ctrl=saturation=80
v4l2-ctl -d /dev/video2 --set-ctrl=gamma=150
v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_automatic=0
v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_temperature=5000
