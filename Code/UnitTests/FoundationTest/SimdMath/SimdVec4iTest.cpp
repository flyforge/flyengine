#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdVec4u.h>

PLASMA_CREATE_SIMPLE_TEST(SimdMath, SimdVec4i)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with 0xCDCDCDCD.
    plSimdVec4i vDefCtor;
    PLASMA_TEST_BOOL(vDefCtor.x() == 0xCDCDCDCD && vDefCtor.y() == 0xCDCDCDCD && vDefCtor.z() == 0xCDCDCDCD && vDefCtor.w() == 0xCDCDCDCD);
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(16) float testBlock[4] = {1, 2, 3, 4};
    plSimdVec4i* pDefCtor = ::new ((void*)&testBlock[0]) plSimdVec4i;
    PLASMA_TEST_BOOL(testBlock[0] == 1 && testBlock[1] == 2 && testBlock[2] == 3 && testBlock[3] == 4);
#endif

    // Make sure the class didn't accidentally change in size.
#if PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE
    PLASMA_CHECK_AT_COMPILETIME(sizeof(plSimdVec4i) == 16);
    PLASMA_CHECK_AT_COMPILETIME(PLASMA_ALIGNMENT_OF(plSimdVec4i) == 16);
#endif

    plSimdVec4i a(2);
    PLASMA_TEST_BOOL(a.x() == 2 && a.y() == 2 && a.z() == 2 && a.w() == 2);

    plSimdVec4i b(1, 2, 3, 4);
    PLASMA_TEST_BOOL(b.x() == 1 && b.y() == 2 && b.z() == 3 && b.w() == 4);

    // Make sure all components have the correct values
#if PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE && PLASMA_ENABLED(PLASMA_COMPILER_MSVC)
    PLASMA_TEST_BOOL(b.m_v.m128i_i32[0] == 1 && b.m_v.m128i_i32[1] == 2 && b.m_v.m128i_i32[2] == 3 && b.m_v.m128i_i32[3] == 4);
#endif

    plSimdVec4i copy(b);
    PLASMA_TEST_BOOL(copy.x() == 1 && copy.y() == 2 && copy.z() == 3 && copy.w() == 4);

    PLASMA_TEST_BOOL(copy.GetComponent<0>() == 1 && copy.GetComponent<1>() == 2 && copy.GetComponent<2>() == 3 && copy.GetComponent<3>() == 4);

    plSimdVec4i vZero = plSimdVec4i::ZeroVector();
    PLASMA_TEST_BOOL(vZero.x() == 0 && vZero.y() == 0 && vZero.z() == 0 && vZero.w() == 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Setter")
  {
    plSimdVec4i a;
    a.Set(2);
    PLASMA_TEST_BOOL(a.x() == 2 && a.y() == 2 && a.z() == 2 && a.w() == 2);

    plSimdVec4i b;
    b.Set(1, 2, 3, 4);
    PLASMA_TEST_BOOL(b.x() == 1 && b.y() == 2 && b.z() == 3 && b.w() == 4);

    plSimdVec4i vSetZero;
    vSetZero.SetZero();
    PLASMA_TEST_BOOL(vSetZero.x() == 0 && vSetZero.y() == 0 && vSetZero.z() == 0 && vSetZero.w() == 0);

    {
      int testBlock[4] = {1, 2, 3, 4};
      plSimdVec4i x;
      x.Load<1>(testBlock);
      PLASMA_TEST_BOOL(x.x() == 1 && x.y() == 0 && x.z() == 0 && x.w() == 0);

      plSimdVec4i xy;
      xy.Load<2>(testBlock);
      PLASMA_TEST_BOOL(xy.x() == 1 && xy.y() == 2 && xy.z() == 0 && xy.w() == 0);

      plSimdVec4i xyz;
      xyz.Load<3>(testBlock);
      PLASMA_TEST_BOOL(xyz.x() == 1 && xyz.y() == 2 && xyz.z() == 3 && xyz.w() == 0);

      plSimdVec4i xyzw;
      xyzw.Load<4>(testBlock);
      PLASMA_TEST_BOOL(xyzw.x() == 1 && xyzw.y() == 2 && xyzw.z() == 3 && xyzw.w() == 4);

      PLASMA_TEST_INT(xyzw.GetComponent<0>(), 1);
      PLASMA_TEST_INT(xyzw.GetComponent<1>(), 2);
      PLASMA_TEST_INT(xyzw.GetComponent<2>(), 3);
      PLASMA_TEST_INT(xyzw.GetComponent<3>(), 4);

      // Make sure all components have the correct values
#if PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE && PLASMA_ENABLED(PLASMA_COMPILER_MSVC)
      PLASMA_TEST_BOOL(xyzw.m_v.m128i_i32[0] == 1 && xyzw.m_v.m128i_i32[1] == 2 && xyzw.m_v.m128i_i32[2] == 3 && xyzw.m_v.m128i_i32[3] == 4);
#endif
    }

    {
      int testBlock[4] = {7, 7, 7, 7};
      int mem[4] = {};

      plSimdVec4i b2(1, 2, 3, 4);

      memcpy(mem, testBlock, 16);
      b2.Store<1>(mem);
      PLASMA_TEST_BOOL(mem[0] == 1 && mem[1] == 7 && mem[2] == 7 && mem[3] == 7);

      memcpy(mem, testBlock, 16);
      b2.Store<2>(mem);
      PLASMA_TEST_BOOL(mem[0] == 1 && mem[1] == 2 && mem[2] == 7 && mem[3] == 7);

      memcpy(mem, testBlock, 16);
      b2.Store<3>(mem);
      PLASMA_TEST_BOOL(mem[0] == 1 && mem[1] == 2 && mem[2] == 3 && mem[3] == 7);

      memcpy(mem, testBlock, 16);
      b2.Store<4>(mem);
      PLASMA_TEST_BOOL(mem[0] == 1 && mem[1] == 2 && mem[2] == 3 && mem[3] == 4);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Conversion")
  {
    plSimdVec4i ia(-3, 5, -7, 11);

    plSimdVec4u ua(ia);
    PLASMA_TEST_BOOL(ua.x() == -3 && ua.y() == 5 && ua.z() == -7 && ua.w() == 11);

    plSimdVec4f fa = ia.ToFloat();
    PLASMA_TEST_BOOL(fa.x() == -3.0f && fa.y() == 5.0f && fa.z() == -7.0f && fa.w() == 11.0f);

    fa = plSimdVec4f(-2.3f, 5.7f, -2147483520.0f, 2147483520.0f);
    plSimdVec4i b = plSimdVec4i::Truncate(fa);
    PLASMA_TEST_INT(b.x(), -2);
    PLASMA_TEST_INT(b.y(), 5);
    PLASMA_TEST_INT(b.z(), -2147483520);
    PLASMA_TEST_INT(b.w(), 2147483520);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Swizzle")
  {
    plSimdVec4i a(3, 5, 7, 9);

    plSimdVec4i b = a.Get<plSwizzle::XXXX>();
    PLASMA_TEST_BOOL(b.x() == 3 && b.y() == 3 && b.z() == 3 && b.w() == 3);

    b = a.Get<plSwizzle::YYYX>();
    PLASMA_TEST_BOOL(b.x() == 5 && b.y() == 5 && b.z() == 5 && b.w() == 3);

    b = a.Get<plSwizzle::ZZZX>();
    PLASMA_TEST_BOOL(b.x() == 7 && b.y() == 7 && b.z() == 7 && b.w() == 3);

    b = a.Get<plSwizzle::WWWX>();
    PLASMA_TEST_BOOL(b.x() == 9 && b.y() == 9 && b.z() == 9 && b.w() == 3);

    b = a.Get<plSwizzle::WZYX>();
    PLASMA_TEST_BOOL(b.x() == 9 && b.y() == 7 && b.z() == 5 && b.w() == 3);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Operators")
  {
    {
      plSimdVec4i a(-3, 5, -7, 9);

      plSimdVec4i b = -a;
      PLASMA_TEST_BOOL(b.x() == 3 && b.y() == -5 && b.z() == 7 && b.w() == -9);

      b.Set(8, 6, 4, 2);
      plSimdVec4i c;
      c = a + b;
      PLASMA_TEST_BOOL(c.x() == 5 && c.y() == 11 && c.z() == -3 && c.w() == 11);

      c = a - b;
      PLASMA_TEST_BOOL(c.x() == -11 && c.y() == -1 && c.z() == -11 && c.w() == 7);

      c = a.CompMul(b);
      PLASMA_TEST_BOOL(c.x() == -24 && c.y() == 30 && c.z() == -28 && c.w() == 18);

      c = a.CompDiv(b);
      PLASMA_TEST_BOOL(c.x() == 0 && c.y() == 0 && c.z() == -1 && c.w() == 4);
    }

    {
      plSimdVec4i a(PLASMA_BIT(1), PLASMA_BIT(2), PLASMA_BIT(3), PLASMA_BIT(4));
      plSimdVec4i b(PLASMA_BIT(4), PLASMA_BIT(3), PLASMA_BIT(3), PLASMA_BIT(5) - 1);
      plSimdVec4i c;

      c = a | b;
      PLASMA_TEST_BOOL(c.x() == (PLASMA_BIT(1) | PLASMA_BIT(4)) && c.y() == (PLASMA_BIT(2) | PLASMA_BIT(3)) && c.z() == PLASMA_BIT(3) && c.w() == PLASMA_BIT(5) - 1);

      c = a & b;
      PLASMA_TEST_BOOL(c.x() == 0 && c.y() == 0 && c.z() == PLASMA_BIT(3) && c.w() == PLASMA_BIT(4));

      c = a ^ b;
      PLASMA_TEST_BOOL(c.x() == (PLASMA_BIT(1) | PLASMA_BIT(4)) && c.y() == (PLASMA_BIT(2) | PLASMA_BIT(3)) && c.z() == 0 && c.w() == PLASMA_BIT(4) - 1);

      c = ~a;
      PLASMA_TEST_BOOL(c.x() == ~PLASMA_BIT(1) && c.y() == ~PLASMA_BIT(2) && c.z() == ~PLASMA_BIT(3) && c.w() == ~PLASMA_BIT(4));

      c = a << 3;
      PLASMA_TEST_BOOL(c.x() == PLASMA_BIT(4) && c.y() == PLASMA_BIT(5) && c.z() == PLASMA_BIT(6) && c.w() == PLASMA_BIT(7));

      c = a >> 1;
      PLASMA_TEST_BOOL(c.x() == PLASMA_BIT(0) && c.y() == PLASMA_BIT(1) && c.z() == PLASMA_BIT(2) && c.w() == PLASMA_BIT(3));

      plSimdVec4i s(1, 2, 3, 4);
      c = a << s;
      PLASMA_TEST_BOOL(c.x() == PLASMA_BIT(2) && c.y() == PLASMA_BIT(4) && c.z() == PLASMA_BIT(6) && c.w() == PLASMA_BIT(8));

      c = b >> s;
      PLASMA_TEST_BOOL(c.x() == PLASMA_BIT(3) && c.y() == PLASMA_BIT(1) && c.z() == PLASMA_BIT(0) && c.w() == PLASMA_BIT(0));
    }

    {
      plSimdVec4i a(-3, 5, -7, 9);
      plSimdVec4i b(8, 6, 4, 2);

      plSimdVec4i c = a;
      c += b;
      PLASMA_TEST_BOOL(c.x() == 5 && c.y() == 11 && c.z() == -3 && c.w() == 11);

      c = a;
      c -= b;
      PLASMA_TEST_BOOL(c.x() == -11 && c.y() == -1 && c.z() == -11 && c.w() == 7);
    }

    {
      plSimdVec4i a(PLASMA_BIT(1), PLASMA_BIT(2), PLASMA_BIT(3), PLASMA_BIT(4));
      plSimdVec4i b(PLASMA_BIT(4), PLASMA_BIT(3), PLASMA_BIT(3), PLASMA_BIT(5) - 1);

      plSimdVec4i c = a;
      c |= b;
      PLASMA_TEST_BOOL(c.x() == (PLASMA_BIT(1) | PLASMA_BIT(4)) && c.y() == (PLASMA_BIT(2) | PLASMA_BIT(3)) && c.z() == PLASMA_BIT(3) && c.w() == PLASMA_BIT(5) - 1);

      c = a;
      c &= b;
      PLASMA_TEST_BOOL(c.x() == 0 && c.y() == 0 && c.z() == PLASMA_BIT(3) && c.w() == PLASMA_BIT(4));

      c = a;
      c ^= b;
      PLASMA_TEST_BOOL(c.x() == (PLASMA_BIT(1) | PLASMA_BIT(4)) && c.y() == (PLASMA_BIT(2) | PLASMA_BIT(3)) && c.z() == 0 && c.w() == PLASMA_BIT(4) - 1);

      c = a;
      c <<= 3;
      PLASMA_TEST_BOOL(c.x() == PLASMA_BIT(4) && c.y() == PLASMA_BIT(5) && c.z() == PLASMA_BIT(6) && c.w() == PLASMA_BIT(7));

      c = a;
      c >>= 1;
      PLASMA_TEST_BOOL(c.x() == PLASMA_BIT(0) && c.y() == PLASMA_BIT(1) && c.z() == PLASMA_BIT(2) && c.w() == PLASMA_BIT(3));

      c = plSimdVec4i(-2, -4, -7, -8);
      c >>= 1;
      PLASMA_TEST_BOOL(c.x() == -1 && c.y() == -2 && c.z() == -4 && c.w() == -4);
    }

    {
      plSimdVec4i a(-3, 5, -7, 9);
      plSimdVec4i b(8, 6, 4, 2);
      plSimdVec4i c;

      c = a.CompMin(b);
      PLASMA_TEST_BOOL(c.x() == -3 && c.y() == 5 && c.z() == -7 && c.w() == 2);

      c = a.CompMax(b);
      PLASMA_TEST_BOOL(c.x() == 8 && c.y() == 6 && c.z() == 4 && c.w() == 9);

      c = a.Abs();
      PLASMA_TEST_BOOL(c.x() == 3 && c.y() == 5 && c.z() == 7 && c.w() == 9);

      plSimdVec4b cmp(false, true, false, true);
      c = plSimdVec4i::Select(cmp, a, b);
      PLASMA_TEST_BOOL(c.x() == 8 && c.y() == 5 && c.z() == 4 && c.w() == 9);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Comparison")
  {
    plSimdVec4i a(-7, 5, 4, 3);
    plSimdVec4i b(8, 6, 4, -2);
    plSimdVec4b cmp;

    cmp = a == b;
    PLASMA_TEST_BOOL(!cmp.x() && !cmp.y() && cmp.z() && !cmp.w());

    cmp = a != b;
    PLASMA_TEST_BOOL(cmp.x() && cmp.y() && !cmp.z() && cmp.w());

    cmp = a <= b;
    PLASMA_TEST_BOOL(cmp.x() && cmp.y() && cmp.z() && !cmp.w());

    cmp = a < b;
    PLASMA_TEST_BOOL(cmp.x() && cmp.y() && !cmp.z() && !cmp.w());

    cmp = a >= b;
    PLASMA_TEST_BOOL(!cmp.x() && !cmp.y() && cmp.z() && cmp.w());

    cmp = a > b;
    PLASMA_TEST_BOOL(!cmp.x() && !cmp.y() && !cmp.z() && cmp.w());
  }
}