#pragma once

#define PLASMA_SSE_20 0x20
#define PLASMA_SSE_30 0x30
#define PLASMA_SSE_31 0x31
#define PLASMA_SSE_41 0x41
#define PLASMA_SSE_42 0x42
#define PLASMA_SSE_AVX 0x50
#define PLASMA_SSE_AVX2 0x51

#define PLASMA_SSE_LEVEL PLASMA_SSE_41

#if PLASMA_SSE_LEVEL >= PLASMA_SSE_20
#  include <emmintrin.h>
#endif

#if PLASMA_SSE_LEVEL >= PLASMA_SSE_30
#  include <pmmintrin.h>
#endif

#if PLASMA_SSE_LEVEL >= PLASMA_SSE_31
#  include <tmmintrin.h>
#endif

#if PLASMA_SSE_LEVEL >= PLASMA_SSE_41
#  include <smmintrin.h>
#endif

#if PLASMA_SSE_LEVEL >= PLASMA_SSE_42
#  include <nmmintrin.h>
#endif

#if PLASMA_SSE_LEVEL >= PLASMA_SSE_AVX
#  include <immintrin.h>
#endif

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
#  define PLASMA_CHECK_SIMD_ALIGNMENT PLASMA_CHECK_ALIGNMENT_16
#else
#  define PLASMA_CHECK_SIMD_ALIGNMENT(x)
#endif

namespace plInternal
{
  using QuadFloat = __m128;
  using QuadBool = __m128;
  using QuadInt = __m128i;
  using QuadUInt = __m128i;
} // namespace plInternal

#include <Foundation/SimdMath/SimdSwizzle.h>

#define PLASMA_SHUFFLE(a0, a1, b2, b3) ((a0) | ((a1) << 2) | ((b2) << 4) | ((b3) << 6))

#define PLASMA_TO_SHUFFLE(s) ((((s) >> 12) & 0x03) | (((s) >> 6) & 0x0c) | ((s) & 0x30) | (((s) << 6) & 0xc0))
