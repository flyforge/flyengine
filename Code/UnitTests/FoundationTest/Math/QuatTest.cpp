#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Quat.h>

PLASMA_CREATE_SIMPLE_TEST(Math, Quaternion)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Default Constructor")
  {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    if (plMath::SupportsNaN<plMat3T::ComponentType>())
    {
      // In debug the default constructor initializes everything with NaN.
      plQuatT p;
      PLASMA_TEST_BOOL(plMath::IsNaN(p.v.x) && plMath::IsNaN(p.v.y) && plMath::IsNaN(p.v.z) && plMath::IsNaN(p.w));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    plQuatT::ComponentType testBlock[4] = {
      (plQuatT::ComponentType)1, (plQuatT::ComponentType)2, (plQuatT::ComponentType)3, (plQuatT::ComponentType)4};
    plQuatT* p = ::new ((void*)&testBlock[0]) plQuatT;
    PLASMA_TEST_BOOL(p->v.x == (plMat3T::ComponentType)1 && p->v.y == (plMat3T::ComponentType)2 && p->v.z == (plMat3T::ComponentType)3 &&
                 p->w == (plMat3T::ComponentType)4);
#endif
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor(x,y,z,w)")
  {
    plQuatT q(1, 2, 3, 4);

    PLASMA_TEST_VEC3(q.v, plVec3T(1, 2, 3), 0.0001f);
    PLASMA_TEST_FLOAT(q.w, 4, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IdentityQuaternion")
  {
    plQuatT q = plQuatT::IdentityQuaternion();

    PLASMA_TEST_VEC3(q.v, plVec3T(0, 0, 0), 0.0001f);
    PLASMA_TEST_FLOAT(q.w, 1, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetIdentity")
  {
    plQuatT q(1, 2, 3, 4);

    q.SetIdentity();

    PLASMA_TEST_VEC3(q.v, plVec3T(0, 0, 0), 0.0001f);
    PLASMA_TEST_FLOAT(q.w, 1, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetElements")
  {
    plQuatT q(5, 6, 7, 8);

    q.SetElements(1, 2, 3, 4);

    PLASMA_TEST_VEC3(q.v, plVec3T(1, 2, 3), 0.0001f);
    PLASMA_TEST_FLOAT(q.w, 4, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetFromAxisAndAngle / operator* (quat, vec)")
  {
    {
      plQuatT q;
      q.SetFromAxisAndAngle(plVec3T(1, 0, 0), plAngle::Degree(90));

      PLASMA_TEST_VEC3(q * plVec3T(0, 1, 0), plVec3T(0, 0, 1), 0.0001f);
    }

    {
      plQuatT q;
      q.SetFromAxisAndAngle(plVec3T(0, 1, 0), plAngle::Degree(90));

      PLASMA_TEST_VEC3(q * plVec3T(1, 0, 0), plVec3T(0, 0, -1), 0.0001f);
    }

    {
      plQuatT q;
      q.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(90));

      PLASMA_TEST_VEC3(q * plVec3T(0, 1, 0), plVec3T(-1, 0, 0), 0.0001f);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetShortestRotation / IsEqualRotation")
  {
    plQuatT q1, q2, q3;
    q1.SetShortestRotation(plVec3T(0, 1, 0), plVec3T(1, 0, 0));
    q2.SetFromAxisAndAngle(plVec3T(0, 0, -1), plAngle::Degree(90));
    q3.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(-90));

    PLASMA_TEST_BOOL(q1.IsEqualRotation(q2, plMath::LargeEpsilon<float>()));
    PLASMA_TEST_BOOL(q1.IsEqualRotation(q3, plMath::LargeEpsilon<float>()));

    PLASMA_TEST_BOOL(plQuatT::IdentityQuaternion().IsEqualRotation(plQuatT::IdentityQuaternion(), plMath::LargeEpsilon<float>()));
    PLASMA_TEST_BOOL(plQuatT::IdentityQuaternion().IsEqualRotation(plQuatT(0, 0, 0, -1), plMath::LargeEpsilon<float>()));

    plQuatT q4{0, 0, 0, 1.00000012f};
    plQuatT q5{0, 0, 0, 1.00000023f};
    PLASMA_TEST_BOOL(q4.IsEqualRotation(q5, plMath::LargeEpsilon<float>()));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetFromMat3")
  {
    plMat3T m;
    m.SetRotationMatrixZ(plAngle::Degree(-90));

    plQuatT q1, q2, q3;
    q1.SetFromMat3(m);
    q2.SetFromAxisAndAngle(plVec3T(0, 0, -1), plAngle::Degree(90));
    q3.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(-90));

    PLASMA_TEST_BOOL(q1.IsEqualRotation(q2, plMath::LargeEpsilon<float>()));
    PLASMA_TEST_BOOL(q1.IsEqualRotation(q3, plMath::LargeEpsilon<float>()));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetSlerp")
  {
    plQuatT q1, q2, q3, qr;
    q1.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(45));
    q2.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(0));
    q3.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(90));

    qr.SetSlerp(q2, q3, 0.5f);

    PLASMA_TEST_BOOL(q1.IsEqualRotation(qr, 0.0001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetRotationAxisAndAngle")
  {
    plQuatT q1, q2, q3;
    q1.SetShortestRotation(plVec3T(0, 1, 0), plVec3T(1, 0, 0));
    q2.SetFromAxisAndAngle(plVec3T(0, 0, -1), plAngle::Degree(90));
    q3.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(-90));

    plVec3T axis;
    plAngle angle;

    PLASMA_TEST_VEC3(axis, plVec3T(0, 0, -1), 0.001f);
    PLASMA_TEST_FLOAT(angle.GetDegree(), 90, plMath::LargeEpsilon<plMat3T::ComponentType>());

    PLASMA_TEST_VEC3(axis, plVec3T(0, 0, -1), 0.001f);
    PLASMA_TEST_FLOAT(angle.GetDegree(), 90, plMath::LargeEpsilon<plMat3T::ComponentType>());

    PLASMA_TEST_VEC3(axis, plVec3T(0, 0, -1), 0.001f);
    PLASMA_TEST_FLOAT(angle.GetDegree(), 90, plMath::LargeEpsilon<plMat3T::ComponentType>());

    PLASMA_TEST_VEC3(axis, plVec3T(1, 0, 0), 0.001f);
    PLASMA_TEST_FLOAT(angle.GetDegree(), 0, plMath::LargeEpsilon<plMat3T::ComponentType>());

    plQuatT otherIdentity(0, 0, 0, -1);
    PLASMA_TEST_VEC3(axis, plVec3T(1, 0, 0), 0.001f);
    PLASMA_TEST_FLOAT(angle.GetDegree(), 360, plMath::LargeEpsilon<plMat3T::ComponentType>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetAsMat3")
  {
    plQuatT q;
    q.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(90));

    plMat3T mr;
    mr.SetRotationMatrixZ(plAngle::Degree(90));

    plMat3T m = q.GetAsMat3();

    PLASMA_TEST_BOOL(mr.IsEqual(m, plMath::DefaultEpsilon<plMat3T::ComponentType>()));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetAsMat4")
  {
    plQuatT q;
    q.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(90));

    plMat4T mr;
    mr.SetRotationMatrixZ(plAngle::Degree(90));

    plMat4T m = q.GetAsMat4();

    PLASMA_TEST_BOOL(mr.IsEqual(m, plMath::DefaultEpsilon<plMat3T::ComponentType>()));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsValid / Normalize")
  {
    plQuatT q(1, 2, 3, 4);
    PLASMA_TEST_BOOL(!q.IsValid(0.001f));

    q.Normalize();
    PLASMA_TEST_BOOL(q.IsValid(0.001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator-")
  {
    plQuatT q, q1;
    q.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(90));
    q1.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(-90));

    plQuatT q2 = -q;
    PLASMA_TEST_BOOL(q1.IsEqualRotation(q2, 0.0001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Dot")
  {
    plQuatT q, q1, q2;
    q.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(90));
    q1.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(-90));
    q2.SetFromAxisAndAngle(plVec3T(0, 1, 0), plAngle::Degree(45));

    PLASMA_TEST_FLOAT(q.Dot(q), 1.0f, 0.0001f);
    PLASMA_TEST_FLOAT(q.Dot(plQuat::IdentityQuaternion()), cos(plAngle::DegToRad(90.0f / 2)), 0.0001f);
    PLASMA_TEST_FLOAT(q.Dot(q1), 0.0f, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator*(quat, quat)")
  {
    plQuatT q1, q2, qr, q3;
    q1.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(60));
    q2.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(30));
    q3.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(90));

    qr = q1 * q2;

    PLASMA_TEST_BOOL(qr.IsEqualRotation(q3, 0.0001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator==/!=")
  {
    plQuatT q1, q2;
    q1.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(60));
    q2.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(30));
    PLASMA_TEST_BOOL(q1 != q2);

    q2.SetFromAxisAndAngle(plVec3T(1, 0, 0), plAngle::Degree(60));
    PLASMA_TEST_BOOL(q1 != q2);

    q2.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(60));
    PLASMA_TEST_BOOL(q1 == q2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsNaN")
  {
    if (plMath::SupportsNaN<plMathTestType>())
    {
      plQuatT q;

      q.SetIdentity();
      PLASMA_TEST_BOOL(!q.IsNaN());

      q.SetIdentity();
      q.w = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(q.IsNaN());

      q.SetIdentity();
      q.v.x = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(q.IsNaN());

      q.SetIdentity();
      q.v.y = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(q.IsNaN());

      q.SetIdentity();
      q.v.z = plMath::NaN<plMathTestType>();
      PLASMA_TEST_BOOL(q.IsNaN());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "rotation direction")
  {
    plMat3T m;
    m.SetRotationMatrixZ(plAngle::Degree(90.0f));

    plQuatT q;
    q.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(90.0f));

    plVec3T xAxis(1, 0, 0);

    plVec3T temp1 = m.TransformDirection(xAxis);
    plVec3T temp2 = q.GetAsMat3().TransformDirection(xAxis);

    PLASMA_TEST_BOOL(temp1.IsEqual(temp2, 0.01f));
  }
}
