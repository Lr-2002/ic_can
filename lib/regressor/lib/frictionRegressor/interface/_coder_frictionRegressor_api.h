/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: _coder_frictionRegressor_api.h
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 2025-10-15 21:00:21
 */

#ifndef _CODER_FRICTIONREGRESSOR_API_H
#define _CODER_FRICTIONREGRESSOR_API_H

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
void frictionRegressor(real_T qd_fltrd[6], real_T frctn[108]);

void frictionRegressor_api(const mxArray *prhs, const mxArray **plhs);

void frictionRegressor_atexit(void);

void frictionRegressor_initialize(void);

void frictionRegressor_terminate(void);

void frictionRegressor_xil_shutdown(void);

void frictionRegressor_xil_terminate(void);

#ifdef __cplusplus
}
#endif

#endif
/*
 * File trailer for _coder_frictionRegressor_api.h
 *
 * [EOF]
 */
