# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Status

This project is to build the robust control pipeline for ic_arm(our team's arm);
We Now have 9 motor on one fdcan line. And we need to control all of them at the target of 500 hz, which need the cpp deployment a lot.

You should consider the continue-makable of the project and organize the system properly. And you could take /Users/lr-2002/project/instantcreation/ic_can/openarm_can for reference.

We use /Users/lr-2002/project/instantcreation/ic_can/dm-tools/USB2FDCAN/SDK/C++ as our control pipeline, you should send and receive according to it.

You should read /Users/lr-2002/project/instantcreation/IC_arm_control/ic_arm_control/control/IC_ARM.py to read the system of our environment.

1. c++ as low-level controller
2. python communication enable(pybind11 for now )
3. cross platform necessary(use less platform-specific function or library)
4. should achieve ic_arm's basic function .
5. for now, do not try to communicate with usb2fdcan, as the robot is not beside me.
6. we have different motors, 8 for arm, 1 for gripper.
    1. DM 10010l
    2. DM 6248
    3. DM 6248
    4. DM 4340
    5. DM 4340
    6. DM 4310
    7. HT 4438
    8. HT 4438

    ----
    servo motor

## Development Setup

When code is added to this repository, this file should be updated with:

- Build commands
- Test execution instructions
- Linting and formatting procedures
- Project architecture overview
- Development workflow guidelines
- update the /Users/lr-2002/project/instantcreation/ic_can/progress.md to display the status.
- write the detail log of update and use git to memorize the change history
- do not delete any file out of /Users/lr-2002/project/instantcreation/ic_can

## Notes

- The project directory is named `ic_can` suggesting this may be related to Controller Area Network (CAN) protocol implementation
- Parent directory is `instantcreation` indicating this might be part of a larger project suite

