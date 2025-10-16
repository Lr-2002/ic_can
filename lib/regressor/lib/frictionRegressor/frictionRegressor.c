/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: frictionRegressor.c
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 2025-10-15 21:00:21
 */

/* Include Files */
#include "frictionRegressor.h"
#include "rt_nonfinite.h"
#include "rt_nonfinite.h"
#include <string.h>

/* Function Definitions */
/*
 * ----------------------------------------------------------------------
 *  The function computes friction regressor for each joint of the robot.
 *  Fv*qd + Fc*sign(qd) + F0, and the second one is continous,
 *  ---------------------------------------------------------------------
 *
 * Arguments    : const double qd_fltrd[6]
 *                double frctn[108]
 * Return Type  : void
 */
void frictionRegressor(const double qd_fltrd[6], double frctn[108])
{
  int i;
  memset(&frctn[0], 0, 108U * sizeof(double));
  for (i = 0; i < 6; i++) {
    double d;
    int b_i;
    b_i = 3 * (i + 1) - 2;
    d = qd_fltrd[i];
    frctn[i + 6 * (b_i - 1)] = d;
    if (rtIsNaN(d)) {
      frctn[i + 6 * b_i] = rtNaN;
    } else if (d < 0.0) {
      frctn[i + 6 * b_i] = -1.0;
    } else {
      frctn[i + 6 * b_i] = (d > 0.0);
    }
    frctn[i + 6 * (b_i + 1)] = 1.0;
  }
}

/*
 * File trailer for frictionRegressor.c
 *
 * [EOF]
 */
