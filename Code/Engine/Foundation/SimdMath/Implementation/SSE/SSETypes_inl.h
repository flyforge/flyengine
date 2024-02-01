#pragma once

#define PL_SSE_20 0x20
#define PL_SSE_30 0x30
#define PL_SSE_31 0x31
#define PL_SSE_41 0x41
#define PL_SSE_42 0x42
#define PL_SSE_AVX 0x50
#define PL_SSE_AVX2 0x51

#define PL_SSE_LEVEL PL_SSE_41

#if PL_SSE_LEVEL >= PL_SSE_20
#  include <emmintrin.h>
#endif

#if PL_SSE_LEVEL >= PL_SSE_30
#  include <pmmintrin.h>
#endif

#if PL_SSE_LEVEL >= PL_SSE_31
#  include <tmmintrin.h>
#endif

#if PL_SSE_LEVEL >= PL_SSE_41
#  include <smmintrin.h>
#endif

#if PL_SSE_LEVEL >= PL_SSE_42
#  include <nmmintrin.h>
#endif

#if PL_SSE_LEVEL >= PL_SSE_AVX
#  include <immintrin.h>
#endif

#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
#  define PL_CHECK_SIMD_ALIGNMENT PL_CHECK_ALIGNMENT_16
#else
#  define PL_CHECK_SIMD_ALIGNMENT(x)
#endif

namespace plInternal
{
  using QuadFloat = __m128;
  using QuadBool = __m128;
  using QuadInt = __m128i;
  using QuadUInt = __m128i;
} // namespace plInternal

#include <Foundation/SimdMath/SimdSwizzle.h>

#define PL_SHUFFLE(a0, a1, b2, b3) ((a0) | ((a1) << 2) | ((b2) << 4) | ((b3) << 6))

#define PL_TO_SHUFFLE(s) ((((s) >> 12) & 0x03) | (((s) >> 6) & 0x0c) | ((s) & 0x30) | (((s) << 6) & 0xc0))
