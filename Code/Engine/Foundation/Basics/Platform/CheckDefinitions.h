#pragma once

#ifndef NULL
#  error "NULL is not defined."
#endif

#ifndef PLASMA_FORCE_INLINE
#  error "PLASMA_FORCE_INLINE is not defined."
#endif

#ifndef PLASMA_ALWAYS_INLINE
#  error "PLASMA_ALWAYS_INLINE is not defined."
#endif

#ifndef PLASMA_ALIGNMENT_OF
#  error "PLASMA_ALIGNMENT_OF is not defined."
#endif

#if PLASMA_IS_NOT_EXCLUSIVE(PLASMA_PLATFORM_32BIT, PLASMA_PLATFORM_64BIT)
#  error "Platform is not defined as 32 Bit or 64 Bit"
#endif

#ifndef PLASMA_DEBUG_BREAK
#  error "PLASMA_DEBUG_BREAK is not defined."
#endif

#ifndef PLASMA_SOURCE_FUNCTION
#  error "PLASMA_SOURCE_FUNCTION is not defined."
#endif

#ifndef PLASMA_SOURCE_FILE
#  error "PLASMA_SOURCE_FILE is not defined."
#endif

#ifndef PLASMA_SOURCE_LINE
#  error "PLASMA_SOURCE_LINE is not defined."
#endif

#if PLASMA_IS_NOT_EXCLUSIVE(PLASMA_PLATFORM_LITTLE_ENDIAN, PLASMA_PLATFORM_BIG_ENDIAN)
#  error "Endianess is not correctly defined."
#endif

#ifndef PLASMA_MATH_CHECK_FOR_NAN
#  error "PLASMA_MATH_CHECK_FOR_NAN is not defined."
#endif

#if PLASMA_IS_NOT_EXCLUSIVE(PLASMA_PLATFORM_ARCH_X86, PLASMA_PLATFORM_ARCH_ARM)
#  error "Platform architecture is not correctly defined."
#endif

#if !defined(PLASMA_SIMD_IMPLEMENTATION) || (PLASMA_SIMD_IMPLEMENTATION == 0)
#  error "PLASMA_SIMD_IMPLEMENTATION is not correctly defined."
#endif
