//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// C_mtrx_fcn.h
//
// Code generation for function 'C_mtrx_fcn'
//

#pragma once

// Include files
#include "rtwtypes.h"
#include "emlrt.h"
#include "mex.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Function Declarations
void C_mtrx_fcn(const emlrtStack *sp, const real_T in1[6], const real_T in2[6],
                const real_T in3[66], real_T C_mtrx_sym[36]);

// End of code generation (C_mtrx_fcn.h)
