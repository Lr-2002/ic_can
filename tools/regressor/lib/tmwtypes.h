/*
 * MATLAB compatible types header
 * Created for standalone compilation of MATLAB Coder generated functions
 */

#ifndef TMWTYPES_H
#define TMWTYPES_H

#include <stddef.h>
#include <stdint.h>

/* Basic type definitions for MATLAB compatibility */
typedef double real_T;
typedef float  real32_T;

/* Fixed-width integer types */
typedef int8_t   int8_T;
typedef uint8_t  uint8_T;
typedef int16_t  int16_T;
typedef uint16_t uint16_T;
typedef int32_t  int32_T;
typedef uint32_t uint32_T;
typedef int64_t  int64_T;
typedef uint64_t uint64_T;

/* Boolean type */
typedef int boolean_T;
#define true  1
#define false 0

/* Function pointer types */
typedef void (*voidFuncPtr)(void);

/* Math constants */
#ifndef PI
#define PI 3.14159265358979323846
#endif

#endif /* TMWTYPES_H */