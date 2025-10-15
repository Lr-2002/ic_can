//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// M_mtrx_fcn_initialize.cpp
//
// Code generation for function 'M_mtrx_fcn_initialize'
//

// Include files
#include "M_mtrx_fcn_initialize.h"
#include "M_mtrx_fcn_data.h"
#include "_coder_M_mtrx_fcn_mex.h"
#include "rt_nonfinite.h"

// Function Declarations
static void M_mtrx_fcn_once();

// Function Definitions
static void M_mtrx_fcn_once()
{
  mex_InitInfAndNan();
}

void M_mtrx_fcn_initialize()
{
  static const volatile char_T *emlrtBreakCheckR2012bFlagVar{nullptr};
  emlrtStack st{
      nullptr, // site
      nullptr, // tls
      nullptr  // prev
  };
  mexFunctionCreateRootTLS();
  st.tls = emlrtRootTLSGlobal;
  emlrtBreakCheckR2012bFlagVar = emlrtGetBreakCheckFlagAddressR2022b(&st);
  emlrtClearAllocCountR2012b(&st, false, 0U, nullptr);
  emlrtEnterRtStackR2012b(&st);
  if (emlrtFirstTimeR2012b(emlrtRootTLSGlobal)) {
    M_mtrx_fcn_once();
  }
}

// End of code generation (M_mtrx_fcn_initialize.cpp)
