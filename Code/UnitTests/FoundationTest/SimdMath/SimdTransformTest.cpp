#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdTransform.h>

PLASMA_CREATE_SIMPLE_TEST(SimdMath, SimdTransform)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
    plSimdTransform t0;

    {
      plSimdQuat qRot;
      qRot.SetFromAxisAndAngle(plSimdVec4f(1, 2, 3).GetNormalized<3>(), plAngle::Degree(42.0f));

      plSimdVec4f pos(4, 5, 6);
      plSimdVec4f scale(7, 8, 9);

      plSimdTransform t(pos);
      PLASMA_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      PLASMA_TEST_BOOL(t.m_Rotation == plSimdQuat::IdentityQuaternion());
      PLASMA_TEST_BOOL((t.m_Scale == plSimdVec4f(1)).AllSet<3>());

      t = plSimdTransform(pos, qRot);
      PLASMA_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      PLASMA_TEST_BOOL(t.m_Rotation == qRot);
      PLASMA_TEST_BOOL((t.m_Scale == plSimdVec4f(1)).AllSet<3>());

      t = plSimdTransform(pos, qRot, scale);
      PLASMA_TEST_BOOL((t.m_Position == pos).AllSet<3>());
      PLASMA_TEST_BOOL(t.m_Rotation == qRot);
      PLASMA_TEST_BOOL((t.m_Scale == scale).AllSet<3>());

      t = plSimdTransform(qRot);
      PLASMA_TEST_BOOL(t.m_Position.IsZero<3>());
      PLASMA_TEST_BOOL(t.m_Rotation == qRot);
      PLASMA_TEST_BOOL((t.m_Scale == plSimdVec4f(1)).AllSet<3>());
    }

    {
      plSimdTransform t;
      t.SetIdentity();

      PLASMA_TEST_BOOL(t.m_Position.IsZero<3>());
      PLASMA_TEST_BOOL(t.m_Rotation == plSimdQuat::IdentityQuaternion());
      PLASMA_TEST_BOOL((t.m_Scale == plSimdVec4f(1)).AllSet<3>());

      PLASMA_TEST_BOOL(t == plSimdTransform::IdentityTransform());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Inverse")
  {
    plSimdTransform tParent(plSimdVec4f(1, 2, 3));
    tParent.m_Rotation.SetFromAxisAndAngle(plSimdVec4f(0, 1, 0), plAngle::Degree(90));
    tParent.m_Scale = plSimdVec4f(2);

    plSimdTransform tToChild(plSimdVec4f(4, 5, 6));
    tToChild.m_Rotation.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(90));
    tToChild.m_Scale = plSimdVec4f(4);

    plSimdTransform tChild;
    tChild = tParent * tToChild;

    // invert twice -> get back original
    plSimdTransform t2 = tToChild;
    t2.Invert();
    t2.Invert();
    PLASMA_TEST_BOOL(t2.IsEqual(tToChild, 0.0001f));

    plSimdTransform tInvToChild = tToChild.GetInverse();

    plSimdTransform tParentFromChild;
    tParentFromChild = tChild * tInvToChild;

    PLASMA_TEST_BOOL(tParent.IsEqual(tParentFromChild, 0.0001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetLocalTransform")
  {
    plSimdQuat q;
    q.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(90));

    plSimdTransform tParent(plSimdVec4f(1, 2, 3));
    tParent.m_Rotation.SetFromAxisAndAngle(plSimdVec4f(0, 1, 0), plAngle::Degree(90));
    tParent.m_Scale = plSimdVec4f(2);

    plSimdTransform tChild;
    tChild.m_Position = plSimdVec4f(13, 12, -5);
    tChild.m_Rotation = tParent.m_Rotation * q;
    tChild.m_Scale = plSimdVec4f(8);

    plSimdTransform tToChild;
    tToChild.SetLocalTransform(tParent, tChild);

    PLASMA_TEST_BOOL(tToChild.m_Position.IsEqual(plSimdVec4f(4, 5, 6), 0.0001f).AllSet<3>());
    PLASMA_TEST_BOOL(tToChild.m_Rotation.IsEqualRotation(q, 0.0001f));
    PLASMA_TEST_BOOL((tToChild.m_Scale == plSimdVec4f(4)).AllSet<3>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetGlobalTransform")
  {
    plSimdTransform tParent(plSimdVec4f(1, 2, 3));
    tParent.m_Rotation.SetFromAxisAndAngle(plSimdVec4f(0, 1, 0), plAngle::Degree(90));
    tParent.m_Scale = plSimdVec4f(2);

    plSimdTransform tToChild(plSimdVec4f(4, 5, 6));
    tToChild.m_Rotation.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(90));
    tToChild.m_Scale = plSimdVec4f(4);

    plSimdTransform tChild;
    tChild.SetGlobalTransform(tParent, tToChild);

    PLASMA_TEST_BOOL(tChild.m_Position.IsEqual(plSimdVec4f(13, 12, -5), 0.0001f).AllSet<3>());
    PLASMA_TEST_BOOL(tChild.m_Rotation.IsEqualRotation(tParent.m_Rotation * tToChild.m_Rotation, 0.0001f));
    PLASMA_TEST_BOOL((tChild.m_Scale == plSimdVec4f(8)).AllSet<3>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetAsMat4")
  {
    plSimdTransform t(plSimdVec4f(1, 2, 3));
    t.m_Rotation.SetFromAxisAndAngle(plSimdVec4f(0, 1, 0), plAngle::Degree(34));
    t.m_Scale = plSimdVec4f(2, -1, 5);

    plSimdMat4f m = t.GetAsMat4();

    // reference
    plSimdMat4f refM;
    {
      plQuat q;
      q.SetFromAxisAndAngle(plVec3(0, 1, 0), plAngle::Degree(34));

      plTransform referenceTransform(plVec3(1, 2, 3), q, plVec3(2, -1, 5));
      plMat4 tmp = referenceTransform.GetAsMat4();
      refM.SetFromArray(tmp.m_fElementsCM, plMatrixLayout::ColumnMajor);
    }
    PLASMA_TEST_BOOL(m.IsEqual(refM, 0.00001f));

    plSimdVec4f p[8] = {plSimdVec4f(-4, 0, 0), plSimdVec4f(5, 0, 0), plSimdVec4f(0, -6, 0), plSimdVec4f(0, 7, 0), plSimdVec4f(0, 0, -8),
      plSimdVec4f(0, 0, 9), plSimdVec4f(1, -2, 3), plSimdVec4f(-4, 5, 7)};

    for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(p); ++i)
    {
      plSimdVec4f pt = t.TransformPosition(p[i]);
      plSimdVec4f pm = m.TransformPosition(p[i]);

      PLASMA_TEST_BOOL(pt.IsEqual(pm, 0.00001f).AllSet<3>());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "TransformPos / Dir / operator*")
  {
    plSimdQuat qRotX, qRotY;
    qRotX.SetFromAxisAndAngle(plSimdVec4f(1, 0, 0), plAngle::Degree(90.0f));
    qRotY.SetFromAxisAndAngle(plSimdVec4f(0, 1, 0), plAngle::Degree(90.0f));

    plSimdTransform t(plSimdVec4f(1, 2, 3, 10), qRotY * qRotX, plSimdVec4f(2, -2, 4, 11));

    plSimdVec4f v;
    v = t.TransformPosition(plSimdVec4f(4, 5, 6, 12));
    PLASMA_TEST_BOOL(v.IsEqual(plSimdVec4f((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), 0.0001f).AllSet<3>());

    v = t.TransformDirection(plSimdVec4f(4, 5, 6, 13));
    PLASMA_TEST_BOOL(v.IsEqual(plSimdVec4f((5 * -2), (-6 * 4), (-4 * 2)), 0.0001f).AllSet<3>());

    v = t * plSimdVec4f(4, 5, 6, 12);
    PLASMA_TEST_BOOL(v.IsEqual(plSimdVec4f((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3), 0.0001f).AllSet<3>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Operators")
  {
    {
      plSimdTransform tParent(plSimdVec4f(1, 2, 3));
      tParent.m_Rotation.SetFromAxisAndAngle(plSimdVec4f(0, 1, 0), plAngle::Degree(90));
      tParent.m_Scale = plSimdVec4f(2);

      plSimdTransform tToChild(plSimdVec4f(4, 5, 6));
      tToChild.m_Rotation.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(90));
      tToChild.m_Scale = plSimdVec4f(4);

      // this is exactly the same as SetGlobalTransform
      plSimdTransform tChild;
      tChild = tParent * tToChild;

      PLASMA_TEST_BOOL(tChild.m_Position.IsEqual(plSimdVec4f(13, 12, -5), 0.0001f).AllSet<3>());
      PLASMA_TEST_BOOL(tChild.m_Rotation.IsEqualRotation(tParent.m_Rotation * tToChild.m_Rotation, 0.0001f));
      PLASMA_TEST_BOOL((tChild.m_Scale == plSimdVec4f(8)).AllSet<3>());

      tChild = tParent;
      tChild *= tToChild;

      PLASMA_TEST_BOOL(tChild.m_Position.IsEqual(plSimdVec4f(13, 12, -5), 0.0001f).AllSet<3>());
      PLASMA_TEST_BOOL(tChild.m_Rotation.IsEqualRotation(tParent.m_Rotation * tToChild.m_Rotation, 0.0001f));
      PLASMA_TEST_BOOL((tChild.m_Scale == plSimdVec4f(8)).AllSet<3>());

      plSimdVec4f a(7, 8, 9);
      plSimdVec4f b;
      b = tToChild.TransformPosition(a);
      b = tParent.TransformPosition(b);

      plSimdVec4f c;
      c = tChild.TransformPosition(a);

      PLASMA_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());

      // verify that it works exactly like a 4x4 matrix
      /*const plMat4 mParent = tParent.GetAsMat4();
      const plMat4 mToChild = tToChild.GetAsMat4();
      const plMat4 mChild = mParent * mToChild;

      PLASMA_TEST_BOOL(mChild.IsEqual(tChild.GetAsMat4(), 0.0001f));*/
    }

    {
      plSimdTransform t(plSimdVec4f(1, 2, 3));
      t.m_Rotation.SetFromAxisAndAngle(plSimdVec4f(0, 1, 0), plAngle::Degree(90));
      t.m_Scale = plSimdVec4f(2);

      plSimdQuat q;
      q.SetFromAxisAndAngle(plSimdVec4f(0, 0, 1), plAngle::Degree(90));

      plSimdTransform t2 = t * q;
      plSimdTransform t4 = q * t;

      plSimdTransform t3 = t;
      t3 *= q;
      PLASMA_TEST_BOOL(t2 == t3);
      PLASMA_TEST_BOOL(t3 != t4);

      plSimdVec4f a(7, 8, 9);
      plSimdVec4f b;
      b = t2.TransformPosition(a);

      plSimdVec4f c = q * a;
      c = t.TransformPosition(c);

      PLASMA_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());
    }

    {
      plSimdTransform t(plSimdVec4f(1, 2, 3));
      t.m_Rotation.SetFromAxisAndAngle(plSimdVec4f(0, 1, 0), plAngle::Degree(90));
      t.m_Scale = plSimdVec4f(2);

      plSimdVec4f p(4, 5, 6);

      plSimdTransform t2 = t + p;
      plSimdTransform t3 = t;
      t3 += p;
      PLASMA_TEST_BOOL(t2 == t3);

      plSimdVec4f a(7, 8, 9);
      plSimdVec4f b;
      b = t2.TransformPosition(a);

      plSimdVec4f c = t.TransformPosition(a) + p;

      PLASMA_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());
    }

    {
      plSimdTransform t(plSimdVec4f(1, 2, 3));
      t.m_Rotation.SetFromAxisAndAngle(plSimdVec4f(0, 1, 0), plAngle::Degree(90));
      t.m_Scale = plSimdVec4f(2);

      plSimdVec4f p(4, 5, 6);

      plSimdTransform t2 = t - p;
      plSimdTransform t3 = t;
      t3 -= p;
      PLASMA_TEST_BOOL(t2 == t3);

      plSimdVec4f a(7, 8, 9);
      plSimdVec4f b;
      b = t2.TransformPosition(a);

      plSimdVec4f c = t.TransformPosition(a) - p;

      PLASMA_TEST_BOOL(b.IsEqual(c, 0.0001f).AllSet());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Comparison")
  {
    plSimdTransform t(plSimdVec4f(1, 2, 3));
    t.m_Rotation.SetFromAxisAndAngle(plSimdVec4f(0, 1, 0), plAngle::Degree(90));

    PLASMA_TEST_BOOL(t == t);

    plSimdTransform t2(plSimdVec4f(1, 2, 4));
    t2.m_Rotation.SetFromAxisAndAngle(plSimdVec4f(0, 1, 0), plAngle::Degree(90));

    PLASMA_TEST_BOOL(t != t2);

    plSimdTransform t3(plSimdVec4f(1, 2, 3));
    t3.m_Rotation.SetFromAxisAndAngle(plSimdVec4f(0, 1, 0), plAngle::Degree(91));

    PLASMA_TEST_BOOL(t != t3);
  }
}
