/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: _coder_regressorWithMotorDynamics_api.h
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 2025-10-15 20:55:52
 */

#ifndef _CODER_REGRESSORWITHMOTORDYNAMICS_API_H
#define _CODER_REGRESSORWITHMOTORDYNAMICS_API_H

/* Include Files */
#include "emlrt.h"
#include "mex.h"
#include "tmwtypes.h"
#include <string.h>

/* Variable Declarations */
extern emlrtCTX emlrtRootTLSGlobal;
extern emlrtContext emlrtContextGlobal;

#ifdef __cplusplus
extern "C" {
#endif

/* Function Declarations */
void regressorWithMotorDynamics(real_T q[6], real_T qd[6], real_T q2d[6],
                                real_T Y[396]);

void regressorWithMotorDynamics_api(const mxArray *const prhs[3],
                                    const mxArray **plhs);

void regressorWithMotorDynamics_atexit(void);

void regressorWithMotorDynamics_initialize(void);

void regressorWithMotorDynamics_terminate(void);

void regressorWithMotorDynamics_xil_shutdown(void);

void regressorWithMotorDynamics_xil_terminate(void);

#ifdef __cplusplus
}
#endif

#endif
/*
 * File trailer for _coder_regressorWithMotorDynamics_api.h
 *
 * [EOF]
 */
