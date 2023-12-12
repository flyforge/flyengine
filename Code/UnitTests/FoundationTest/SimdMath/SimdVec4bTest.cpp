#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdVec4b.h>

PLASMA_CREATE_SIMPLE_TEST(SimdMath, SimdVec4b)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
#if PLASMA_DISABLED(PLASMA_COMPILER_GCC) && PLASMA_DISABLED(PLASMA_COMPILE_FOR_DEBUG)
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(16) float testBlock[4] = {1, 2, 3, 4};
    plSimdVec4b* pDefCtor = ::new ((void*)&testBlock[0]) plSimdVec4b;
    PLASMA_TEST_BOOL(testBlock[0] == 1.0f && testBlock[1] == 2.0f && testBlock[2] == 3.0f && testBlock[3] == 4.0f);
#endif

    // Make sure the class didn't accidentally change in size.
#if PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE
    PLASMA_CHECK_AT_COMPILETIME(sizeof(plSimdVec4b) == 16);
    PLASMA_CHECK_AT_COMPILETIME(PLASMA_ALIGNMENT_OF(plSimdVec4b) == 16);
#endif

    plSimdVec4b vInit1B(true);
    PLASMA_TEST_BOOL(vInit1B.x() == true && vInit1B.y() == true && vInit1B.z() == true && vInit1B.w() == true);

    // Make sure all components have the correct value
#if PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE && PLASMA_ENABLED(PLASMA_COMPILER_MSVC)
    PLASMA_TEST_BOOL(vInit1B.m_v.m128_u32[0] == 0xFFFFFFFF && vInit1B.m_v.m128_u32[1] == 0xFFFFFFFF && vInit1B.m_v.m128_u32[2] == 0xFFFFFFFF &&
                 vInit1B.m_v.m128_u32[3] == 0xFFFFFFFF);
#endif

    plSimdVec4b vInit4B(false, true, false, true);
    PLASMA_TEST_BOOL(vInit4B.x() == false && vInit4B.y() == true && vInit4B.z() == false && vInit4B.w() == true);

    // Make sure all components have the correct value
#if PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE && PLASMA_ENABLED(PLASMA_COMPILER_MSVC)
    PLASMA_TEST_BOOL(
      vInit4B.m_v.m128_u32[0] == 0 && vInit4B.m_v.m128_u32[1] == 0xFFFFFFFF && vInit4B.m_v.m128_u32[2] == 0 && vInit4B.m_v.m128_u32[3] == 0xFFFFFFFF);
#endif

    plSimdVec4b vCopy(vInit4B);
    PLASMA_TEST_BOOL(vCopy.x() == false && vCopy.y() == true && vCopy.z() == false && vCopy.w() == true);

    PLASMA_TEST_BOOL(
      vCopy.GetComponent<0>() == false && vCopy.GetComponent<1>() == true && vCopy.GetComponent<2>() == false && vCopy.GetComponent<3>() == true);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Swizzle")
  {
    plSimdVec4b a(true, false, true, false);

    plSimdVec4b b = a.Get<plSwizzle::XXXX>();
    PLASMA_TEST_BOOL(b.x() && b.y() && b.z() && b.w());

    b = a.Get<plSwizzle::YYYX>();
    PLASMA_TEST_BOOL(!b.x() && !b.y() && !b.z() && b.w());

    b = a.Get<plSwizzle::ZZZX>();
    PLASMA_TEST_BOOL(b.x() && b.y() && b.z() && b.w());

    b = a.Get<plSwizzle::WWWX>();
    PLASMA_TEST_BOOL(!b.x() && !b.y() && !b.z() && b.w());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Operators")
  {
    plSimdVec4b a(true, false, true, false);
    plSimdVec4b b(false, true, true, false);

    plSimdVec4b c = a && b;
    PLASMA_TEST_BOOL(!c.x() && !c.y() && c.z() && !c.w());

    c = a || b;
    PLASMA_TEST_BOOL(c.x() && c.y() && c.z() && !c.w());

    c = !a;
    PLASMA_TEST_BOOL(!c.x() && c.y() && !c.z() && c.w());
    PLASMA_TEST_BOOL(c.AnySet<2>());
    PLASMA_TEST_BOOL(!c.AllSet<4>());
    PLASMA_TEST_BOOL(!c.NoneSet<4>());

    c = c || a;
    PLASMA_TEST_BOOL(c.AnySet<4>());
    PLASMA_TEST_BOOL(c.AllSet<4>());
    PLASMA_TEST_BOOL(!c.NoneSet<4>());

    c = !c;
    PLASMA_TEST_BOOL(!c.AnySet<4>());
    PLASMA_TEST_BOOL(!c.AllSet<4>());
    PLASMA_TEST_BOOL(c.NoneSet<4>());

    c = a == b;
    PLASMA_TEST_BOOL(!c.x() && !c.y() && c.z() && c.w());

    c = a != b;
    PLASMA_TEST_BOOL(c.x() && c.y() && !c.z() && !c.w());

    PLASMA_TEST_BOOL(a.AllSet<1>());
    PLASMA_TEST_BOOL(b.NoneSet<1>());

    plSimdVec4b cmp(false, true, false, true);
    c = plSimdVec4b::Select(cmp, a, b);
    PLASMA_TEST_BOOL(!c.x() && !c.y() && c.z() && !c.w());
  }
}