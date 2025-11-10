#!/bin/bash
# Auto test result 2
# FPS: 61.6, Score: 118.5
echo 'Applying camera configuration...'

v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_automatic=0
v4l2-ctl -d /dev/video2 --set-ctrl=brightness=5
v4l2-ctl -d /dev/video2 --set-ctrl=contrast=90
v4l2-ctl -d /dev/video2 --set-ctrl=saturation=100
v4l2-ctl -d /dev/video2 --set-ctrl=gamma=130
v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_temperature=5500
