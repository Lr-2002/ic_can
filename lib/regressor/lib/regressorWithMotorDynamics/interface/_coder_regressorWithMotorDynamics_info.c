/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: _coder_regressorWithMotorDynamics_info.c
 *
 * MATLAB Coder version            : 24.1
 * C/C++ source code generated on  : 2025-10-15 20:55:52
 */

/* Include Files */
#include "_coder_regressorWithMotorDynamics_info.h"
#include "emlrt.h"
#include "tmwtypes.h"

/* Function Declarations */
static const mxArray *c_emlrtMexFcnResolvedFunctionsI(void);

/* Function Definitions */
/*
 * Arguments    : void
 * Return Type  : const mxArray *
 */
static const mxArray *c_emlrtMexFcnResolvedFunctionsI(void)
{
  const mxArray *nameCaptureInfo;
  const char_T *data[4] = {
      "789cc553cb4a033114cd482d6eaac5851bbf4141c48d20087d8060a5d08a0b476a3ab9b6"
      "c13c240fdb7e84f85b2e5dbaf027fc033b9d47a7953085d17a373727"
      "87e49c7b429077d1f21042db28aab772d42b31aec67d032dd632ef397a529ba8b4702ee1"
      "5fe31e4861606c22203087f424919c0a2c4c77f204488196ec19c88c",
      "79a00cba9443270bae42c49b192a052115ae6b43081e3b962335d473872c0bd23c3e1cf3"
      "9656ccc33af2a82ef1b78dbbdaa97fad41697f44855f9723c12426da"
      "2793691c34e80598d1bec2864a71c0b136a056a0b4af60300d4d4b7543cdb0258d54f598"
      "3ae49939c705e7dccd9933e1b5c18260457aa9af683ff1715fd047d9",
      "e9236288b47d0673bdf7827acca9b7c8fff6fb626be40084ff33cfd9b3e6e6b9b3e27cae"
      "ff5c415bb3fe72f6e9ad53efdcdbff5aa75e52ffa557f45fee39f4aa"
      "4bbcbd3c191dd7a9b5412b909d4903378fda83e6dc473b4727cf0772e0bfbeff1b21c89f"
      "b1",
      ""};
  nameCaptureInfo = NULL;
  emlrtNameCaptureMxArrayR2016a(&data[0], 1760U, &nameCaptureInfo);
  return nameCaptureInfo;
}

/*
 * Arguments    : void
 * Return Type  : mxArray *
 */
mxArray *emlrtMexFcnProperties(void)
{
  mxArray *xEntryPoints;
  mxArray *xInputs;
  mxArray *xResult;
  const char_T *propFieldName[9] = {"Version",
                                    "ResolvedFunctions",
                                    "Checksum",
                                    "EntryPoints",
                                    "CoverageInfo",
                                    "IsPolymorphic",
                                    "PropertyList",
                                    "UUID",
                                    "ClassEntryPointIsHandle"};
  const char_T *epFieldName[8] = {
      "Name",     "NumberOfInputs", "NumberOfOutputs", "ConstantInputs",
      "FullPath", "TimeStamp",      "Constructor",     "Visible"};
  xEntryPoints =
      emlrtCreateStructMatrix(1, 1, 8, (const char_T **)&epFieldName[0]);
  xInputs = emlrtCreateLogicalMatrix(1, 3);
  emlrtSetField(xEntryPoints, 0, "Name",
                emlrtMxCreateString("regressorWithMotorDynamics"));
  emlrtSetField(xEntryPoints, 0, "NumberOfInputs",
                emlrtMxCreateDoubleScalar(3.0));
  emlrtSetField(xEntryPoints, 0, "NumberOfOutputs",
                emlrtMxCreateDoubleScalar(1.0));
  emlrtSetField(xEntryPoints, 0, "ConstantInputs", xInputs);
  emlrtSetField(
      xEntryPoints, 0, "FullPath",
      emlrtMxCreateString(
          "C:\\Users\\win\\Downloads\\dynamic_calibration-master\\dynamic_"
          "calibration-master\\dynamics\\regressorWithMotorDynamics.m"));
  emlrtSetField(xEntryPoints, 0, "TimeStamp",
                emlrtMxCreateDoubleScalar(739905.57278935181));
  emlrtSetField(xEntryPoints, 0, "Constructor",
                emlrtMxCreateLogicalScalar(false));
  emlrtSetField(xEntryPoints, 0, "Visible", emlrtMxCreateLogicalScalar(true));
  xResult =
      emlrtCreateStructMatrix(1, 1, 9, (const char_T **)&propFieldName[0]);
  emlrtSetField(xResult, 0, "Version",
                emlrtMxCreateString("24.1.0.2837808 (R2024a) Update 7"));
  emlrtSetField(xResult, 0, "ResolvedFunctions",
                (mxArray *)c_emlrtMexFcnResolvedFunctionsI());
  emlrtSetField(xResult, 0, "Checksum",
                emlrtMxCreateString("aEH8SwYUB6IbLAbtz0dqRB"));
  emlrtSetField(xResult, 0, "EntryPoints", xEntryPoints);
  return xResult;
}

/*
 * File trailer for _coder_regressorWithMotorDynamics_info.c
 *
 * [EOF]
 */
