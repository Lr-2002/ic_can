/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: _coder_C_mtrx_fcn_api.h
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 2025-10-15 16:55:25
 */

#ifndef _CODER_C_MTRX_FCN_API_H
#define _CODER_C_MTRX_FCN_API_H

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
void C_mtrx_fcn(real_T in1[6], real_T in2[6], real_T in3[66],
                real_T C_mtrx_sym[36]);

void C_mtrx_fcn_api(const mxArray *const prhs[3], const mxArray **plhs);

void C_mtrx_fcn_atexit(void);

void C_mtrx_fcn_initialize(void);

void C_mtrx_fcn_terminate(void);

void C_mtrx_fcn_xil_shutdown(void);

void C_mtrx_fcn_xil_terminate(void);

#ifdef __cplusplus
}
#endif

#endif
/*
 * File trailer for _coder_C_mtrx_fcn_api.h
 *
 * [EOF]
 */
