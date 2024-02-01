
#pragma once

#if !defined(__clang__) && (defined(__GNUC__) || defined(__GNUG__))

#  undef PL_COMPILER_GCC
#  define PL_COMPILER_GCC PL_ON

/// \todo re-investigate: attribute(always inline) does not work for some reason
#  define PL_ALWAYS_INLINE inline
#  define PL_FORCE_INLINE inline

#  define PL_ALIGNMENT_OF(type) PL_COMPILE_TIME_MAX(__alignof(type), PL_ALIGNMENT_MINIMUM)

#  if __has_builtin(__builtin_debugtrap)
#    define PL_DEBUG_BREAK     \
      {                        \
        __builtin_debugtrap(); \
      }
#  elif defined(__i386__) || defined(__x86_64__)
#    define PL_DEBUG_BREAK            \
      {                               \
        __asm__ __volatile__("int3"); \
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

#  define PL_WARNING_PUSH() _Pragma("GCC diagnostic push")
#  define PL_WARNING_POP() _Pragma("GCC diagnostic pop")
#  define PL_WARNING_DISABLE_GCC(_x) _Pragma(PL_STRINGIZE(GCC diagnostic ignored _x))

#else

#  define PL_WARNING_DISABLE_GCC(_x)

#endif
