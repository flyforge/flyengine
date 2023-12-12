#include <CoreTest/CoreTestPCH.h>

#include <Core/World/CoordinateSystem.h>

void TestLength(const plCoordinateSystemConversion& atoB, const plCoordinateSystemConversion& btoA, float fSourceLength, float fTargetLength)
{
  PLASMA_TEST_FLOAT(atoB.ConvertSourceLength(fSourceLength), fTargetLength, plMath::DefaultEpsilon<float>());
  PLASMA_TEST_FLOAT(atoB.ConvertTargetLength(fTargetLength), fSourceLength, plMath::DefaultEpsilon<float>());

  PLASMA_TEST_FLOAT(btoA.ConvertTargetLength(fSourceLength), fTargetLength, plMath::DefaultEpsilon<float>());
  PLASMA_TEST_FLOAT(btoA.ConvertSourceLength(fTargetLength), fSourceLength, plMath::DefaultEpsilon<float>());
}

void TestPosition(
  const plCoordinateSystemConversion& atoB, const plCoordinateSystemConversion& btoA, const plVec3& vSourcePos, const plVec3& vTargetPos)
{
  TestLength(atoB, btoA, vSourcePos.GetLength(), vTargetPos.GetLength());

  PLASMA_TEST_VEC3(atoB.ConvertSourcePosition(vSourcePos), vTargetPos, plMath::DefaultEpsilon<float>());
  PLASMA_TEST_VEC3(atoB.ConvertTargetPosition(vTargetPos), vSourcePos, plMath::DefaultEpsilon<float>());
  PLASMA_TEST_VEC3(btoA.ConvertSourcePosition(vTargetPos), vSourcePos, plMath::DefaultEpsilon<float>());
  PLASMA_TEST_VEC3(btoA.ConvertTargetPosition(vSourcePos), vTargetPos, plMath::DefaultEpsilon<float>());
}

void TestRotation(const plCoordinateSystemConversion& atoB, const plCoordinateSystemConversion& btoA, const plVec3& vSourceStartDir,
  const plVec3& vSourceEndDir, const plQuat& qSourceRot, const plVec3& vTargetStartDir, const plVec3& vTargetEndDir, const plQuat& qTargetRot)
{
  TestPosition(atoB, btoA, vSourceStartDir, vTargetStartDir);
  TestPosition(atoB, btoA, vSourceEndDir, vTargetEndDir);

  PLASMA_TEST_BOOL(atoB.ConvertSourceRotation(qSourceRot).IsEqualRotation(qTargetRot, plMath::DefaultEpsilon<float>()));
  PLASMA_TEST_BOOL(atoB.ConvertTargetRotation(qTargetRot).IsEqualRotation(qSourceRot, plMath::DefaultEpsilon<float>()));
  PLASMA_TEST_BOOL(btoA.ConvertSourceRotation(qTargetRot).IsEqualRotation(qSourceRot, plMath::DefaultEpsilon<float>()));
  PLASMA_TEST_BOOL(btoA.ConvertTargetRotation(qSourceRot).IsEqualRotation(qTargetRot, plMath::DefaultEpsilon<float>()));

  PLASMA_TEST_VEC3(qSourceRot * vSourceStartDir, vSourceEndDir, plMath::DefaultEpsilon<float>());
  PLASMA_TEST_VEC3(qTargetRot * vTargetStartDir, vTargetEndDir, plMath::DefaultEpsilon<float>());
}

plQuat FromAxisAndAngle(const plVec3& vAxis, plAngle angle)
{
  plQuat q;
  q.SetFromAxisAndAngle(vAxis.GetNormalized(), angle);
  return q;
}

bool IsRightHanded(const plCoordinateSystem& cs)
{
  plVec3 vF = cs.m_vUpDir.CrossRH(cs.m_vRightDir);

  return vF.Dot(cs.m_vForwardDir) > 0;
}

void TestCoordinateSystemConversion(const plCoordinateSystem& a, const plCoordinateSystem& b)
{
  const bool bAisRH = IsRightHanded(a);
  const bool bBisRH = IsRightHanded(b);
  const plAngle A_CWRot = bAisRH ? plAngle::Degree(-90.0f) : plAngle::Degree(90.0f);
  const plAngle B_CWRot = bBisRH ? plAngle::Degree(-90.0f) : plAngle::Degree(90.0f);

  plCoordinateSystemConversion AtoB;
  AtoB.SetConversion(a, b);

  plCoordinateSystemConversion BtoA;
  BtoA.SetConversion(b, a);

  TestPosition(AtoB, BtoA, a.m_vForwardDir, b.m_vForwardDir);
  TestPosition(AtoB, BtoA, a.m_vRightDir, b.m_vRightDir);
  TestPosition(AtoB, BtoA, a.m_vUpDir, b.m_vUpDir);

  TestRotation(AtoB, BtoA, a.m_vForwardDir, a.m_vRightDir, FromAxisAndAngle(a.m_vUpDir, A_CWRot), b.m_vForwardDir, b.m_vRightDir,
    FromAxisAndAngle(b.m_vUpDir, B_CWRot));
  TestRotation(AtoB, BtoA, a.m_vUpDir, a.m_vForwardDir, FromAxisAndAngle(a.m_vRightDir, A_CWRot), b.m_vUpDir, b.m_vForwardDir,
    FromAxisAndAngle(b.m_vRightDir, B_CWRot));
  TestRotation(AtoB, BtoA, a.m_vUpDir, a.m_vRightDir, FromAxisAndAngle(a.m_vForwardDir, -A_CWRot), b.m_vUpDir, b.m_vRightDir,
    FromAxisAndAngle(b.m_vForwardDir, -B_CWRot));
}


PLASMA_CREATE_SIMPLE_TEST(World, CoordinateSystem)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PLASMA / OpenXR")
  {
    plCoordinateSystem plCoordSysLH;
    plCoordSysLH.m_vForwardDir = plVec3(1.0f, 0.0f, 0.0f);
    plCoordSysLH.m_vRightDir = plVec3(0.0f, 1.0f, 0.0f);
    plCoordSysLH.m_vUpDir = plVec3(0.0f, 0.0f, 1.0f);

    plCoordinateSystem openXrRH;
    openXrRH.m_vForwardDir = plVec3(0.0f, 0.0f, -1.0f);
    openXrRH.m_vRightDir = plVec3(1.0f, 0.0f, 0.0f);
    openXrRH.m_vUpDir = plVec3(0.0f, 1.0f, 0.0f);

    TestCoordinateSystemConversion(plCoordSysLH, openXrRH);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Scaled PLASMA / Scaled OpenXR")
  {
    plCoordinateSystem plCoordSysLH;
    plCoordSysLH.m_vForwardDir = plVec3(0.1f, 0.0f, 0.0f);
    plCoordSysLH.m_vRightDir = plVec3(0.0f, 0.1f, 0.0f);
    plCoordSysLH.m_vUpDir = plVec3(0.0f, 0.0f, 0.1f);

    plCoordinateSystem openXrRH;
    openXrRH.m_vForwardDir = plVec3(0.0f, 0.0f, -20.0f);
    openXrRH.m_vRightDir = plVec3(20.0f, 0.0f, 0.0f);
    openXrRH.m_vUpDir = plVec3(0.0f, 20.0f, 0.0f);

    TestCoordinateSystemConversion(plCoordSysLH, openXrRH);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PLASMA / Flipped PLASMA")
  {
    plCoordinateSystem plCoordSysLH;
    plCoordSysLH.m_vForwardDir = plVec3(1.0f, 0.0f, 0.0f);
    plCoordSysLH.m_vRightDir = plVec3(0.0f, 1.0f, 0.0f);
    plCoordSysLH.m_vUpDir = plVec3(0.0f, 0.0f, 1.0f);

    plCoordinateSystem plCoordSysFlippedLH;
    plCoordSysFlippedLH.m_vForwardDir = plVec3(-1.0f, 0.0f, 0.0f);
    plCoordSysFlippedLH.m_vRightDir = plVec3(0.0f, -1.0f, 0.0f);
    plCoordSysFlippedLH.m_vUpDir = plVec3(0.0f, 0.0f, 1.0f);

    TestCoordinateSystemConversion(plCoordSysLH, plCoordSysFlippedLH);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "OpenXR / Flipped OpenXR")
  {
    plCoordinateSystem openXrRH;
    openXrRH.m_vForwardDir = plVec3(0.0f, 0.0f, -1.0f);
    openXrRH.m_vRightDir = plVec3(1.0f, 0.0f, 0.0f);
    openXrRH.m_vUpDir = plVec3(0.0f, 1.0f, 0.0f);

    plCoordinateSystem openXrFlippedRH;
    openXrFlippedRH.m_vForwardDir = plVec3(0.0f, 0.0f, 1.0f);
    openXrFlippedRH.m_vRightDir = plVec3(-1.0f, 0.0f, 0.0f);
    openXrFlippedRH.m_vUpDir = plVec3(0.0f, 1.0f, 0.0f);

    TestCoordinateSystemConversion(openXrRH, openXrFlippedRH);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Identity")
  {
    plCoordinateSystem plCoordSysLH;
    plCoordSysLH.m_vForwardDir = plVec3(1.0f, 0.0f, 0.0f);
    plCoordSysLH.m_vRightDir = plVec3(0.0f, 1.0f, 0.0f);
    plCoordSysLH.m_vUpDir = plVec3(0.0f, 0.0f, 1.0f);

    TestCoordinateSystemConversion(plCoordSysLH, plCoordSysLH);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Default Constructed")
  {
    plCoordinateSystem plCoordSysLH;
    plCoordSysLH.m_vForwardDir = plVec3(1.0f, 0.0f, 0.0f);
    plCoordSysLH.m_vRightDir = plVec3(0.0f, 1.0f, 0.0f);
    plCoordSysLH.m_vUpDir = plVec3(0.0f, 0.0f, 1.0f);

    const plAngle rot = plAngle::Degree(90.0f);

    plCoordinateSystemConversion defaultConstucted;

    TestPosition(defaultConstucted, defaultConstucted, plCoordSysLH.m_vForwardDir, plCoordSysLH.m_vForwardDir);
    TestPosition(defaultConstucted, defaultConstucted, plCoordSysLH.m_vRightDir, plCoordSysLH.m_vRightDir);
    TestPosition(defaultConstucted, defaultConstucted, plCoordSysLH.m_vUpDir, plCoordSysLH.m_vUpDir);

    TestRotation(defaultConstucted, defaultConstucted, plCoordSysLH.m_vForwardDir, plCoordSysLH.m_vRightDir,
      FromAxisAndAngle(plCoordSysLH.m_vUpDir, rot), plCoordSysLH.m_vForwardDir, plCoordSysLH.m_vRightDir,
      FromAxisAndAngle(plCoordSysLH.m_vUpDir, rot));
    TestRotation(defaultConstucted, defaultConstucted, plCoordSysLH.m_vUpDir, plCoordSysLH.m_vForwardDir,
      FromAxisAndAngle(plCoordSysLH.m_vRightDir, rot), plCoordSysLH.m_vUpDir, plCoordSysLH.m_vForwardDir,
      FromAxisAndAngle(plCoordSysLH.m_vRightDir, rot));
    TestRotation(defaultConstucted, defaultConstucted, plCoordSysLH.m_vUpDir, plCoordSysLH.m_vRightDir,
      FromAxisAndAngle(plCoordSysLH.m_vForwardDir, -rot), plCoordSysLH.m_vUpDir, plCoordSysLH.m_vRightDir,
      FromAxisAndAngle(plCoordSysLH.m_vForwardDir, -rot));
  }
}
