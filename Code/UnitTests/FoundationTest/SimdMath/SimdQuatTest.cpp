#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdQuat.h>

PLASMA_CREATE_SIMPLE_TEST(SimdMath, SimdQuat)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    // In debug the default constructor initializes everything with NaN.
    plSimdQuat vDefCtor;
    PLASMA_TEST_BOOL(vDefCtor.IsNaN());
#else

#  if PLASMA_DISABLED(PLASMA_COMPILER_GCC)
    // Placement new of the default constructor should not have any effect on the previous data.
    alignas(16) float testBlock[4] = {1, 2, 3, 4};
    plSimdQuat* pDefCtor = ::new ((void*)&testBlock[0]) plSimdQuat;
    PLASMA_TEST_BOOL(pDefCtor->m_v.x() == 1.0f && pDefCtor->m_v.y() == 2.0f && pDefCtor->m_v.z() == 3.0f && pDefCtor->m_v.w() == 4.0f);
#  endif

#endif

    // Make sure the class didn't accidentally change in size.
#if PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE
    PLASMA_CHECK_AT_COMPILETIME(sizeof(plSimdQuat) == 16);
    PLASMA_CHECK_AT_COMPILETIME(PLASMA_ALIGNMENT_OF(plSimdQuat) == 16);
#endif
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IdentityQuaternion")
  {
    plSimdQuat q = plSimdQuat::IdentityQuaternion();

    PLASMA_TEST_BOOL(q.m_v.x() == 0.0f && q.m_v.y() == 0.0f && q.m_v.z() == 0.0f && q.m_v.w() == 1.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetIdentity")
  {
    plSimdQuat q(plSimdVec4f(1, 2, 3, 4));

    q.SetIdentity();

    PLASMA_TEST_BOOL(q.m_v.x() == 0.0f && q.m_v.y() == 0.0f && q.m_v.z() == 0.0f && q.m_v.w() == 1.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetFromAxisAndAngle / operator* (quat, vec)")
  {
    {
      plSimdQuat q;
      q.SetFromAxisAndAngle(plSimdVec4f(1, 0, 0), plAngle::Degree(90));

      PLASMA_TEST_BOOL((q * plSimdVec4f(0, 1, 0)).IsEqual(plSimdVec4f(0, 0, 1), 0.0001f).AllSet());
    }

    {
      plSimdQuat q;
      q.SetFromAxisAndAngle(plSimdVec4f(0, 1, 0), plAngle::Degree(90));

      PLASMA_TEST_BOOL((q * plSimdVec4f(1, 0, 0)).IsEqual(plSimdVec4f(0, 0, -1), 0.0001f).AllSet());
    }

    {
      plSimdQuat q;
      q.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(90));

      PLASMA_TEST_BOOL((q * plSimdVec4f(0, 1, 0)).IsEqual(plSimdVec4f(-1, 0, 0), 0.0001f).AllSet());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetShortestRotation / IsEqualRotation")
  {
    plSimdQuat q1, q2, q3;
    q1.SetShortestRotation(plSimdVec4f(0, 1, 0), plSimdVec4f(1, 0, 0));
    q2.SetFromAxisAndAngle(plSimdVec4f(0, 0, -1), plAngle::Degree(90));
    q3.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(-90));

    PLASMA_TEST_BOOL(q1.IsEqualRotation(q2, plMath::LargeEpsilon<float>()));
    PLASMA_TEST_BOOL(q1.IsEqualRotation(q3, plMath::LargeEpsilon<float>()));

    PLASMA_TEST_BOOL(plSimdQuat::IdentityQuaternion().IsEqualRotation(plSimdQuat::IdentityQuaternion(), plMath::LargeEpsilon<float>()));
    PLASMA_TEST_BOOL(plSimdQuat::IdentityQuaternion().IsEqualRotation(plSimdQuat(plSimdVec4f(0, 0, 0, -1)), plMath::LargeEpsilon<float>()));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetSlerp")
  {
    plSimdQuat q1, q2, q3, qr;
    q1.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(45));
    q2.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(0));
    q3.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(90));

    qr.SetSlerp(q2, q3, 0.5f);

    PLASMA_TEST_BOOL(q1.IsEqualRotation(qr, 0.0001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetRotationAxisAndAngle")
  {
    plSimdQuat q1, q2, q3;
    q1.SetShortestRotation(plSimdVec4f(0, 1, 0), plSimdVec4f(1, 0, 0));
    q2.SetFromAxisAndAngle(plSimdVec4f(0, 0, -1), plAngle::Degree(90));
    q3.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(-90));

    plSimdVec4f axis;
    plSimdFloat angle;

    PLASMA_TEST_BOOL(q1.GetRotationAxisAndAngle(axis, angle) == PLASMA_SUCCESS);
    PLASMA_TEST_BOOL(axis.IsEqual(plSimdVec4f(0, 0, -1), 0.001f).AllSet<3>());
    PLASMA_TEST_FLOAT(plAngle::RadToDeg((float)angle), 90, plMath::LargeEpsilon<float>());

    PLASMA_TEST_BOOL(q2.GetRotationAxisAndAngle(axis, angle) == PLASMA_SUCCESS);
    PLASMA_TEST_BOOL(axis.IsEqual(plSimdVec4f(0, 0, -1), 0.001f).AllSet<3>());
    PLASMA_TEST_FLOAT(plAngle::RadToDeg((float)angle), 90, plMath::LargeEpsilon<float>());

    PLASMA_TEST_BOOL(q3.GetRotationAxisAndAngle(axis, angle) == PLASMA_SUCCESS);
    PLASMA_TEST_BOOL(axis.IsEqual(plSimdVec4f(0, 0, -1), 0.001f).AllSet<3>());
    PLASMA_TEST_FLOAT(plAngle::RadToDeg((float)angle), 90, plMath::LargeEpsilon<float>());

    PLASMA_TEST_BOOL(plSimdQuat::IdentityQuaternion().GetRotationAxisAndAngle(axis, angle) == PLASMA_SUCCESS);
    PLASMA_TEST_BOOL(axis.IsEqual(plSimdVec4f(1, 0, 0), 0.001f).AllSet<3>());
    PLASMA_TEST_FLOAT(plAngle::RadToDeg((float)angle), 0, plMath::LargeEpsilon<float>());

    plSimdQuat otherIdentity(plSimdVec4f(0, 0, 0, -1));
    PLASMA_TEST_BOOL(otherIdentity.GetRotationAxisAndAngle(axis, angle) == PLASMA_SUCCESS);
    PLASMA_TEST_BOOL(axis.IsEqual(plSimdVec4f(1, 0, 0), 0.001f).AllSet<3>());
    PLASMA_TEST_FLOAT(plAngle::RadToDeg((float)angle), 360, plMath::LargeEpsilon<float>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsValid / Normalize")
  {
    plSimdQuat q(plSimdVec4f(1, 2, 3, 4));
    PLASMA_TEST_BOOL(!q.IsValid(0.001f));

    q.Normalize();
    PLASMA_TEST_BOOL(q.IsValid(0.001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator-")
  {
    plSimdQuat q, q1;
    q.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(90));
    q1.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(-90));

    plSimdQuat q2 = -q;
    PLASMA_TEST_BOOL(q1.IsEqualRotation(q2, 0.0001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator*(quat, quat)")
  {
    plSimdQuat q1, q2, qr, q3;
    q1.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(60));
    q2.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(30));
    q3.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(90));

    qr = q1 * q2;

    PLASMA_TEST_BOOL(qr.IsEqualRotation(q3, 0.0001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator==/!=")
  {
    plSimdQuat q1, q2;
    q1.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(60));
    q2.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(30));
    PLASMA_TEST_BOOL(q1 != q2);

    q2.SetFromAxisAndAngle(plSimdVec4f(1, 0, 0), plAngle::Degree(60));
    PLASMA_TEST_BOOL(q1 != q2);

    q2.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(60));
    PLASMA_TEST_BOOL(q1 == q2);
  }
}
