#!/bin/bash
# BEST CONFIG - 2025-11-10T22:25:18.604340
# BEST FPS: 61.7
echo 'Applying BEST camera configuration...'

v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_automatic=0
v4l2-ctl -d /dev/video2 --set-ctrl=brightness=0
v4l2-ctl -d /dev/video2 --set-ctrl=contrast=75
v4l2-ctl -d /dev/video2 --set-ctrl=saturation=85
v4l2-ctl -d /dev/video2 --set-ctrl=gamma=140
v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_temperature=5000
