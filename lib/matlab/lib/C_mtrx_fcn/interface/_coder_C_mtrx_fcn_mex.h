/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: _coder_C_mtrx_fcn_mex.h
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 2025-10-15 16:55:25
 */

#ifndef _CODER_C_MTRX_FCN_MEX_H
#define _CODER_C_MTRX_FCN_MEX_H

/* Include Files */
#include "emlrt.h"
#include "mex.h"
#include "tmwtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Function Declarations */
MEXFUNCTION_LINKAGE void mexFunction(int32_T nlhs, mxArray *plhs[],
                                     int32_T nrhs, const mxArray *prhs[]);

emlrtCTX mexFunctionCreateRootTLS(void);

void unsafe_C_mtrx_fcn_mexFunction(int32_T nlhs, mxArray *plhs[1], int32_T nrhs,
                                   const mxArray *prhs[3]);

#ifdef __cplusplus
}
#endif

#endif
/*
 * File trailer for _coder_C_mtrx_fcn_mex.h
 *
 * [EOF]
 */
