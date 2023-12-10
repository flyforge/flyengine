
#pragma once

/// \file

/// \brief Concatenates two strings, even when the strings are macros themselves
#define PLASMA_CONCAT(x, y) PLASMA_CONCAT_HELPER(x, y)
#define PLASMA_CONCAT_HELPER(x, y) PLASMA_CONCAT_HELPER2(x, y)
#define PLASMA_CONCAT_HELPER2(x, y) x##y

/// \brief Turns some piece of code (usually some identifier name) into a string. Even works on macros.
#define PLASMA_STRINGIZE(str) PLASMA_STRINGIZE_HELPER(str)
#define PLASMA_STRINGIZE_HELPER(x) #x

/// \brief Concatenates two strings, even when the strings are macros themselves
#define PLASMA_PP_CONCAT(x, y) PLASMA_CONCAT_HELPER(x, y)

/// \brief Turns some piece of code (usually some identifier name) into a string. Even works on macros.
#define PLASMA_PP_STRINGIFY(str) PLASMA_STRINGIZE_HELPER(str)

/// \brief Max value of two compile-time constant expression.
#define PLASMA_COMPILE_TIME_MAX(a, b) ((a) > (b) ? (a) : (b))

/// \brief Min value of two compile-time constant expression.
#define PLASMA_COMPILE_TIME_MIN(a, b) ((a) < (b) ? (a) : (b))


/// \brief Creates a bit mask with only the n-th Bit set. Useful when creating enum values for flags.
#define PLASMA_BIT(n) (1ull << (n))
