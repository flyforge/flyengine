
#pragma once

#ifdef __clang__

#  undef PLASMA_COMPILER_CLANG
#  define PLASMA_COMPILER_CLANG PLASMA_ON

#  define PLASMA_ALWAYS_INLINE __attribute__((always_inline)) inline
#  if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
#    define PLASMA_FORCE_INLINE inline
#  else
#    define PLASMA_FORCE_INLINE __attribute__((always_inline)) inline
#  endif

#  define PLASMA_ALIGNMENT_OF(type) PLASMA_COMPILE_TIME_MAX(__alignof(type), PLASMA_ALIGNMENT_MINIMUM)

#  if __has_builtin(__builtin_debugtrap)
#    define PLASMA_DEBUG_BREAK     \
      {                        \
        __builtin_debugtrap(); \
      }
#  elif __has_builtin(__debugbreak)
#    define PLASMA_DEBUG_BREAK \
      {                    \
        __debugbreak();    \
      }
#  else
#    include <signal.h>
#    if defined(SIGTRAP)
#      define PLASMA_DEBUG_BREAK \
        {                    \
          raise(SIGTRAP);    \
        }
#    else
#      define PLASMA_DEBUG_BREAK \
        {                    \
          raise(SIGABRT);    \
        }
#    endif
#  endif

#  define PLASMA_SOURCE_FUNCTION __PRETTY_FUNCTION__
#  define PLASMA_SOURCE_LINE __LINE__
#  define PLASMA_SOURCE_FILE __FILE__

#  ifdef BUILDSYSTEM_BUILDTYPE_Debug
#    undef PLASMA_COMPILE_FOR_DEBUG
#    define PLASMA_COMPILE_FOR_DEBUG PLASMA_ON
#  endif

#  define PLASMA_WARNING_PUSH() _Pragma("clang diagnostic push")
#  define PLASMA_WARNING_POP() _Pragma("clang diagnostic pop")
#  define PLASMA_WARNING_DISABLE_CLANG(_x) _Pragma(PLASMA_STRINGIZE(clang diagnostic ignored _x))

#else

#  define PLASMA_WARNING_DISABLE_CLANG(_x)

#endif
