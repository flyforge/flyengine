#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdFloat.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(SimdMath);

PLASMA_CREATE_SIMPLE_TEST(SimdMath, SimdFloat)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with NaN.
    plSimdFloat vDefCtor;
    PLASMA_TEST_BOOL(plMath::IsNaN((float)vDefCtor));
#else
// GCC assumes that the contents of the memory before calling the default constructor are irrelevant.
// So it optimizes away the 1,2,3,4 initializer completely.
#  if PLASMA_DISABLED(PLASMA_COMPILER_GCC)
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(16) float testBlock[4] = {1, 2, 3, 4};
    plSimdFloat* pDefCtor = ::new ((void*)&testBlock[0]) plSimdFloat;
    PLASMA_TEST_BOOL_MSG((float)(*pDefCtor) == 1.0f, "Default constructed value is %f", (float)(*pDefCtor));
#  endif
#endif

    // Make sure the class didn't accidentally change in size.
#if PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE
    PLASMA_CHECK_AT_COMPILETIME(sizeof(plSimdFloat) == 16);
    PLASMA_CHECK_AT_COMPILETIME(PLASMA_ALIGNMENT_OF(plSimdFloat) == 16);
#endif

    plSimdFloat vInit1F(2.0f);
    PLASMA_TEST_BOOL(vInit1F == 2.0f);

    // Make sure all components are set to the same value
#if (PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE) && PLASMA_ENABLED(PLASMA_COMPILER_MSVC)
    PLASMA_TEST_BOOL(
      vInit1F.m_v.m128_f32[0] == 2.0f && vInit1F.m_v.m128_f32[1] == 2.0f && vInit1F.m_v.m128_f32[2] == 2.0f && vInit1F.m_v.m128_f32[3] == 2.0f);
#endif

    plSimdFloat vInit1I(1);
    PLASMA_TEST_BOOL(vInit1I == 1.0f);

    // Make sure all components are set to the same value
#if PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE && PLASMA_ENABLED(PLASMA_COMPILER_MSVC)
    PLASMA_TEST_BOOL(
      vInit1I.m_v.m128_f32[0] == 1.0f && vInit1I.m_v.m128_f32[1] == 1.0f && vInit1I.m_v.m128_f32[2] == 1.0f && vInit1I.m_v.m128_f32[3] == 1.0f);
#endif

    plSimdFloat vInit1U(4553u);
    PLASMA_TEST_BOOL(vInit1U == 4553.0f);

    // Make sure all components are set to the same value
#if PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE && PLASMA_ENABLED(PLASMA_COMPILER_MSVC)
    PLASMA_TEST_BOOL(vInit1U.m_v.m128_f32[0] == 4553.0f && vInit1U.m_v.m128_f32[1] == 4553.0f && vInit1U.m_v.m128_f32[2] == 4553.0f &&
                 vInit1U.m_v.m128_f32[3] == 4553.0f);
#endif

    plSimdFloat z = plSimdFloat::Zero();
    PLASMA_TEST_BOOL(z == 0.0f);

    // Make sure all components are set to the same value
#if PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE && PLASMA_ENABLED(PLASMA_COMPILER_MSVC)
    PLASMA_TEST_BOOL(z.m_v.m128_f32[0] == 0.0f && z.m_v.m128_f32[1] == 0.0f && z.m_v.m128_f32[2] == 0.0f && z.m_v.m128_f32[3] == 0.0f);
#endif
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Operators")
  {
    plSimdFloat a = 5.0f;
    plSimdFloat b = 2.0f;

    PLASMA_TEST_FLOAT(a + b, 7.0f, plMath::SmallEpsilon<float>());
    PLASMA_TEST_FLOAT(a - b, 3.0f, plMath::SmallEpsilon<float>());
    PLASMA_TEST_FLOAT(a * b, 10.0f, plMath::SmallEpsilon<float>());
    PLASMA_TEST_FLOAT(a / b, 2.5f, plMath::SmallEpsilon<float>());

    plSimdFloat c = 1.0f;
    c += a;
    PLASMA_TEST_FLOAT(c, 6.0f, plMath::SmallEpsilon<float>());

    c = 1.0f;
    c -= b;
    PLASMA_TEST_FLOAT(c, -1.0f, plMath::SmallEpsilon<float>());

    c = 1.0f;
    c *= a;
    PLASMA_TEST_FLOAT(c, 5.0f, plMath::SmallEpsilon<float>());

    c = 1.0f;
    c /= a;
    PLASMA_TEST_FLOAT(c, 0.2f, plMath::SmallEpsilon<float>());

    PLASMA_TEST_BOOL(c.IsEqual(0.201f, plMath::HugeEpsilon<float>()));
    PLASMA_TEST_BOOL(c.IsEqual(0.199f, plMath::HugeEpsilon<float>()));
    PLASMA_TEST_BOOL(!c.IsEqual(0.202f, plMath::HugeEpsilon<float>()));
    PLASMA_TEST_BOOL(!c.IsEqual(0.198f, plMath::HugeEpsilon<float>()));

    c = b;
    PLASMA_TEST_BOOL(c == b);
    PLASMA_TEST_BOOL(c != a);
    PLASMA_TEST_BOOL(a > b);
    PLASMA_TEST_BOOL(c >= b);
    PLASMA_TEST_BOOL(b < a);
    PLASMA_TEST_BOOL(b <= c);

    PLASMA_TEST_BOOL(c == 2.0f);
    PLASMA_TEST_BOOL(c != 5.0f);
    PLASMA_TEST_BOOL(a > 2.0f);
    PLASMA_TEST_BOOL(c >= 2.0f);
    PLASMA_TEST_BOOL(b < 5.0f);
    PLASMA_TEST_BOOL(b <= 2.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Misc")
  {
    plSimdFloat a = 2.0f;

    PLASMA_TEST_FLOAT(a.GetReciprocal(), 0.5f, plMath::SmallEpsilon<float>());
    PLASMA_TEST_FLOAT(a.GetReciprocal<plMathAcc::FULL>(), 0.5f, plMath::SmallEpsilon<float>());
    PLASMA_TEST_FLOAT(a.GetReciprocal<plMathAcc::BITS_23>(), 0.5f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(a.GetReciprocal<plMathAcc::BITS_12>(), 0.5f, plMath::HugeEpsilon<float>());

    PLASMA_TEST_FLOAT(a.GetSqrt(), 1.41421356f, plMath::SmallEpsilon<float>());
    PLASMA_TEST_FLOAT(a.GetSqrt<plMathAcc::FULL>(), 1.41421356f, plMath::SmallEpsilon<float>());
    PLASMA_TEST_FLOAT(a.GetSqrt<plMathAcc::BITS_23>(), 1.41421356f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(a.GetSqrt<plMathAcc::BITS_12>(), 1.41421356f, plMath::HugeEpsilon<float>());

    PLASMA_TEST_FLOAT(a.GetInvSqrt(), 0.70710678f, plMath::SmallEpsilon<float>());
    PLASMA_TEST_FLOAT(a.GetInvSqrt<plMathAcc::FULL>(), 0.70710678f, plMath::SmallEpsilon<float>());
    PLASMA_TEST_FLOAT(a.GetInvSqrt<plMathAcc::BITS_23>(), 0.70710678f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(a.GetInvSqrt<plMathAcc::BITS_12>(), 0.70710678f, plMath::HugeEpsilon<float>());

    plSimdFloat b = 5.0f;
    PLASMA_TEST_BOOL(a.Max(b) == b);
    PLASMA_TEST_BOOL(a.Min(b) == a);

    plSimdFloat c = -4.0f;
    PLASMA_TEST_FLOAT(c.Abs(), 4.0f, plMath::SmallEpsilon<float>());
  }
}
