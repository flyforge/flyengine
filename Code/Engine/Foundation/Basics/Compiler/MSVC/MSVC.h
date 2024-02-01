#pragma once

#if defined(_MSC_VER) && !defined(__clang__)

#  undef PL_COMPILER_MSVC
#  define PL_COMPILER_MSVC PL_ON

#  if __clang__ || __castxml__
#    undef PL_COMPILER_MSVC_CLANG
#    define PL_COMPILER_MSVC_CLANG PL_ON
#  else
#    undef PL_COMPILER_MSVC_PURE
#    define PL_COMPILER_MSVC_PURE PL_ON
#  endif

#  ifdef _DEBUG
#    undef PL_COMPILE_FOR_DEBUG
#    define PL_COMPILE_FOR_DEBUG PL_ON
#  endif


// Functions marked as PL_ALWAYS_INLINE will be inlined even in Debug builds, which means you will step over them in a debugger
#  define PL_ALWAYS_INLINE __forceinline

#  if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
#    define PL_FORCE_INLINE inline
#  else
#    define PL_FORCE_INLINE __forceinline
#  endif

// workaround for MSVC compiler issue with alignment determination of dependent types
#  define PL_ALIGNMENT_OF(type) PL_COMPILE_TIME_MAX(PL_ALIGNMENT_MINIMUM, PL_COMPILE_TIME_MIN(sizeof(type), __alignof(type)))

#  if PL_ENABLED(PL_COMPILE_FOR_DEBUG) || (_MSC_VER >= 1929 /* broken in early VS2019 but works again in VS2022 and later 2019 versions*/)

#    define PL_DEBUG_BREAK \
      {                    \
        __debugbreak();    \
      }

#  else

#    define PL_DEBUG_BREAK                         \
      {                                            \
        /* Declared with DLL export in Assert.h */ \
        MSVC_OutOfLine_DebugBreak();               \
      }

#  endif

#  if PL_ENABLED(PL_COMPILER_MSVC_CLANG)
#    define PL_SOURCE_FUNCTION __PRETTY_FUNCTION__
#  else
#    define PL_SOURCE_FUNCTION __FUNCTION__
#  endif

#  define PL_SOURCE_LINE __LINE__
#  define PL_SOURCE_FILE __FILE__

// PL_VA_NUM_ARGS() is a very nifty macro to retrieve the number of arguments handed to a variable-argument macro
// unfortunately, VS 2010 still has this compiler bug which treats a __VA_ARGS__ argument as being one single parameter:
// https://connect.microsoft.com/VisualStudio/feedback/details/521844/variadic-macro-treating-va-args-as-a-single-parameter-for-other-macros#details
#  if _MSC_VER >= 1400 && PL_DISABLED(PL_COMPILER_MSVC_CLANG)
#    define PL_VA_NUM_ARGS_HELPER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) N
#    define PL_VA_NUM_ARGS_REVERSE_SEQUENCE 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1
#    define PL_LEFT_PARENTHESIS (
#    define PL_RIGHT_PARENTHESIS )
#    define PL_VA_NUM_ARGS(...) PL_VA_NUM_ARGS_HELPER PL_LEFT_PARENTHESIS __VA_ARGS__, PL_VA_NUM_ARGS_REVERSE_SEQUENCE PL_RIGHT_PARENTHESIS
#  endif

#  define PL_WARNING_PUSH() __pragma(warning(push))
#  define PL_WARNING_POP() __pragma(warning(pop))
#  define PL_WARNING_DISABLE_MSVC(_x) __pragma(warning(disable \
                                                       : _x))

#else

#  define PL_WARNING_DISABLE_MSVC(_x)

#endif
