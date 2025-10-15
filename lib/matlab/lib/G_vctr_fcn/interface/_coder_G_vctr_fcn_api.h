/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: _coder_G_vctr_fcn_api.h
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 2025-10-15 16:53:35
 */

#ifndef _CODER_G_VCTR_FCN_API_H
#define _CODER_G_VCTR_FCN_API_H

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
void G_vctr_fcn(real_T in1[6], real_T in2[66], real_T G_vctr_sym[6]);

void G_vctr_fcn_api(const mxArray *const prhs[2], const mxArray **plhs);

void G_vctr_fcn_atexit(void);

void G_vctr_fcn_initialize(void);

void G_vctr_fcn_terminate(void);

void G_vctr_fcn_xil_shutdown(void);

void G_vctr_fcn_xil_terminate(void);

#ifdef __cplusplus
}
#endif

#endif
/*
 * File trailer for _coder_G_vctr_fcn_api.h
 *
 * [EOF]
 */
