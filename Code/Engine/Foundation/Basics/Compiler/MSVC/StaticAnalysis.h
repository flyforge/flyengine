
#pragma once

#undef PLASMA_MSVC_ANALYSIS_WARNING_PUSH
#undef PLASMA_MSVC_ANALYSIS_WARNING_POP
#undef PLASMA_MSVC_ANALYSIS_WARNING_DISABLE
#undef PLASMA_MSVC_ANALYSIS_ASSUME

// These use the __pragma version to control the warnings so that they can be used within other macros etc.
#define PLASMA_MSVC_ANALYSIS_WARNING_PUSH __pragma(warning(push))
#define PLASMA_MSVC_ANALYSIS_WARNING_POP __pragma(warning(pop))
#define PLASMA_MSVC_ANALYSIS_WARNING_DISABLE(warningNumber) __pragma(warning(disable : warningNumber))
#define PLASMA_MSVC_ANALYSIS_ASSUME(expression) __assume(expression)
