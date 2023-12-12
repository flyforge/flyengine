
#pragma once

#if !defined(__clang__) && (defined(__GNUC__) || defined(__GNUG__))

#  undef PLASMA_COMPILER_GCC
#  define PLASMA_COMPILER_GCC PLASMA_ON

/// \todo re-investigate: attribute(always inline) does not work for some reason
#  define PLASMA_ALWAYS_INLINE inline
#  define PLASMA_FORCE_INLINE inline

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
