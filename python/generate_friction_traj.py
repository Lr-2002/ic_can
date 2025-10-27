import numpy as np
import time

init_pos = [10, 0, 0, 0, 0, 0]
domain = [9, -9]
init_pos = np.array(init_pos)
upper = init_pos + 9
lower = init_pos - 9
freq = 400
time = 3
upper_points = np.linspace(init_pos, upper, freq * time)
print(upper_points)
lower_points = np.linspace(upper, lower, freq * time)
print(lower_points)
lower_points = np.flip(lower_points, axis=0)
traj = np.concatenate((upper_points, lower_points), axis=0)
print(traj)
