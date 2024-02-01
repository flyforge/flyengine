#pragma once

#if defined(__clang__) || defined(__GNUC__)

#  if defined(__x86_64__) || defined(__i386__)
#    undef PL_PLATFORM_ARCH_X86
#    define PL_PLATFORM_ARCH_X86 PL_ON
#  elif defined(__arm__) || defined(__aarch64__)
#    undef PL_PLATFORM_ARCH_ARM
#    define PL_PLATFORM_ARCH_ARM PL_ON
#  else
#    error unhandled target architecture
#  endif

#  if defined(__x86_64__) || defined(__aarch64__)
#    undef PL_PLATFORM_64BIT
#    define PL_PLATFORM_64BIT PL_ON
#  elif defined(__i386__) || defined(__arm__)
#    undef PL_PLATFORM_32BIT
#    define PL_PLATFORM_32BIT PL_ON
#  else
#    error unhandled platform bit count
#  endif

#elif defined(_MSC_VER)

#  if defined(_M_AMD64) || defined(_M_IX86)
#    undef PL_PLATFORM_ARCH_X86
#    define PL_PLATFORM_ARCH_X86 PL_ON
#  elif defined(_M_ARM) || defined(_M_ARM64)
#    undef PL_PLATFORM_ARCH_ARM
#    define PL_PLATFORM_ARCH_ARM PL_ON
#  else
#    error unhandled target architecture
#  endif

#  if defined(_M_AMD64) || defined(_M_ARM64)
#    undef PL_PLATFORM_64BIT
#    define PL_PLATFORM_64BIT PL_ON
#  elif defined(_M_IX86) || defined(_M_ARM)
#    undef PL_PLATFORM_32BIT
#    define PL_PLATFORM_32BIT PL_ON
#  else
#    error unhandled platform bit count
#  endif

#else
#  error unhandled compiler
#endif
