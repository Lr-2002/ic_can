#!/bin/bash
# Auto test result 6
# FPS: 61.4, Score: 118.5
echo 'Applying camera configuration...'

v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_automatic=0
v4l2-ctl -d /dev/video2 --set-ctrl=brightness=-10
v4l2-ctl -d /dev/video2 --set-ctrl=contrast=75
v4l2-ctl -d /dev/video2 --set-ctrl=saturation=85
v4l2-ctl -d /dev/video2 --set-ctrl=gamma=140
v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_temperature=5000
