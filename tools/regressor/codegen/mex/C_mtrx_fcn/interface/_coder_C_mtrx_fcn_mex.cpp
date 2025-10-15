//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
//
// _coder_C_mtrx_fcn_mex.cpp
//
// Code generation for function '_coder_C_mtrx_fcn_mex'
//

// Include files
#include "_coder_C_mtrx_fcn_mex.h"
#include "C_mtrx_fcn_data.h"
#include "C_mtrx_fcn_initialize.h"
#include "C_mtrx_fcn_terminate.h"
#include "_coder_C_mtrx_fcn_api.h"
#include "rt_nonfinite.h"
#include <stdexcept>

void emlrtExceptionBridge();
void emlrtExceptionBridge()
{
  throw std::runtime_error("");
}
// Function Definitions
void C_mtrx_fcn_mexFunction(int32_T nlhs, mxArray *plhs[1], int32_T nrhs,
                            const mxArray *prhs[3])
{
  emlrtStack st{
      nullptr, // site
      nullptr, // tls
      nullptr  // prev
  };
  const mxArray *outputs;
  st.tls = emlrtRootTLSGlobal;
  // Check for proper number of arguments.
  if (nrhs != 3) {
    emlrtErrMsgIdAndTxt(&st, "EMLRT:runTime:WrongNumberOfInputs", 5, 12, 3, 4,
                        10, "C_mtrx_fcn");
  }
  if (nlhs > 1) {
    emlrtErrMsgIdAndTxt(&st, "EMLRT:runTime:TooManyOutputArguments", 3, 4, 10,
                        "C_mtrx_fcn");
  }
  // Call the function.
  C_mtrx_fcn_api(prhs, &outputs);
  // Copy over outputs to the caller.
  emlrtReturnArrays(1, &plhs[0], &outputs);
}

void mexFunction(int32_T nlhs, mxArray *plhs[], int32_T nrhs,
                 const mxArray *prhs[])
{
  mexAtExit(&C_mtrx_fcn_atexit);
  // Module initialization.
  C_mtrx_fcn_initialize();
  try { // Dispatch the entry-point.
    C_mtrx_fcn_mexFunction(nlhs, plhs, nrhs, prhs);
    // Module termination.
    C_mtrx_fcn_terminate();
  } catch (...) {
    emlrtCleanupOnException((emlrtCTX *)emlrtRootTLSGlobal);
    throw;
  }
}

emlrtCTX mexFunctionCreateRootTLS()
{
  emlrtCreateRootTLSR2022a(&emlrtRootTLSGlobal, &emlrtContextGlobal, nullptr, 1,
                           (void *)&emlrtExceptionBridge, "GBK", true);
  return emlrtRootTLSGlobal;
}

// End of code generation (_coder_C_mtrx_fcn_mex.cpp)
