#!/bin/bash
# Auto test result 4
# FPS: 61.4, Score: 118.5
echo 'Applying camera configuration...'

v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_automatic=0
v4l2-ctl -d /dev/video2 --set-ctrl=brightness=0
v4l2-ctl -d /dev/video2 --set-ctrl=contrast=100
v4l2-ctl -d /dev/video2 --set-ctrl=saturation=90
v4l2-ctl -d /dev/video2 --set-ctrl=gamma=120
v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_temperature=5200
