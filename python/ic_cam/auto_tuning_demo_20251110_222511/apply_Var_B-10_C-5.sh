#!/bin/bash
# Var_B-10_C-5
# Performance: 61.7 FPS
echo 'Applying camera configuration...'

v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_automatic=0
v4l2-ctl -d /dev/video2 --set-ctrl=brightness=-5
v4l2-ctl -d /dev/video2 --set-ctrl=contrast=85
v4l2-ctl -d /dev/video2 --set-ctrl=saturation=100
v4l2-ctl -d /dev/video2 --set-ctrl=gamma=130
v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_temperature=5500
