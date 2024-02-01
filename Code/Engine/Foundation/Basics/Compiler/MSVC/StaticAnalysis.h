
#pragma once

#undef PL_MSVC_ANALYSIS_WARNING_PUSH
#undef PL_MSVC_ANALYSIS_WARNING_POP
#undef PL_MSVC_ANALYSIS_WARNING_DISABLE
#undef PL_MSVC_ANALYSIS_ASSUME

// These use the __pragma version to control the warnings so that they can be used within other macros etc.
#define PL_MSVC_ANALYSIS_WARNING_PUSH __pragma(warning(push))
#define PL_MSVC_ANALYSIS_WARNING_POP __pragma(warning(pop))
#define PL_MSVC_ANALYSIS_WARNING_DISABLE(warningNumber) __pragma(warning(disable : warningNumber))
#define PL_MSVC_ANALYSIS_ASSUME(expression) __assume(expression)
