#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Transform.h>

PLASMA_CREATE_SIMPLE_TEST(Math, Transform)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructors")
  {
    plTransformT t0;

    {
      plTransformT t(plVec3T(1, 2, 3));
      PLASMA_TEST_VEC3(t.m_vPosition, plVec3T(1, 2, 3), 0);
    }

    {
      plQuat qRot;
      qRot.SetFromAxisAndAngle(plVec3T(1, 2, 3).GetNormalized(), plAngle::Degree(42.0f));

      plTransformT t(plVec3T(4, 5, 6), qRot);
      PLASMA_TEST_VEC3(t.m_vPosition, plVec3T(4, 5, 6), 0);
      PLASMA_TEST_BOOL(t.m_qRotation == qRot);
    }

    {
      plMat3 mRot;
      mRot.SetRotationMatrix(plVec3T(1, 2, 3).GetNormalized(), plAngle::Degree(42.0f));

      plQuat q;
      q.SetFromMat3(mRot);

      plTransformT t(plVec3T(4, 5, 6), q);
      PLASMA_TEST_VEC3(t.m_vPosition, plVec3T(4, 5, 6), 0);
      PLASMA_TEST_BOOL(t.m_qRotation.GetAsMat3().IsEqual(mRot, 0.0001f));
    }

    {
      plQuat qRot;
      qRot.SetIdentity();

      plTransformT t(plVec3T(4, 5, 6), qRot, plVec3T(2, 3, 4));
      PLASMA_TEST_VEC3(t.m_vPosition, plVec3T(4, 5, 6), 0);
      PLASMA_TEST_BOOL(t.m_qRotation.GetAsMat3().IsEqual(plMat3(1, 0, 0, 0, 1, 0, 0, 0, 1), 0.001f));
      PLASMA_TEST_VEC3(t.m_vScale, plVec3T(2, 3, 4), 0);
    }

    {
      plMat3T mRot;
      mRot.SetRotationMatrix(plVec3T(1, 2, 3).GetNormalized(), plAngle::Degree(42.0f));
      plMat4T mTrans;
      mTrans.SetTransformationMatrix(mRot, plVec3T(1, 2, 3));

      plTransformT t;
      t.SetFromMat4(mTrans);
      PLASMA_TEST_VEC3(t.m_vPosition, plVec3T(1, 2, 3), 0);
      PLASMA_TEST_BOOL(t.m_qRotation.GetAsMat3().IsEqual(mRot, 0.001f));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetIdentity")
  {
    plTransformT t;
    t.SetIdentity();

    PLASMA_TEST_VEC3(t.m_vPosition, plVec3T(0), 0);
    PLASMA_TEST_BOOL(t.m_qRotation == plQuat::IdentityQuaternion());
    PLASMA_TEST_BOOL(t.m_vScale == plVec3T(1.0f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetAsMat4")
  {
    plQuat qRot;
    qRot.SetIdentity();

    plTransformT t(plVec3T(4, 5, 6), qRot, plVec3T(2, 3, 4));
    PLASMA_TEST_BOOL(t.GetAsMat4() == plMat4(2, 0, 0, 4, 0, 3, 0, 5, 0, 0, 4, 6, 0, 0, 0, 1));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator + / -")
  {
    plTransformT t0, t1;
    t0.SetIdentity();
    t1.SetIdentity();

    t1 = t0 + plVec3T(2, 3, 4);
    PLASMA_TEST_VEC3(t1.m_vPosition, plVec3T(2, 3, 4), 0.0001f);

    t1 = t1 - plVec3T(4, 2, 1);
    PLASMA_TEST_VEC3(t1.m_vPosition, plVec3T(-2, 1, 3), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator * (quat)")
  {
    plQuat qRotX, qRotY;
    qRotX.SetFromAxisAndAngle(plVec3T(1, 0, 0), plAngle::Radian(1.57079637f));
    qRotY.SetFromAxisAndAngle(plVec3T(0, 1, 0), plAngle::Radian(1.57079637f));

    plTransformT t0, t1;
    t0.SetIdentity();
    t1.SetIdentity();

    t1 = qRotX * t0;
    PLASMA_TEST_VEC3(t1.m_vPosition, plVec3T(0, 0, 0), 0.0001f);

    plQuat q;
    q.SetFromMat3(plMat3(1, 0, 0, 0, 0, -1, 0, 1, 0));
    PLASMA_TEST_BOOL(t1.m_qRotation.IsEqualRotation(q, 0.0001f));

    t1 = qRotY * t1;
    PLASMA_TEST_VEC3(t1.m_vPosition, plVec3T(0, 0, 0), 0.0001f);
    q.SetFromMat3(plMat3(0, 1, 0, 0, 0, -1, -1, 0, 0));
    PLASMA_TEST_BOOL(t1.m_qRotation.IsEqualRotation(q, 0.0001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator * (vec3)")
  {
    plQuat qRotX, qRotY;
    qRotX.SetFromAxisAndAngle(plVec3T(1, 0, 0), plAngle::Radian(1.57079637f));
    qRotY.SetFromAxisAndAngle(plVec3T(0, 1, 0), plAngle::Radian(1.57079637f));

    plTransformT t;
    t.SetIdentity();

    t = qRotX * t;

    PLASMA_TEST_BOOL(t.m_qRotation.IsEqualRotation(qRotX, 0.0001f));
    PLASMA_TEST_VEC3(t.m_vPosition, plVec3T(0, 0, 0), 0.0001f);
    PLASMA_TEST_VEC3(t.m_vScale, plVec3T(1, 1, 1), 0.0001f);

    t = t + plVec3T(1, 2, 3);

    PLASMA_TEST_BOOL(t.m_qRotation.IsEqualRotation(qRotX, 0.0001f));
    PLASMA_TEST_VEC3(t.m_vPosition, plVec3T(1, 2, 3), 0.0001f);
    PLASMA_TEST_VEC3(t.m_vScale, plVec3T(1, 1, 1), 0.0001f);

    t = qRotY * t;

    PLASMA_TEST_BOOL(t.m_qRotation.IsEqualRotation(qRotY * qRotX, 0.0001f));
    PLASMA_TEST_VEC3(t.m_vPosition, plVec3T(1, 2, 3), 0.0001f);
    PLASMA_TEST_VEC3(t.m_vScale, plVec3T(1, 1, 1), 0.0001f);

    plQuat q;
    q.SetFromMat3(plMat3(0, 1, 0, 0, 0, -1, -1, 0, 0));
    PLASMA_TEST_BOOL(t.m_qRotation.IsEqualRotation(q, 0.0001f));

    plVec3T v;
    v = t * plVec3T(4, 5, 6);

    PLASMA_TEST_VEC3(v, plVec3T(5 + 1, -6 + 2, -4 + 3), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsIdentical")
  {
    plTransformT t(plVec3T(1, 2, 3));
    t.m_qRotation.SetFromAxisAndAngle(plVec3T(0, 1, 0), plAngle::Degree(90));

    PLASMA_TEST_BOOL(t.IsIdentical(t));

    plTransformT t2(plVec3T(1, 2, 4));
    t2.m_qRotation.SetFromAxisAndAngle(plVec3T(0, 1, 0), plAngle::Degree(90));

    PLASMA_TEST_BOOL(!t.IsIdentical(t2));

    plTransformT t3(plVec3T(1, 2, 3));
    t3.m_qRotation.SetFromAxisAndAngle(plVec3T(0, 1, 0), plAngle::Degree(91));

    PLASMA_TEST_BOOL(!t.IsIdentical(t3));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator == / !=")
  {
    plTransformT t(plVec3T(1, 2, 3));
    t.m_qRotation.SetFromAxisAndAngle(plVec3T(0, 1, 0), plAngle::Degree(90));

    PLASMA_TEST_BOOL(t == t);

    plTransformT t2(plVec3T(1, 2, 4));
    t2.m_qRotation.SetFromAxisAndAngle(plVec3T(0, 1, 0), plAngle::Degree(90));

    PLASMA_TEST_BOOL(t != t2);

    plTransformT t3(plVec3T(1, 2, 3));
    t3.m_qRotation.SetFromAxisAndAngle(plVec3T(0, 1, 0), plAngle::Degree(91));

    PLASMA_TEST_BOOL(t != t3);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEqual")
  {
    plTransformT t(plVec3T(1, 2, 3));
    t.m_qRotation.SetFromAxisAndAngle(plVec3T(0, 1, 0), plAngle::Degree(90));

    PLASMA_TEST_BOOL(t.IsEqual(t, 0.0001f));

    plTransformT t2(plVec3T(1, 2, 3.0002f));
    t2.m_qRotation.SetFromAxisAndAngle(plVec3T(0, 1, 0), plAngle::Degree(90));

    PLASMA_TEST_BOOL(t.IsEqual(t2, 0.001f));
    PLASMA_TEST_BOOL(!t.IsEqual(t2, 0.0001f));

    plTransformT t3(plVec3T(1, 2, 3));
    t3.m_qRotation.SetFromAxisAndAngle(plVec3T(0, 1, 0), plAngle::Degree(90.01f));

    PLASMA_TEST_BOOL(t.IsEqual(t3, 0.01f));
    PLASMA_TEST_BOOL(!t.IsEqual(t3, 0.0001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator*(plTransformT, plTransformT)")
  {
    plTransformT tParent(plVec3T(1, 2, 3));
    tParent.m_qRotation.SetFromAxisAndAngle(plVec3T(0, 1, 0), plAngle::Radian(1.57079637f));
    tParent.m_vScale.Set(2);

    plTransformT tToChild(plVec3T(4, 5, 6));
    tToChild.m_qRotation.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Radian(1.57079637f));
    tToChild.m_vScale.Set(4);

    // this is exactly the same as SetGlobalTransform
    plTransformT tChild;
    tChild = tParent * tToChild;

    PLASMA_TEST_VEC3(tChild.m_vPosition, plVec3T(13, 12, -5), 0.003f);
    PLASMA_TEST_BOOL(tChild.m_qRotation.GetAsMat3().IsEqual(plMat3(0, 0, 1, 1, 0, 0, 0, 1, 0), 0.0001f));
    PLASMA_TEST_VEC3(tChild.m_vScale, plVec3T(8, 8, 8), 0.0001f);

    // verify that it works exactly like a 4x4 matrix
    const plMat4 mParent = tParent.GetAsMat4();
    const plMat4 mToChild = tToChild.GetAsMat4();
    const plMat4 mChild = mParent * mToChild;

    PLASMA_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), 0.0001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator*(plTransformT, plMat4)")
  {
    plTransformT tParent(plVec3T(1, 2, 3));
    tParent.m_qRotation.SetFromAxisAndAngle(plVec3T(0, 1, 0), plAngle::Degree(90));
    tParent.m_vScale.Set(2);

    plTransformT tToChild(plVec3T(4, 5, 6));
    tToChild.m_qRotation.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(90));
    tToChild.m_vScale.Set(4);

    // this is exactly the same as SetGlobalTransform
    plTransformT tChild;
    tChild = tParent * tToChild;

    PLASMA_TEST_VEC3(tChild.m_vPosition, plVec3T(13, 12, -5), 0.0001f);
    PLASMA_TEST_BOOL(tChild.m_qRotation.GetAsMat3().IsEqual(plMat3(0, 0, 1, 1, 0, 0, 0, 1, 0), 0.0001f));
    PLASMA_TEST_VEC3(tChild.m_vScale, plVec3T(8, 8, 8), 0.0001f);

    // verify that it works exactly like a 4x4 matrix
    const plMat4 mParent = tParent.GetAsMat4();
    const plMat4 mToChild = tToChild.GetAsMat4();
    const plMat4 mChild = mParent * mToChild;

    PLASMA_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), 0.0001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator*(plMat4, plTransformT)")
  {
    plTransformT tParent(plVec3T(1, 2, 3));
    tParent.m_qRotation.SetFromAxisAndAngle(plVec3T(0, 1, 0), plAngle::Degree(90));
    tParent.m_vScale.Set(2);

    plTransformT tToChild(plVec3T(4, 5, 6));
    tToChild.m_qRotation.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(90));
    tToChild.m_vScale.Set(4);

    // this is exactly the same as SetGlobalTransform
    plTransformT tChild;
    tChild = tParent * tToChild;

    PLASMA_TEST_VEC3(tChild.m_vPosition, plVec3T(13, 12, -5), 0.0001f);
    PLASMA_TEST_BOOL(tChild.m_qRotation.GetAsMat3().IsEqual(plMat3(0, 0, 1, 1, 0, 0, 0, 1, 0), 0.0001f));
    PLASMA_TEST_VEC3(tChild.m_vScale, plVec3T(8, 8, 8), 0.0001f);

    // verify that it works exactly like a 4x4 matrix
    const plMat4 mParent = tParent.GetAsMat4();
    const plMat4 mToChild = tToChild.GetAsMat4();
    const plMat4 mChild = mParent * mToChild;

    PLASMA_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), 0.0001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Invert / GetInverse")
  {
    plTransformT tParent(plVec3T(1, 2, 3));
    tParent.m_qRotation.SetFromAxisAndAngle(plVec3T(0, 1, 0), plAngle::Degree(90));
    tParent.m_vScale.Set(2);

    plTransformT tToChild(plVec3T(4, 5, 6));
    tToChild.m_qRotation.SetFromAxisAndAngle(plVec3T(0, 0, 1), plAngle::Degree(90));
    tToChild.m_vScale.Set(4);

    plTransformT tChild;
    tChild.SetGlobalTransform(tParent, tToChild);

    // negate twice -> get back original
    tToChild.Invert();
    tToChild.Invert();

    plTransformT tInvToChild = tToChild.GetInverse();

    plTransformT tParentFromChild;
    tParentFromChild.SetGlobalTransform(tChild, tInvToChild);

    PLASMA_TEST_BOOL(tParent.IsEqual(tParentFromChild, 0.0001f));
  }

  //////////////////////////////////////////////////////////////////////////
  // Tests copied and ported over from plSimdTransform
  //////////////////////////////////////////////////////////////////////////

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
    plTransform t0;

    {
      plQuat qRot;
      qRot.SetFromAxisAndAngle(plVec3(1, 2, 3).GetNormalized(), plAngle::Degree(42.0f));

      plVec3 pos(4, 5, 6);
      plVec3 scale(7, 8, 9);

      plTransform t(pos);
      PLASMA_TEST_BOOL((t.m_vPosition == pos));
      PLASMA_TEST_BOOL(t.m_qRotation == plQuat::IdentityQuaternion());
      PLASMA_TEST_BOOL((t.m_vScale == plVec3(1)));

      t = plTransform(pos, qRot);
      PLASMA_TEST_BOOL((t.m_vPosition == pos));
      PLASMA_TEST_BOOL(t.m_qRotation == qRot);
      PLASMA_TEST_BOOL((t.m_vScale == plVec3(1)));

      t = plTransform(pos, qRot, scale);
      PLASMA_TEST_BOOL((t.m_vPosition == pos));
      PLASMA_TEST_BOOL(t.m_qRotation == qRot);
      PLASMA_TEST_BOOL((t.m_vScale == scale));

      t = plTransform(plVec3::ZeroVector(), qRot);
      PLASMA_TEST_BOOL(t.m_vPosition.IsZero());
      PLASMA_TEST_BOOL(t.m_qRotation == qRot);
      PLASMA_TEST_BOOL((t.m_vScale == plVec3(1)));
    }

    {
      plTransform t;
      t.SetIdentity();

      PLASMA_TEST_BOOL(t.m_vPosition.IsZero());
      PLASMA_TEST_BOOL(t.m_qRotation == plQuat::IdentityQuaternion());
      PLASMA_TEST_BOOL((t.m_vScale == plVec3(1)));

      PLASMA_TEST_BOOL(t == plTransform::IdentityTransform());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Inverse")
  {
    plTransform tParent(plVec3(1, 2, 3));
    tParent.m_qRotation.SetFromAxisAndAngle(plVec3(0, 1, 0), plAngle::Degree(90));
    tParent.m_vScale = plVec3(2);

    plTransform tToChild(plVec3(4, 5, 6));
    tToChild.m_qRotation.SetFromAxisAndAngle(plVec3(0, 0, 1), plAngle::Degree(90));
    tToChild.m_vScale = plVec3(4);

    plTransform tChild;
    tChild = tParent * tToChild;

    // invert twice -> get back original
    plTransform t2 = tToChild;
    t2.Invert();
    t2.Invert();
    PLASMA_TEST_BOOL(t2.IsEqual(tToChild, 0.0001f));

    plTransform tInvToChild = tToChild.GetInverse();

    plTransform tParentFromChild;
    tParentFromChild = tChild * tInvToChild;

    PLASMA_TEST_BOOL(tParent.IsEqual(tParentFromChild, 0.0001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetLocalTransform")
  {
    plQuat q;
    q.SetFromAxisAndAngle(plVec3(0, 0, 1), plAngle::Degree(90));

    plTransform tParent(plVec3(1, 2, 3));
    tParent.m_qRotation.SetFromAxisAndAngle(plVec3(0, 1, 0), plAngle::Degree(90));
    tParent.m_vScale = plVec3(2);

    plTransform tChild;
    tChild.m_vPosition = plVec3(13, 12, -5);
    tChild.m_qRotation = tParent.m_qRotation * q;
    tChild.m_vScale = plVec3(8);

    plTransform tToChild;
    tToChild.SetLocalTransform(tParent, tChild);

    PLASMA_TEST_BOOL(tToChild.m_vPosition.IsEqual(plVec3(4, 5, 6), 0.0001f));
    PLASMA_TEST_BOOL(tToChild.m_qRotation.IsEqualRotation(q, 0.0001f));
    PLASMA_TEST_BOOL((tToChild.m_vScale == plVec3(4)));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetGlobalTransform")
  {
    plTransform tParent(plVec3(1, 2, 3));
    tParent.m_qRotation.SetFromAxisAndAngle(plVec3(0, 1, 0), plAngle::Degree(90));
    tParent.m_vScale = plVec3(2);

    plTransform tToChild(plVec3(4, 5, 6));
    tToChild.m_qRotation.SetFromAxisAndAngle(plVec3(0, 0, 1), plAngle::Degree(90));
    tToChild.m_vScale = plVec3(4);

    plTransform tChild;
    tChild.SetGlobalTransform(tParent, tToChild);

    PLASMA_TEST_BOOL(tChild.m_vPosition.IsEqual(plVec3(13, 12, -5), 0.0001f));
    PLASMA_TEST_BOOL(tChild.m_qRotation.IsEqualRotation(tParent.m_qRotation * tToChild.m_qRotation, 0.0001f));
    PLASMA_TEST_BOOL((tChild.m_vScale == plVec3(8)));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetAsMat4")
  {
    plTransform t(plVec3(1, 2, 3));
    t.m_qRotation.SetFromAxisAndAngle(plVec3(0, 1, 0), plAngle::Degree(34));
    t.m_vScale = plVec3(2, -1, 5);

    plMat4 m = t.GetAsMat4();

    plMat4 refM;
    refM.SetZero();
    {
      plQuat q;
      q.SetFromAxisAndAngle(plVec3(0, 1, 0), plAngle::Degree(34));

      plTransform referenceTransform(plVec3(1, 2, 3), q, plVec3(2, -1, 5));
      plMat4 tmp = referenceTransform.GetAsMat4();
      refM.SetFromArray(tmp.m_fElementsCM, plMatrixLayout::ColumnMajor);
    }
    PLASMA_TEST_BOOL(m.IsEqual(refM, 0.00001f));

    plVec3 p[8] = {
      plVec3(-4, 0, 0), plVec3(5, 0, 0), plVec3(0, -6, 0), plVec3(0, 7, 0), plVec3(0, 0, -8), plVec3(0, 0, 9), plVec3(1, -2, 3), plVec3(-4, 5, 7)};

    for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(p); ++i)
    {
      plVec3 pt = t.TransformPosition(p[i]);
      plVec3 pm = m.TransformPosition(p[i]);

      PLASMA_TEST_BOOL(pt.IsEqual(pm, 0.00001f));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "TransformPos / Dir / operator*")
  {
    plQuat qRotX, qRotY;
    qRotX.SetFromAxisAndAngle(plVec3(1, 0, 0), plAngle::Degree(90.0f));
    qRotY.SetFromAxisAndAngle(plVec3(0, 1, 0), plAngle::Degree(90.0f));

    plTransform t(plVec3(1, 2, 3), qRotY * qRotX, plVec3(2, -2, 4));

    plVec3 v;
    v = t.TransformPosition(plVec3(4, 5, 6));
    PLASMA_TEST_BOOL(v.IsEqual(plVec3((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), 0.0001f));

    v = t.TransformDirection(plVec3(4, 5, 6));
    PLASMA_TEST_BOOL(v.IsEqual(plVec3((5 * -2), (-6 * 4), (-4 * 2)), 0.0001f));

    v = t * plVec3(4, 5, 6);
    PLASMA_TEST_BOOL(v.IsEqual(plVec3((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), 0.0001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Operators")
  {
    {
      plTransform tParent(plVec3(1, 2, 3));
      tParent.m_qRotation.SetFromAxisAndAngle(plVec3(0, 1, 0), plAngle::Degree(90));
      tParent.m_vScale = plVec3(2);

      plTransform tToChild(plVec3(4, 5, 6));
      tToChild.m_qRotation.SetFromAxisAndAngle(plVec3(0, 0, 1), plAngle::Degree(90));
      tToChild.m_vScale = plVec3(4);

      // this is exactly the same as SetGlobalTransform
      plTransform tChild;
      tChild = tParent * tToChild;

      PLASMA_TEST_BOOL(tChild.m_vPosition.IsEqual(plVec3(13, 12, -5), 0.0001f));
      PLASMA_TEST_BOOL(tChild.m_qRotation.IsEqualRotation(tParent.m_qRotation * tToChild.m_qRotation, 0.0001f));
      PLASMA_TEST_BOOL((tChild.m_vScale == plVec3(8)));

      tChild = tParent;
      tChild = tChild * tToChild;

      PLASMA_TEST_BOOL(tChild.m_vPosition.IsEqual(plVec3(13, 12, -5), 0.0001f));
      PLASMA_TEST_BOOL(tChild.m_qRotation.IsEqualRotation(tParent.m_qRotation * tToChild.m_qRotation, 0.0001f));
      PLASMA_TEST_BOOL((tChild.m_vScale == plVec3(8)));

      plVec3 a(7, 8, 9);
      plVec3 b;
      b = tToChild.TransformPosition(a);
      b = tParent.TransformPosition(b);

      plVec3 c;
      c = tChild.TransformPosition(a);

      PLASMA_TEST_BOOL(b.IsEqual(c, 0.0001f));

      // verify that it works exactly like a 4x4 matrix
      const plMat4 mParent = tParent.GetAsMat4();
      const plMat4 mToChild = tToChild.GetAsMat4();
      const plMat4 mChild = mParent * mToChild;

      PLASMA_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), 0.0001f));
    }

    {
      plTransform t(plVec3(1, 2, 3));
      t.m_qRotation.SetFromAxisAndAngle(plVec3(0, 1, 0), plAngle::Degree(90));
      t.m_vScale = plVec3(2);

      plQuat q;
      q.SetFromAxisAndAngle(plVec3(0, 0, 1), plAngle::Degree(90));

      plTransform t2 = t * q;
      plTransform t4 = q * t;

      plTransform t3 = t;
      t3 = t3 * q;
      PLASMA_TEST_BOOL(t2 == t3);
      PLASMA_TEST_BOOL(t3 != t4);

      plVec3 a(7, 8, 9);
      plVec3 b;
      b = t2.TransformPosition(a);

      plVec3 c = q * a;
      c = t.TransformPosition(c);

      PLASMA_TEST_BOOL(b.IsEqual(c, 0.0001f));
    }

    {
      plTransform t(plVec3(1, 2, 3));
      t.m_qRotation.SetFromAxisAndAngle(plVec3(0, 1, 0), plAngle::Degree(90));
      t.m_vScale = plVec3(2);

      plVec3 p(4, 5, 6);

      plTransform t2 = t + p;
      plTransform t3 = t;
      t3 += p;
      PLASMA_TEST_BOOL(t2 == t3);

      plVec3 a(7, 8, 9);
      plVec3 b;
      b = t2.TransformPosition(a);

      plVec3 c = t.TransformPosition(a) + p;

      PLASMA_TEST_BOOL(b.IsEqual(c, 0.0001f));
    }

    {
      plTransform t(plVec3(1, 2, 3));
      t.m_qRotation.SetFromAxisAndAngle(plVec3(0, 1, 0), plAngle::Degree(90));
      t.m_vScale = plVec3(2);

      plVec3 p(4, 5, 6);

      plTransform t2 = t - p;
      plTransform t3 = t;
      t3 -= p;
      PLASMA_TEST_BOOL(t2 == t3);

      plVec3 a(7, 8, 9);
      plVec3 b;
      b = t2.TransformPosition(a);

      plVec3 c = t.TransformPosition(a) - p;

      PLASMA_TEST_BOOL(b.IsEqual(c, 0.0001f));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Comparison")
  {
    plTransform t(plVec3(1, 2, 3));
    t.m_qRotation.SetFromAxisAndAngle(plVec3(0, 1, 0), plAngle::Degree(90));

    PLASMA_TEST_BOOL(t == t);

    plTransform t2(plVec3(1, 2, 4));
    t2.m_qRotation.SetFromAxisAndAngle(plVec3(0, 1, 0), plAngle::Degree(90));

    PLASMA_TEST_BOOL(t != t2);

    plTransform t3(plVec3(1, 2, 3));
    t3.m_qRotation.SetFromAxisAndAngle(plVec3(0, 1, 0), plAngle::Degree(91));

    PLASMA_TEST_BOOL(t != t3);
  }
}
