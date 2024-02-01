
#pragma once

/// \file

/// \brief Used to pass a token through without modification
/// Useful to separate tokens that have no whitespace in between and thus would otherwise form one string.
#define PL_PP_IDENTITY(x) x

/// \brief Concatenates two strings, even when the strings are macros themselves
#define PL_CONCAT(x, y) PL_CONCAT_HELPER(x, y)
#define PL_CONCAT_HELPER(x, y) PL_CONCAT_HELPER2(x, y)
#define PL_CONCAT_HELPER2(x, y) x##y

/// \brief Turns some piece of code (usually some identifier name) into a string. Even works on macros.
#define PL_STRINGIZE(str) PL_STRINGIZE_HELPER(str)
#define PL_STRINGIZE_HELPER(x) #x

/// \brief Concatenates two strings, even when the strings are macros themselves
#define PL_PP_CONCAT(x, y) PL_CONCAT_HELPER(x, y)

/// \brief Turns some piece of code (usually some identifier name) into a string. Even works on macros.
#define PL_PP_STRINGIFY(str) PL_STRINGIZE_HELPER(str)

/// \brief Max value of two compile-time constant expression.
#define PL_COMPILE_TIME_MAX(a, b) ((a) > (b) ? (a) : (b))

/// \brief Min value of two compile-time constant expression.
#define PL_COMPILE_TIME_MIN(a, b) ((a) < (b) ? (a) : (b))


/// \brief Creates a bit mask with only the n-th Bit set. Useful when creating enum values for flags.
#define PL_BIT(n) (1ull << (n))
