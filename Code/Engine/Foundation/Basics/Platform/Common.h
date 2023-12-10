#pragma once

// On MSVC 2008 in 64 Bit <cmath> generates a lot of warnings (actually it is math.h, which is included by cmath)
#define PLASMA_MSVC_WARNING_NUMBER 4985
#include <Foundation/Basics/Compiler/MSVC/DisableWarning_MSVC.h>

// include std header
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <new>

#include <Foundation/Basics/Compiler/MSVC/RestoreWarning_MSVC.h>

// redefine NULL to nullptr
#undef NULL
#define NULL nullptr

// include c++11 specific header
#include <type_traits>
#include <utility>

#ifndef __has_cpp_attribute
#  define __has_cpp_attribute(name) 0
#endif

// [[nodiscard]] helper
#if __has_cpp_attribute(nodiscard)
#  define PLASMA_NODISCARD [[nodiscard]]
#else
#  define PLASMA_NODISCARD
#endif

#ifndef __INTELLISENSE__

// Macros to do compile-time checks, such as to ensure sizes of types
// PLASMA_CHECK_AT_COMPILETIME(exp) : only checks exp
// PLASMA_CHECK_AT_COMPILETIME_MSG(exp, msg) : checks exp and displays msg
#  define PLASMA_CHECK_AT_COMPILETIME(exp) static_assert(exp, PLASMA_STRINGIZE(exp) " is false.");

#  define PLASMA_CHECK_AT_COMPILETIME_MSG(exp, msg) static_assert(exp, PLASMA_STRINGIZE(exp) " is false. Message: " msg);

#else

// IntelliSense often isn't smart enough to evaluate these conditions correctly

#  define PLASMA_CHECK_AT_COMPILETIME(exp)

#  define PLASMA_CHECK_AT_COMPILETIME_MSG(exp, msg)

#endif

/// \brief Disallow the copy constructor and the assignment operator for this type.
#define PLASMA_DISALLOW_COPY_AND_ASSIGN(type) \
  type(const type&) = delete;             \
  void operator=(const type&) = delete

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
/// \brief Macro helper to check alignment
#  define PLASMA_CHECK_ALIGNMENT(ptr, alignment) PLASMA_ASSERT_DEV(((size_t)ptr & ((alignment) - 1)) == 0, "Wrong alignment.")
#else
/// \brief Macro helper to check alignment
#  define PLASMA_CHECK_ALIGNMENT(ptr, alignment)
#endif

#define PLASMA_CHECK_ALIGNMENT_16(ptr) PLASMA_CHECK_ALIGNMENT(ptr, 16)
#define PLASMA_CHECK_ALIGNMENT_32(ptr) PLASMA_CHECK_ALIGNMENT(ptr, 32)
#define PLASMA_CHECK_ALIGNMENT_64(ptr) PLASMA_CHECK_ALIGNMENT(ptr, 64)
#define PLASMA_CHECK_ALIGNMENT_128(ptr) PLASMA_CHECK_ALIGNMENT(ptr, 128)

#define PLASMA_WINCHECK_1 1          // PLASMA_INCLUDED_WINDOWS_H defined to 1, _WINDOWS_ defined (stringyfied to nothing)
#define PLASMA_WINCHECK_1_WINDOWS_ 1 // PLASMA_INCLUDED_WINDOWS_H defined to 1, _WINDOWS_ undefined (stringyfied to "_WINDOWS_")
#define PLASMA_WINCHECK_PLASMA_INCLUDED_WINDOWS_H \
  0 // PLASMA_INCLUDED_WINDOWS_H undefined (stringyfied to "PLASMA_INCLUDED_WINDOWS_H", _WINDOWS_ defined (stringyfied to nothing)
#define PLASMA_WINCHECK_PLASMA_INCLUDED_WINDOWS_H_WINDOWS_ \
  1 // PLASMA_INCLUDED_WINDOWS_H undefined (stringyfied to "PLASMA_INCLUDED_WINDOWS_H", _WINDOWS_ undefined (stringyfied to "_WINDOWS_")

/// \brief Checks whether Windows.h has been included directly instead of through 'IncludeWindows.h'
///
/// Does this by stringifying the available defines, concatenating them into one long word, which is a known #define that evaluates to 0 or 1
#define PLASMA_CHECK_WINDOWS_INCLUDE(PLASMA_WINH_INCLUDED, WINH_INCLUDED)                                       \
  PLASMA_CHECK_AT_COMPILETIME_MSG(PLASMA_CONCAT(PLASMA_WINCHECK_, PLASMA_CONCAT(PLASMA_WINH_INCLUDED, WINH_INCLUDED)) == 1, \
    "Windows.h has been included but not through pl. #include <Foundation/Basics/Platform/Win/IncludeWindows.h> instead of Windows.h");


/// \brief Define some macros to work with the MSVC analysis warning
/// Note that the StaticAnalysis.h in Basics/Compiler/MSVC will define the MSVC specific versions.
#define PLASMA_MSVC_ANALYSIS_WARNING_PUSH
#define PLASMA_MSVC_ANALYSIS_WARNING_POP
#define PLASMA_MSVC_ANALYSIS_WARNING_DISABLE(warningNumber)
#define PLASMA_MSVC_ANALYSIS_ASSUME(expression)

#if defined(_MSC_VER)
#  include <Foundation/Basics/Compiler/MSVC/StaticAnalysis.h>
#endif

#if PLASMA_ENABLED(PLASMA_COMPILE_ENGINE_AS_DLL)

/// \brief The tool 'StaticLinkUtil' inserts this macro into each file in a library.
/// Each library also needs to contain exactly one instance of PLASMA_STATICLINK_LIBRARY.
/// The macros create functions that reference each other, which means the linker is forced to look at all files in the library.
/// This in turn will drag all global variables into the visibility of the linker, and since it mustn't optimize them away,
/// they then end up in the final application, where they will do what they are meant for.
#  define PLASMA_STATICLINK_FILE(LibraryName, UniqueName) PLASMA_CHECK_WINDOWS_INCLUDE(PLASMA_INCLUDED_WINDOWS_H, _WINDOWS_)


/// \brief Used by the tool 'StaticLinkUtil' to generate the block after PLASMA_STATICLINK_LIBRARY, to create references to all
/// files inside a library. \see PLASMA_STATICLINK_FILE
#  define PLASMA_STATICLINK_REFERENCE(UniqueName)

/// \brief This must occur exactly once in each static library, such that all PLASMA_STATICLINK_FILE macros can reference it.
#  define PLASMA_STATICLINK_LIBRARY(LibraryName) void plReferenceFunction_##LibraryName(bool bReturn = true)

#else

struct plStaticLinkHelper
{
  using Func = void (*)(bool);
  plStaticLinkHelper(Func f) { f(true); }
};

/// \brief The tool 'StaticLinkUtil' inserts this macro into each file in a library.
/// Each library also needs to contain exactly one instance of PLASMA_STATICLINK_LIBRARY.
/// The macros create functions that reference each other, which means the linker is forced to look at all files in the library.
/// This in turn will drag all global variables into the visibility of the linker, and since it mustn't optimize them away,
/// they then end up in the final application, where they will do what they are meant for.
#  define PLASMA_STATICLINK_FILE(LibraryName, UniqueName)      \
    void plReferenceFunction_##UniqueName(bool bReturn) {} \
    void plReferenceFunction_##LibraryName(bool bReturn);  \
    static plStaticLinkHelper StaticLinkHelper_##UniqueName(plReferenceFunction_##LibraryName);

/// \brief Used by the tool 'StaticLinkUtil' to generate the block after PLASMA_STATICLINK_LIBRARY, to create references to all
/// files inside a library. \see PLASMA_STATICLINK_FILE
#  define PLASMA_STATICLINK_REFERENCE(UniqueName)                   \
    void plReferenceFunction_##UniqueName(bool bReturn = true); \
    plReferenceFunction_##UniqueName()

/// \brief This must occur exactly once in each static library, such that all PLASMA_STATICLINK_FILE macros can reference it.
#  define PLASMA_STATICLINK_LIBRARY(LibraryName) void plReferenceFunction_##LibraryName(bool bReturn = true)

#endif

namespace plInternal
{
  template <typename T, size_t N>
  char (*ArraySizeHelper(T (&)[N]))[N];
}

/// \brief Macro to determine the size of a static array
#define PLASMA_ARRAY_SIZE(a) (sizeof(*plInternal::ArraySizeHelper(a)) + 0)

/// \brief Template helper which allows to suppress "Unused variable" warnings (e.g. result used in platform specific block, ..)
template <class T>
void PLASMA_IGNORE_UNUSED(const T&)
{
}


// Math Debug checks
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)

#  undef PLASMA_MATH_CHECK_FOR_NAN
#  define PLASMA_MATH_CHECK_FOR_NAN PLASMA_ON

#endif

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  define PLASMA_DECL_EXPORT __declspec(dllexport)
#  define PLASMA_DECL_IMPORT __declspec(dllimport)
#  define PLASMA_DECL_EXPORT_FRIEND __declspec(dllexport)
#  define PLASMA_DECL_IMPORT_FRIEND __declspec(dllimport)
#else
#  define PLASMA_DECL_EXPORT [[gnu::visibility("default")]]
#  define PLASMA_DECL_IMPORT [[gnu::visibility("default")]]
#  define PLASMA_DECL_EXPORT_FRIEND
#  define PLASMA_DECL_IMPORT_FRIEND
#endif
