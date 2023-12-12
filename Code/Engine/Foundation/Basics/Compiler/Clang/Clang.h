
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

#  define PLASMA_DEBUG_BREAK \
    {                    \
      __builtin_trap();  \
    }

#  define PLASMA_SOURCE_FUNCTION __PRETTY_FUNCTION__
#  define PLASMA_SOURCE_LINE __LINE__
#  define PLASMA_SOURCE_FILE __FILE__

#  ifdef BUILDSYSTEM_BUILDTYPE_Debug
#    undef PLASMA_COMPILE_FOR_DEBUG
#    define PLASMA_COMPILE_FOR_DEBUG PLASMA_ON
#  endif

#endif
