#!/bin/bash
# Auto test result 3
# FPS: 61.5, Score: 118.5
echo 'Applying camera configuration...'

v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_automatic=0
v4l2-ctl -d /dev/video2 --set-ctrl=brightness=-5
v4l2-ctl -d /dev/video2 --set-ctrl=contrast=70
v4l2-ctl -d /dev/video2 --set-ctrl=saturation=75
v4l2-ctl -d /dev/video2 --set-ctrl=gamma=160
v4l2-ctl -d /dev/video2 --set-ctrl=white_balance_temperature=4500
