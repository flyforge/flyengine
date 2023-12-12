#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Vec4.h>
#include <Foundation/SimdMath/SimdVec4f.h>

namespace
{
  static bool AllCompSame(const plSimdFloat& a)
  {
    // Make sure all components are the same
    plSimdVec4f test;
    test.m_v = a.m_v;
    return test.x() == test.y() && test.x() == test.z() && test.x() == test.w();
  }

  template <plMathAcc::Enum acc>
  static void TestLength(const plSimdVec4f& a, float r[4], const plSimdFloat& eps)
  {
    plSimdFloat l1 = a.GetLength<1, acc>();
    plSimdFloat l2 = a.GetLength<2, acc>();
    plSimdFloat l3 = a.GetLength<3, acc>();
    plSimdFloat l4 = a.GetLength<4, acc>();
    PLASMA_TEST_FLOAT(l1, r[0], eps);
    PLASMA_TEST_FLOAT(l2, r[1], eps);
    PLASMA_TEST_FLOAT(l3, r[2], eps);
    PLASMA_TEST_FLOAT(l4, r[3], eps);
    PLASMA_TEST_BOOL(AllCompSame(l1));
    PLASMA_TEST_BOOL(AllCompSame(l2));
    PLASMA_TEST_BOOL(AllCompSame(l3));
    PLASMA_TEST_BOOL(AllCompSame(l4));
  }

  template <plMathAcc::Enum acc>
  static void TestInvLength(const plSimdVec4f& a, float r[4], const plSimdFloat& eps)
  {
    plSimdFloat l1 = a.GetInvLength<1, acc>();
    plSimdFloat l2 = a.GetInvLength<2, acc>();
    plSimdFloat l3 = a.GetInvLength<3, acc>();
    plSimdFloat l4 = a.GetInvLength<4, acc>();
    PLASMA_TEST_FLOAT(l1, r[0], eps);
    PLASMA_TEST_FLOAT(l2, r[1], eps);
    PLASMA_TEST_FLOAT(l3, r[2], eps);
    PLASMA_TEST_FLOAT(l4, r[3], eps);
    PLASMA_TEST_BOOL(AllCompSame(l1));
    PLASMA_TEST_BOOL(AllCompSame(l2));
    PLASMA_TEST_BOOL(AllCompSame(l3));
    PLASMA_TEST_BOOL(AllCompSame(l4));
  }

  template <plMathAcc::Enum acc>
  static void TestNormalize(const plSimdVec4f& a, plSimdVec4f n[4], plSimdFloat r[4], const plSimdFloat& eps)
  {
    plSimdVec4f n1 = a.GetNormalized<1, acc>();
    plSimdVec4f n2 = a.GetNormalized<2, acc>();
    plSimdVec4f n3 = a.GetNormalized<3, acc>();
    plSimdVec4f n4 = a.GetNormalized<4, acc>();
    PLASMA_TEST_BOOL(n1.IsEqual(n[0], eps).AllSet());
    PLASMA_TEST_BOOL(n2.IsEqual(n[1], eps).AllSet());
    PLASMA_TEST_BOOL(n3.IsEqual(n[2], eps).AllSet());
    PLASMA_TEST_BOOL(n4.IsEqual(n[3], eps).AllSet());

    plSimdVec4f a1 = a;
    plSimdVec4f a2 = a;
    plSimdVec4f a3 = a;
    plSimdVec4f a4 = a;

    plSimdFloat l1 = a1.GetLengthAndNormalize<1, acc>();
    plSimdFloat l2 = a2.GetLengthAndNormalize<2, acc>();
    plSimdFloat l3 = a3.GetLengthAndNormalize<3, acc>();
    plSimdFloat l4 = a4.GetLengthAndNormalize<4, acc>();
    PLASMA_TEST_FLOAT(l1, r[0], eps);
    PLASMA_TEST_FLOAT(l2, r[1], eps);
    PLASMA_TEST_FLOAT(l3, r[2], eps);
    PLASMA_TEST_FLOAT(l4, r[3], eps);
    PLASMA_TEST_BOOL(AllCompSame(l1));
    PLASMA_TEST_BOOL(AllCompSame(l2));
    PLASMA_TEST_BOOL(AllCompSame(l3));
    PLASMA_TEST_BOOL(AllCompSame(l4));

    PLASMA_TEST_BOOL(a1.IsEqual(n[0], eps).AllSet());
    PLASMA_TEST_BOOL(a2.IsEqual(n[1], eps).AllSet());
    PLASMA_TEST_BOOL(a3.IsEqual(n[2], eps).AllSet());
    PLASMA_TEST_BOOL(a4.IsEqual(n[3], eps).AllSet());

    PLASMA_TEST_BOOL(a1.IsNormalized<1>(eps));
    PLASMA_TEST_BOOL(a2.IsNormalized<2>(eps));
    PLASMA_TEST_BOOL(a3.IsNormalized<3>(eps));
    PLASMA_TEST_BOOL(a4.IsNormalized<4>(eps));
    PLASMA_TEST_BOOL(!a1.IsNormalized<2>(eps));
    PLASMA_TEST_BOOL(!a2.IsNormalized<3>(eps));
    PLASMA_TEST_BOOL(!a3.IsNormalized<4>(eps));

    a1 = a;
    a1.Normalize<1, acc>();
    a2 = a;
    a2.Normalize<2, acc>();
    a3 = a;
    a3.Normalize<3, acc>();
    a4 = a;
    a4.Normalize<4, acc>();
    PLASMA_TEST_BOOL(a1.IsEqual(n[0], eps).AllSet());
    PLASMA_TEST_BOOL(a2.IsEqual(n[1], eps).AllSet());
    PLASMA_TEST_BOOL(a3.IsEqual(n[2], eps).AllSet());
    PLASMA_TEST_BOOL(a4.IsEqual(n[3], eps).AllSet());
  }

  template <plMathAcc::Enum acc>
  static void TestNormalizeIfNotZero(const plSimdVec4f& a, plSimdVec4f n[4], const plSimdFloat& eps)
  {
    plSimdVec4f a1 = a;
    a1.NormalizeIfNotZero<1>(eps);
    plSimdVec4f a2 = a;
    a2.NormalizeIfNotZero<2>(eps);
    plSimdVec4f a3 = a;
    a3.NormalizeIfNotZero<3>(eps);
    plSimdVec4f a4 = a;
    a4.NormalizeIfNotZero<4>(eps);
    PLASMA_TEST_BOOL(a1.IsEqual(n[0], eps).AllSet());
    PLASMA_TEST_BOOL(a2.IsEqual(n[1], eps).AllSet());
    PLASMA_TEST_BOOL(a3.IsEqual(n[2], eps).AllSet());
    PLASMA_TEST_BOOL(a4.IsEqual(n[3], eps).AllSet());

    PLASMA_TEST_BOOL(a1.IsNormalized<1>(eps));
    PLASMA_TEST_BOOL(a2.IsNormalized<2>(eps));
    PLASMA_TEST_BOOL(a3.IsNormalized<3>(eps));
    PLASMA_TEST_BOOL(a4.IsNormalized<4>(eps));
    PLASMA_TEST_BOOL(!a1.IsNormalized<2>(eps));
    PLASMA_TEST_BOOL(!a2.IsNormalized<3>(eps));
    PLASMA_TEST_BOOL(!a3.IsNormalized<4>(eps));

    plSimdVec4f b(eps);
    b.NormalizeIfNotZero<4>(eps);
    PLASMA_TEST_BOOL(b.IsZero<4>());
  }
} // namespace

PLASMA_CREATE_SIMPLE_TEST(SimdMath, SimdVec4f)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with NaN.
    plSimdVec4f vDefCtor;
    PLASMA_TEST_BOOL(vDefCtor.IsNaN<4>());
#else
// GCC assumes that the contents of the memory prior to the placement constructor doesn't matter
// So it optimizes away the initialization.
#  if PLASMA_DISABLED(PLASMA_COMPILER_GCC)
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(16) float testBlock[4] = {1, 2, 3, 4};
    plSimdVec4f* pDefCtor = ::new ((void*)&testBlock[0]) plSimdVec4f;
    PLASMA_TEST_BOOL(pDefCtor->x() == 1.0f && pDefCtor->y() == 2.0f && pDefCtor->z() == 3.0f && pDefCtor->w() == 4.0f);
#  endif
#endif

    // Make sure the class didn't accidentally change in size.
#if PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE
    PLASMA_CHECK_AT_COMPILETIME(sizeof(plSimdVec4f) == 16);
    PLASMA_CHECK_AT_COMPILETIME(PLASMA_ALIGNMENT_OF(plSimdVec4f) == 16);
#endif

    plSimdVec4f vInit1F(2.0f);
    PLASMA_TEST_BOOL(vInit1F.x() == 2.0f && vInit1F.y() == 2.0f && vInit1F.z() == 2.0f && vInit1F.w() == 2.0f);

    plSimdFloat a(3.0f);
    plSimdVec4f vInit1SF(a);
    PLASMA_TEST_BOOL(vInit1SF.x() == 3.0f && vInit1SF.y() == 3.0f && vInit1SF.z() == 3.0f && vInit1SF.w() == 3.0f);

    plSimdVec4f vInit4F(1.0f, 2.0f, 3.0f, 4.0f);
    PLASMA_TEST_BOOL(vInit4F.x() == 1.0f && vInit4F.y() == 2.0f && vInit4F.z() == 3.0f && vInit4F.w() == 4.0f);

    // Make sure all components have the correct values
#if PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE && PLASMA_ENABLED(PLASMA_COMPILER_MSVC)
    PLASMA_TEST_BOOL(
      vInit4F.m_v.m128_f32[0] == 1.0f && vInit4F.m_v.m128_f32[1] == 2.0f && vInit4F.m_v.m128_f32[2] == 3.0f && vInit4F.m_v.m128_f32[3] == 4.0f);
#endif

    plSimdVec4f vCopy(vInit4F);
    PLASMA_TEST_BOOL(vCopy.x() == 1.0f && vCopy.y() == 2.0f && vCopy.z() == 3.0f && vCopy.w() == 4.0f);

    plSimdVec4f vZero = plSimdVec4f::ZeroVector();
    PLASMA_TEST_BOOL(vZero.x() == 0.0f && vZero.y() == 0.0f && vZero.z() == 0.0f && vZero.w() == 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Setter")
  {
    plSimdVec4f a;
    a.Set(2.0f);
    PLASMA_TEST_BOOL(a.x() == 2.0f && a.y() == 2.0f && a.z() == 2.0f && a.w() == 2.0f);

    plSimdVec4f b;
    b.Set(1.0f, 2.0f, 3.0f, 4.0f);
    PLASMA_TEST_BOOL(b.x() == 1.0f && b.y() == 2.0f && b.z() == 3.0f && b.w() == 4.0f);

    b.SetX(5.0f);
    PLASMA_TEST_BOOL(b.x() == 5.0f && b.y() == 2.0f && b.z() == 3.0f && b.w() == 4.0f);

    b.SetY(6.0f);
    PLASMA_TEST_BOOL(b.x() == 5.0f && b.y() == 6.0f && b.z() == 3.0f && b.w() == 4.0f);

    b.SetZ(7.0f);
    PLASMA_TEST_BOOL(b.x() == 5.0f && b.y() == 6.0f && b.z() == 7.0f && b.w() == 4.0f);

    b.SetW(8.0f);
    PLASMA_TEST_BOOL(b.x() == 5.0f && b.y() == 6.0f && b.z() == 7.0f && b.w() == 8.0f);

    plSimdVec4f c;
    c.SetZero();
    PLASMA_TEST_BOOL(c.x() == 0.0f && c.y() == 0.0f && c.z() == 0.0f && c.w() == 0.0f);

    {
      float testBlock[4] = {1, 2, 3, 4};
      plSimdVec4f x;
      x.Load<1>(testBlock);
      PLASMA_TEST_BOOL(x.x() == 1.0f && x.y() == 0.0f && x.z() == 0.0f && x.w() == 0.0f);

      plSimdVec4f xy;
      xy.Load<2>(testBlock);
      PLASMA_TEST_BOOL(xy.x() == 1.0f && xy.y() == 2.0f && xy.z() == 0.0f && xy.w() == 0.0f);

      plSimdVec4f xyz;
      xyz.Load<3>(testBlock);
      PLASMA_TEST_BOOL(xyz.x() == 1.0f && xyz.y() == 2.0f && xyz.z() == 3.0f && xyz.w() == 0.0f);

      plSimdVec4f xyzw;
      xyzw.Load<4>(testBlock);
      PLASMA_TEST_BOOL(xyzw.x() == 1.0f && xyzw.y() == 2.0f && xyzw.z() == 3.0f && xyzw.w() == 4.0f);

      PLASMA_TEST_BOOL(xyzw.GetComponent(0) == 1.0f);
      PLASMA_TEST_BOOL(xyzw.GetComponent(1) == 2.0f);
      PLASMA_TEST_BOOL(xyzw.GetComponent(2) == 3.0f);
      PLASMA_TEST_BOOL(xyzw.GetComponent(3) == 4.0f);
      PLASMA_TEST_BOOL(xyzw.GetComponent(4) == 4.0f);

      // Make sure all components have the correct values
#if PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE && PLASMA_ENABLED(PLASMA_COMPILER_MSVC)
      PLASMA_TEST_BOOL(xyzw.m_v.m128_f32[0] == 1.0f && xyzw.m_v.m128_f32[1] == 2.0f && xyzw.m_v.m128_f32[2] == 3.0f && xyzw.m_v.m128_f32[3] == 4.0f);
#endif
    }

    {
      float testBlock[4] = {7, 7, 7, 7};
      float mem[4] = {};

      plSimdVec4f b2(1, 2, 3, 4);

      memcpy(mem, testBlock, 16);
      b2.Store<1>(mem);
      PLASMA_TEST_BOOL(mem[0] == 1.0f && mem[1] == 7.0f && mem[2] == 7.0f && mem[3] == 7.0f);

      memcpy(mem, testBlock, 16);
      b2.Store<2>(mem);
      PLASMA_TEST_BOOL(mem[0] == 1.0f && mem[1] == 2.0f && mem[2] == 7.0f && mem[3] == 7.0f);

      memcpy(mem, testBlock, 16);
      b2.Store<3>(mem);
      PLASMA_TEST_BOOL(mem[0] == 1.0f && mem[1] == 2.0f && mem[2] == 3.0f && mem[3] == 7.0f);

      memcpy(mem, testBlock, 16);
      b2.Store<4>(mem);
      PLASMA_TEST_BOOL(mem[0] == 1.0f && mem[1] == 2.0f && mem[2] == 3.0f && mem[3] == 4.0f);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Functions")
  {
    {
      plSimdVec4f a(1.0f, 2.0f, 4.0f, 8.0f);
      plSimdVec4f b(1.0f, 0.5f, 0.25f, 0.125f);

      PLASMA_TEST_BOOL(a.GetReciprocal().IsEqual(b, plMath::SmallEpsilon<float>()).AllSet());
      PLASMA_TEST_BOOL(a.GetReciprocal<plMathAcc::FULL>().IsEqual(b, plMath::SmallEpsilon<float>()).AllSet());
      PLASMA_TEST_BOOL(a.GetReciprocal<plMathAcc::BITS_23>().IsEqual(b, plMath::DefaultEpsilon<float>()).AllSet());
      PLASMA_TEST_BOOL(a.GetReciprocal<plMathAcc::BITS_12>().IsEqual(b, plMath::HugeEpsilon<float>()).AllSet());
    }

    {
      plSimdVec4f a(1.0f, 2.0f, 4.0f, 8.0f);
      plSimdVec4f b(1.0f, plMath::Sqrt(2.0f), plMath::Sqrt(4.0f), plMath::Sqrt(8.0f));

      PLASMA_TEST_BOOL(a.GetSqrt().IsEqual(b, plMath::SmallEpsilon<float>()).AllSet());
      PLASMA_TEST_BOOL(a.GetSqrt<plMathAcc::FULL>().IsEqual(b, plMath::SmallEpsilon<float>()).AllSet());
      PLASMA_TEST_BOOL(a.GetSqrt<plMathAcc::BITS_23>().IsEqual(b, plMath::DefaultEpsilon<float>()).AllSet());
      PLASMA_TEST_BOOL(a.GetSqrt<plMathAcc::BITS_12>().IsEqual(b, plMath::HugeEpsilon<float>()).AllSet());
    }

    {
      plSimdVec4f a(1.0f, 2.0f, 4.0f, 8.0f);
      plSimdVec4f b(1.0f, 1.0f / plMath::Sqrt(2.0f), 1.0f / plMath::Sqrt(4.0f), 1.0f / plMath::Sqrt(8.0f));

      PLASMA_TEST_BOOL(a.GetInvSqrt().IsEqual(b, plMath::SmallEpsilon<float>()).AllSet());
      PLASMA_TEST_BOOL(a.GetInvSqrt<plMathAcc::FULL>().IsEqual(b, plMath::SmallEpsilon<float>()).AllSet());
      PLASMA_TEST_BOOL(a.GetInvSqrt<plMathAcc::BITS_23>().IsEqual(b, plMath::DefaultEpsilon<float>()).AllSet());
      PLASMA_TEST_BOOL(a.GetInvSqrt<plMathAcc::BITS_12>().IsEqual(b, plMath::HugeEpsilon<float>()).AllSet());
    }

    {
      plSimdVec4f a(2.0f, -2.0f, 4.0f, -8.0f);
      float r[4];
      r[0] = 2.0f;
      r[1] = plVec2(a.x(), a.y()).GetLength();
      r[2] = plVec3(a.x(), a.y(), a.z()).GetLength();
      r[3] = plVec4(a.x(), a.y(), a.z(), a.w()).GetLength();

      PLASMA_TEST_FLOAT(a.GetLength<1>(), r[0], plMath::SmallEpsilon<float>());
      PLASMA_TEST_FLOAT(a.GetLength<2>(), r[1], plMath::SmallEpsilon<float>());
      PLASMA_TEST_FLOAT(a.GetLength<3>(), r[2], plMath::SmallEpsilon<float>());
      PLASMA_TEST_FLOAT(a.GetLength<4>(), r[3], plMath::SmallEpsilon<float>());

      TestLength<plMathAcc::FULL>(a, r, plMath::SmallEpsilon<float>());
      TestLength<plMathAcc::BITS_23>(a, r, plMath::DefaultEpsilon<float>());
      TestLength<plMathAcc::BITS_12>(a, r, 0.01f);
    }

    {
      plSimdVec4f a(2.0f, -2.0f, 4.0f, -8.0f);
      float r[4];
      r[0] = 0.5f;
      r[1] = 1.0f / plVec2(a.x(), a.y()).GetLength();
      r[2] = 1.0f / plVec3(a.x(), a.y(), a.z()).GetLength();
      r[3] = 1.0f / plVec4(a.x(), a.y(), a.z(), a.w()).GetLength();

      PLASMA_TEST_FLOAT(a.GetInvLength<1>(), r[0], plMath::SmallEpsilon<float>());
      PLASMA_TEST_FLOAT(a.GetInvLength<2>(), r[1], plMath::SmallEpsilon<float>());
      PLASMA_TEST_FLOAT(a.GetInvLength<3>(), r[2], plMath::SmallEpsilon<float>());
      PLASMA_TEST_FLOAT(a.GetInvLength<4>(), r[3], plMath::SmallEpsilon<float>());

      TestInvLength<plMathAcc::FULL>(a, r, plMath::SmallEpsilon<float>());
      TestInvLength<plMathAcc::BITS_23>(a, r, plMath::DefaultEpsilon<float>());
      TestInvLength<plMathAcc::BITS_12>(a, r, plMath::HugeEpsilon<float>());
    }

    {
      plSimdVec4f a(2.0f, -2.0f, 4.0f, -8.0f);
      float r[4];
      r[0] = 2.0f * 2.0f;
      r[1] = plVec2(a.x(), a.y()).GetLengthSquared();
      r[2] = plVec3(a.x(), a.y(), a.z()).GetLengthSquared();
      r[3] = plVec4(a.x(), a.y(), a.z(), a.w()).GetLengthSquared();

      PLASMA_TEST_FLOAT(a.GetLengthSquared<1>(), r[0], plMath::SmallEpsilon<float>());
      PLASMA_TEST_FLOAT(a.GetLengthSquared<2>(), r[1], plMath::SmallEpsilon<float>());
      PLASMA_TEST_FLOAT(a.GetLengthSquared<3>(), r[2], plMath::SmallEpsilon<float>());
      PLASMA_TEST_FLOAT(a.GetLengthSquared<4>(), r[3], plMath::SmallEpsilon<float>());
    }

    {
      plSimdVec4f a(2.0f, -2.0f, 4.0f, -8.0f);
      plSimdFloat r[4];
      r[0] = 2.0f;
      r[1] = plVec2(a.x(), a.y()).GetLength();
      r[2] = plVec3(a.x(), a.y(), a.z()).GetLength();
      r[3] = plVec4(a.x(), a.y(), a.z(), a.w()).GetLength();

      plSimdVec4f n[4];
      n[0] = a / r[0];
      n[1] = a / r[1];
      n[2] = a / r[2];
      n[3] = a / r[3];

      TestNormalize<plMathAcc::FULL>(a, n, r, plMath::SmallEpsilon<float>());
      TestNormalize<plMathAcc::BITS_23>(a, n, r, plMath::DefaultEpsilon<float>());
      TestNormalize<plMathAcc::BITS_12>(a, n, r, 0.01f);
    }

    {
      plSimdVec4f a(2.0f, -2.0f, 4.0f, -8.0f);
      plSimdVec4f n[4];
      n[0] = a / 2.0f;
      n[1] = a / plVec2(a.x(), a.y()).GetLength();
      n[2] = a / plVec3(a.x(), a.y(), a.z()).GetLength();
      n[3] = a / plVec4(a.x(), a.y(), a.z(), a.w()).GetLength();

      TestNormalizeIfNotZero<plMathAcc::FULL>(a, n, plMath::SmallEpsilon<float>());
      TestNormalizeIfNotZero<plMathAcc::BITS_23>(a, n, plMath::DefaultEpsilon<float>());
      TestNormalizeIfNotZero<plMathAcc::BITS_12>(a, n, plMath::HugeEpsilon<float>());
    }

    {
      plSimdVec4f a;

      a.Set(0.0f, 2.0f, 0.0f, 0.0f);
      PLASMA_TEST_BOOL(a.IsZero<1>());
      PLASMA_TEST_BOOL(!a.IsZero<2>());

      a.Set(0.0f, 0.0f, 3.0f, 0.0f);
      PLASMA_TEST_BOOL(a.IsZero<2>());
      PLASMA_TEST_BOOL(!a.IsZero<3>());

      a.Set(0.0f, 0.0f, 0.0f, 4.0f);
      PLASMA_TEST_BOOL(a.IsZero<3>());
      PLASMA_TEST_BOOL(!a.IsZero<4>());

      float smallEps = plMath::SmallEpsilon<float>();
      a.Set(smallEps, 2.0f, smallEps, smallEps);
      PLASMA_TEST_BOOL(a.IsZero<1>(plMath::DefaultEpsilon<float>()));
      PLASMA_TEST_BOOL(!a.IsZero<2>(plMath::DefaultEpsilon<float>()));

      a.Set(smallEps, smallEps, 3.0f, smallEps);
      PLASMA_TEST_BOOL(a.IsZero<2>(plMath::DefaultEpsilon<float>()));
      PLASMA_TEST_BOOL(!a.IsZero<3>(plMath::DefaultEpsilon<float>()));

      a.Set(smallEps, smallEps, smallEps, 4.0f);
      PLASMA_TEST_BOOL(a.IsZero<3>(plMath::DefaultEpsilon<float>()));
      PLASMA_TEST_BOOL(!a.IsZero<4>(plMath::DefaultEpsilon<float>()));
    }

    {
      plSimdVec4f a;

      float NaN = plMath::NaN<float>();
      float Inf = plMath::Infinity<float>();

      a.Set(NaN, 1.0f, NaN, NaN);
      PLASMA_TEST_BOOL(a.IsNaN<1>());
      PLASMA_TEST_BOOL(a.IsNaN<2>());
      PLASMA_TEST_BOOL(!a.IsValid<2>());

      a.Set(Inf, 1.0f, NaN, NaN);
      PLASMA_TEST_BOOL(!a.IsNaN<1>());
      PLASMA_TEST_BOOL(!a.IsNaN<2>());
      PLASMA_TEST_BOOL(!a.IsValid<2>());

      a.Set(1.0f, 2.0f, Inf, NaN);
      PLASMA_TEST_BOOL(a.IsNaN<4>());
      PLASMA_TEST_BOOL(!a.IsNaN<3>());
      PLASMA_TEST_BOOL(a.IsValid<2>());
      PLASMA_TEST_BOOL(!a.IsValid<3>());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Swizzle")
  {
    plSimdVec4f a(3.0f, 5.0f, 7.0f, 9.0f);

    plSimdVec4f b = a.Get<plSwizzle::XXXX>();
    PLASMA_TEST_BOOL(b.x() == 3.0f && b.y() == 3.0f && b.z() == 3.0f && b.w() == 3.0f);

    b = a.Get<plSwizzle::YYYX>();
    PLASMA_TEST_BOOL(b.x() == 5.0f && b.y() == 5.0f && b.z() == 5.0f && b.w() == 3.0f);

    b = a.Get<plSwizzle::ZZZX>();
    PLASMA_TEST_BOOL(b.x() == 7.0f && b.y() == 7.0f && b.z() == 7.0f && b.w() == 3.0f);

    b = a.Get<plSwizzle::WWWX>();
    PLASMA_TEST_BOOL(b.x() == 9.0f && b.y() == 9.0f && b.z() == 9.0f && b.w() == 3.0f);

    b = a.Get<plSwizzle::WZYX>();
    PLASMA_TEST_BOOL(b.x() == 9.0f && b.y() == 7.0f && b.z() == 5.0f && b.w() == 3.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Operators")
  {
    {
      plSimdVec4f a(-3.0f, 5.0f, -7.0f, 9.0f);

      plSimdVec4f b = -a;
      PLASMA_TEST_BOOL(b.x() == 3.0f && b.y() == -5.0f && b.z() == 7.0f && b.w() == -9.0f);

      b.Set(8.0f, 6.0f, 4.0f, 2.0f);
      plSimdVec4f c;
      c = a + b;
      PLASMA_TEST_BOOL(c.x() == 5.0f && c.y() == 11.0f && c.z() == -3.0f && c.w() == 11.0f);

      c = a - b;
      PLASMA_TEST_BOOL(c.x() == -11.0f && c.y() == -1.0f && c.z() == -11.0f && c.w() == 7.0f);

      c = a * plSimdFloat(3.0f);
      PLASMA_TEST_BOOL(c.x() == -9.0f && c.y() == 15.0f && c.z() == -21.0f && c.w() == 27.0f);

      c = a / plSimdFloat(2.0f);
      PLASMA_TEST_BOOL(c.x() == -1.5f && c.y() == 2.5f && c.z() == -3.5f && c.w() == 4.5f);

      c = a.CompMul(b);
      PLASMA_TEST_BOOL(c.x() == -24.0f && c.y() == 30.0f && c.z() == -28.0f && c.w() == 18.0f);

      plSimdVec4f divRes(-0.375f, 5.0f / 6.0f, -1.75f, 4.5f);
      plSimdVec4f d1 = a.CompDiv(b);
      plSimdVec4f d2 = a.CompDiv<plMathAcc::FULL>(b);
      plSimdVec4f d3 = a.CompDiv<plMathAcc::BITS_23>(b);
      plSimdVec4f d4 = a.CompDiv<plMathAcc::BITS_12>(b);

      PLASMA_TEST_BOOL(d1.IsEqual(divRes, plMath::SmallEpsilon<float>()).AllSet());
      PLASMA_TEST_BOOL(d2.IsEqual(divRes, plMath::SmallEpsilon<float>()).AllSet());
      PLASMA_TEST_BOOL(d3.IsEqual(divRes, plMath::DefaultEpsilon<float>()).AllSet());
      PLASMA_TEST_BOOL(d4.IsEqual(divRes, 0.01f).AllSet());
    }

    {
      plSimdVec4f a(-3.4f, 5.4f, -7.6f, 9.6f);
      plSimdVec4f b(8.0f, 6.0f, 4.0f, 2.0f);
      plSimdVec4f c;

      c = a.CompMin(b);
      PLASMA_TEST_BOOL(c.x() == -3.4f && c.y() == 5.4f && c.z() == -7.6f && c.w() == 2.0f);

      c = a.CompMax(b);
      PLASMA_TEST_BOOL(c.x() == 8.0f && c.y() == 6.0f && c.z() == 4.0f && c.w() == 9.6f);

      c = a.Abs();
      PLASMA_TEST_BOOL(c.x() == 3.4f && c.y() == 5.4f && c.z() == 7.6f && c.w() == 9.6f);

      c = a.Round();
      PLASMA_TEST_BOOL(c.x() == -3.0f && c.y() == 5.0f && c.z() == -8.0f && c.w() == 10.0f);

      c = a.Floor();
      PLASMA_TEST_BOOL(c.x() == -4.0f && c.y() == 5.0f && c.z() == -8.0f && c.w() == 9.0f);

      c = a.Ceil();
      PLASMA_TEST_BOOL(c.x() == -3.0f && c.y() == 6.0f && c.z() == -7.0f && c.w() == 10.0f);

      c = a.Trunc();
      PLASMA_TEST_BOOL(c.x() == -3.0f && c.y() == 5.0f && c.z() == -7.0f && c.w() == 9.0f);

      c = a.Fraction();
      PLASMA_TEST_BOOL(c.IsEqual(plSimdVec4f(-0.4f, 0.4f, -0.6f, 0.6f), plMath::SmallEpsilon<float>()).AllSet());
    }

    {
      plSimdVec4f a(-3.0f, 5.0f, -7.0f, 9.0f);
      plSimdVec4f b(8.0f, 6.0f, 4.0f, 2.0f);

      plSimdVec4b cmp(true, false, false, true);
      plSimdVec4f c;

      c = a.FlipSign(cmp);
      PLASMA_TEST_BOOL(c.x() == 3.0f && c.y() == 5.0f && c.z() == -7.0f && c.w() == -9.0f);

      c = plSimdVec4f::Select(cmp, b, a);
      PLASMA_TEST_BOOL(c.x() == 8.0f && c.y() == 5.0f && c.z() == -7.0f && c.w() == 2.0f);

      c = plSimdVec4f::Select(cmp, a, b);
      PLASMA_TEST_BOOL(c.x() == -3.0f && c.y() == 6.0f && c.z() == 4.0f && c.w() == 9.0f);
    }

    {
      plSimdVec4f a(-3.0f, 5.0f, -7.0f, 9.0f);
      plSimdVec4f b(8.0f, 6.0f, 4.0f, 2.0f);

      plSimdVec4f c = a;
      c += b;
      PLASMA_TEST_BOOL(c.x() == 5.0f && c.y() == 11.0f && c.z() == -3.0f && c.w() == 11.0f);

      c = a;
      c -= b;
      PLASMA_TEST_BOOL(c.x() == -11.0f && c.y() == -1.0f && c.z() == -11.0f && c.w() == 7.0f);

      c = a;
      c *= plSimdFloat(3.0f);
      PLASMA_TEST_BOOL(c.x() == -9.0f && c.y() == 15.0f && c.z() == -21.0f && c.w() == 27.0f);

      c = a;
      c /= plSimdFloat(2.0f);
      PLASMA_TEST_BOOL(c.x() == -1.5f && c.y() == 2.5f && c.z() == -3.5f && c.w() == 4.5f);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Comparison")
  {
    plSimdVec4f a(7.0f, 5.0f, 4.0f, 3.0f);
    plSimdVec4f b(8.0f, 6.0f, 4.0f, 2.0f);
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

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Advanced Operators")
  {
    {
      plSimdVec4f a(-3.0f, 5.0f, -7.0f, 9.0f);

      PLASMA_TEST_FLOAT(a.HorizontalSum<1>(), -3.0f, 0.0f);
      PLASMA_TEST_FLOAT(a.HorizontalSum<2>(), 2.0f, 0.0f);
      PLASMA_TEST_FLOAT(a.HorizontalSum<3>(), -5.0f, 0.0f);
      PLASMA_TEST_FLOAT(a.HorizontalSum<4>(), 4.0f, 0.0f);
      PLASMA_TEST_BOOL(AllCompSame(a.HorizontalSum<1>()));
      PLASMA_TEST_BOOL(AllCompSame(a.HorizontalSum<2>()));
      PLASMA_TEST_BOOL(AllCompSame(a.HorizontalSum<3>()));
      PLASMA_TEST_BOOL(AllCompSame(a.HorizontalSum<4>()));

      PLASMA_TEST_FLOAT(a.HorizontalMin<1>(), -3.0f, 0.0f);
      PLASMA_TEST_FLOAT(a.HorizontalMin<2>(), -3.0f, 0.0f);
      PLASMA_TEST_FLOAT(a.HorizontalMin<3>(), -7.0f, 0.0f);
      PLASMA_TEST_FLOAT(a.HorizontalMin<4>(), -7.0f, 0.0f);
      PLASMA_TEST_BOOL(AllCompSame(a.HorizontalMin<1>()));
      PLASMA_TEST_BOOL(AllCompSame(a.HorizontalMin<2>()));
      PLASMA_TEST_BOOL(AllCompSame(a.HorizontalMin<3>()));
      PLASMA_TEST_BOOL(AllCompSame(a.HorizontalMin<4>()));

      PLASMA_TEST_FLOAT(a.HorizontalMax<1>(), -3.0f, 0.0f);
      PLASMA_TEST_FLOAT(a.HorizontalMax<2>(), 5.0f, 0.0f);
      PLASMA_TEST_FLOAT(a.HorizontalMax<3>(), 5.0f, 0.0f);
      PLASMA_TEST_FLOAT(a.HorizontalMax<4>(), 9.0f, 0.0f);
      PLASMA_TEST_BOOL(AllCompSame(a.HorizontalMax<1>()));
      PLASMA_TEST_BOOL(AllCompSame(a.HorizontalMax<2>()));
      PLASMA_TEST_BOOL(AllCompSame(a.HorizontalMax<3>()));
      PLASMA_TEST_BOOL(AllCompSame(a.HorizontalMax<4>()));
    }

    {
      plSimdVec4f a(-3.0f, 5.0f, -7.0f, 9.0f);
      plSimdVec4f b(8.0f, 6.0f, 4.0f, 2.0f);

      PLASMA_TEST_FLOAT(a.Dot<1>(b), -24.0f, 0.0f);
      PLASMA_TEST_FLOAT(a.Dot<2>(b), 6.0f, 0.0f);
      PLASMA_TEST_FLOAT(a.Dot<3>(b), -22.0f, 0.0f);
      PLASMA_TEST_FLOAT(a.Dot<4>(b), -4.0f, 0.0f);
      PLASMA_TEST_BOOL(AllCompSame(a.Dot<1>(b)));
      PLASMA_TEST_BOOL(AllCompSame(a.Dot<2>(b)));
      PLASMA_TEST_BOOL(AllCompSame(a.Dot<3>(b)));
      PLASMA_TEST_BOOL(AllCompSame(a.Dot<4>(b)));
    }

    {
      plSimdVec4f a(1.0f, 2.0f, 3.0f, 0.0f);
      plSimdVec4f b(2.0f, -4.0f, 6.0f, 8.0f);

      plVec3 res = plVec3(a.x(), a.y(), a.z()).CrossRH(plVec3(b.x(), b.y(), b.z()));

      plSimdVec4f c = a.CrossRH(b);
      PLASMA_TEST_BOOL(c.x() == res.x);
      PLASMA_TEST_BOOL(c.y() == res.y);
      PLASMA_TEST_BOOL(c.z() == res.z);
    }

    {
      plSimdVec4f a(1.0f, 2.0f, 3.0f, 0.0f);
      plSimdVec4f b(2.0f, -4.0f, 6.0f, 0.0f);

      plVec3 res = plVec3(a.x(), a.y(), a.z()).CrossRH(plVec3(b.x(), b.y(), b.z()));

      plSimdVec4f c = a.CrossRH(b);
      PLASMA_TEST_BOOL(c.x() == res.x);
      PLASMA_TEST_BOOL(c.y() == res.y);
      PLASMA_TEST_BOOL(c.z() == res.z);
    }

    {
      plSimdVec4f a(-3.0f, 5.0f, -7.0f, 0.0f);
      plSimdVec4f b = a.GetOrthogonalVector();

      PLASMA_TEST_BOOL(!b.IsZero<3>());
      PLASMA_TEST_FLOAT(a.Dot<3>(b), 0.0f, 0.0f);
    }

    {
      plSimdVec4f a(-3.0f, 5.0f, -7.0f, 9.0f);
      plSimdVec4f b(8.0f, 6.0f, 4.0f, 2.0f);
      plSimdVec4f c(1.0f, 2.0f, 3.0f, 4.0f);
      plSimdVec4f d;

      d = plSimdVec4f::MulAdd(a, b, c);
      PLASMA_TEST_BOOL(d.x() == -23.0f && d.y() == 32.0f && d.z() == -25.0f && d.w() == 22.0f);

      d = plSimdVec4f::MulAdd(a, plSimdFloat(3.0f), c);
      PLASMA_TEST_BOOL(d.x() == -8.0f && d.y() == 17.0f && d.z() == -18.0f && d.w() == 31.0f);

      d = plSimdVec4f::MulSub(a, b, c);
      PLASMA_TEST_BOOL(d.x() == -25.0f && d.y() == 28.0f && d.z() == -31.0f && d.w() == 14.0f);

      d = plSimdVec4f::MulSub(a, plSimdFloat(3.0f), c);
      PLASMA_TEST_BOOL(d.x() == -10.0f && d.y() == 13.0f && d.z() == -24.0f && d.w() == 23.0f);

      d = plSimdVec4f::CopySign(b, a);
      PLASMA_TEST_BOOL(d.x() == -8.0f && d.y() == 6.0f && d.z() == -4.0f && d.w() == 2.0f);
    }
  }
}