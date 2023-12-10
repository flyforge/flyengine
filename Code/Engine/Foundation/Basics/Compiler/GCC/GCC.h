
#pragma once

#if !defined(__clang__) && (defined(__GNUC__) || defined(__GNUG__))

#  undef PLASMA_COMPILER_GCC
#  define PLASMA_COMPILER_GCC PLASMA_ON

/// \todo re-investigate: attribute(always inline) does not work for some reason
#  define PLASMA_ALWAYS_INLINE inline
#  define PLASMA_FORCE_INLINE inline

#  define PLASMA_ALIGNMENT_OF(type) PLASMA_COMPILE_TIME_MAX(__alignof(type), PLASMA_ALIGNMENT_MINIMUM)

#  if __has_builtin(__builtin_debugtrap)
#    define PLASMA_DEBUG_BREAK     \
      {                        \
        __builtin_debugtrap(); \
      }
#  elif defined(__i386__) || defined(__x86_64__)
#    define PLASMA_DEBUG_BREAK            \
      {                               \
        __asm__ __volatile__("int3"); \
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

#  define PLASMA_WARNING_PUSH() _Pragma("GCC diagnostic push")
#  define PLASMA_WARNING_POP() _Pragma("GCC diagnostic pop")
#  define PLASMA_WARNING_DISABLE_GCC(_x) _Pragma(PLASMA_STRINGIZE(GCC diagnostic ignored _x))

#else

#  define PLASMA_WARNING_DISABLE_GCC(_x)

#endif
