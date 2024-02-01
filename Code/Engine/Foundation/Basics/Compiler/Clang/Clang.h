
#pragma once

#ifdef __clang__

#  undef PL_COMPILER_CLANG
#  define PL_COMPILER_CLANG PL_ON

#  define PL_ALWAYS_INLINE __attribute__((always_inline)) inline
#  if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
#    define PL_FORCE_INLINE inline
#  else
#    define PL_FORCE_INLINE __attribute__((always_inline)) inline
#  endif

#  define PL_ALIGNMENT_OF(type) PL_COMPILE_TIME_MAX(__alignof(type), PL_ALIGNMENT_MINIMUM)

#  if __has_builtin(__builtin_debugtrap)
#    define PL_DEBUG_BREAK     \
      {                        \
        __builtin_debugtrap(); \
      }
#  elif __has_builtin(__debugbreak)
#    define PL_DEBUG_BREAK \
      {                    \
        __debugbreak();    \
      }
#  else
#    include <signal.h>
#    if defined(SIGTRAP)
#      define PL_DEBUG_BREAK \
        {                    \
          raise(SIGTRAP);    \
        }
#    else
#      define PL_DEBUG_BREAK \
        {                    \
          raise(SIGABRT);    \
        }
#    endif
#  endif

#  define PL_SOURCE_FUNCTION __PRETTY_FUNCTION__
#  define PL_SOURCE_LINE __LINE__
#  define PL_SOURCE_FILE __FILE__

#  ifdef BUILDSYSTEM_BUILDTYPE_Debug
#    undef PL_COMPILE_FOR_DEBUG
#    define PL_COMPILE_FOR_DEBUG PL_ON
#  endif

#  define PL_WARNING_PUSH() _Pragma("clang diagnostic push")
#  define PL_WARNING_POP() _Pragma("clang diagnostic pop")
#  define PL_WARNING_DISABLE_CLANG(_x) _Pragma(PL_STRINGIZE(clang diagnostic ignored _x))

#else

#  define PL_WARNING_DISABLE_CLANG(_x)

#endif
