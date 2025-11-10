#!/bin/bash
# Config #5 - 2025-11-10T22:22:09.891839
# FPS: 60.5 - Changed gamma to 150
echo 'Applying camera configuration...'

v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_automatic=0
v4l2-ctl -d /dev/video2 --set-ctrl=brightness=0
v4l2-ctl -d /dev/video2 --set-ctrl=contrast=80
v4l2-ctl -d /dev/video2 --set-ctrl=saturation=80
v4l2-ctl -d /dev/video2 --set-ctrl=gamma=150
v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_automatic=0
v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_temperature=5000
