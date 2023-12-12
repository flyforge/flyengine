#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Implementation/AllClasses_inl.h>
#include <Foundation/Math/Mat3.h>

PLASMA_CREATE_SIMPLE_TEST(Math, Mat3)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Default Constructor")
  {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    if (plMath::SupportsNaN<plMathTestType>())
    {
      // In debug the default constructor initializes everything with NaN.
      plMat3T m;
      PLASMA_TEST_BOOL(plMath::IsNaN(m.m_fElementsCM[0]) && plMath::IsNaN(m.m_fElementsCM[1]) && plMath::IsNaN(m.m_fElementsCM[2]) &&
                   plMath::IsNaN(m.m_fElementsCM[3]) && plMath::IsNaN(m.m_fElementsCM[4]) && plMath::IsNaN(m.m_fElementsCM[5]) &&
                   plMath::IsNaN(m.m_fElementsCM[6]) && plMath::IsNaN(m.m_fElementsCM[7]) && plMath::IsNaN(m.m_fElementsCM[8]));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    plMat3T::ComponentType testBlock[9] = {(plMat3T::ComponentType)1, (plMat3T::ComponentType)2, (plMat3T::ComponentType)3, (plMat3T::ComponentType)4,
      (plMat3T::ComponentType)5, (plMat3T::ComponentType)6, (plMat3T::ComponentType)7, (plMat3T::ComponentType)8, (plMat3T::ComponentType)9};

    plMat3T* m = ::new ((void*)&testBlock[0]) plMat3T;

    PLASMA_TEST_BOOL(m->m_fElementsCM[0] == (plMat3T::ComponentType)1 && m->m_fElementsCM[1] == (plMat3T::ComponentType)2 &&
                 m->m_fElementsCM[2] == (plMat3T::ComponentType)3 && m->m_fElementsCM[3] == (plMat3T::ComponentType)4 &&
                 m->m_fElementsCM[4] == (plMat3T::ComponentType)5 && m->m_fElementsCM[5] == (plMat3T::ComponentType)6 &&
                 m->m_fElementsCM[6] == (plMat3T::ComponentType)7 && m->m_fElementsCM[7] == (plMat3T::ComponentType)8 &&
                 m->m_fElementsCM[8] == (plMat3T::ComponentType)9);
#endif
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor (Array Data)")
  {
    const plMathTestType data[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    {
      plMat3T m(data, plMatrixLayout::ColumnMajor);

      PLASMA_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && m.m_fElementsCM[1] == 2.0f && m.m_fElementsCM[2] == 3.0f && m.m_fElementsCM[3] == 4.0f &&
                   m.m_fElementsCM[4] == 5.0f && m.m_fElementsCM[5] == 6.0f && m.m_fElementsCM[6] == 7.0f && m.m_fElementsCM[7] == 8.0f &&
                   m.m_fElementsCM[8] == 9.0f);
    }

    {
      plMat3T m(data, plMatrixLayout::RowMajor);

      PLASMA_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && m.m_fElementsCM[1] == 4.0f && m.m_fElementsCM[2] == 7.0f && m.m_fElementsCM[3] == 2.0f &&
                   m.m_fElementsCM[4] == 5.0f && m.m_fElementsCM[5] == 8.0f && m.m_fElementsCM[6] == 3.0f && m.m_fElementsCM[7] == 6.0f &&
                   m.m_fElementsCM[8] == 9.0f);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor (Elements)")
  {
    plMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    PLASMA_TEST_FLOAT(m.Element(0, 0), 1, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(1, 0), 2, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(2, 0), 3, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(0, 1), 4, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(1, 1), 5, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(2, 1), 6, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(0, 2), 7, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(1, 2), 8, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(2, 2), 9, 0.00001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetFromArray")
  {
    const plMathTestType data[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    {
      plMat3T m;
      m.SetFromArray(data, plMatrixLayout::ColumnMajor);

      PLASMA_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && m.m_fElementsCM[1] == 2.0f && m.m_fElementsCM[2] == 3.0f && m.m_fElementsCM[3] == 4.0f &&
                   m.m_fElementsCM[4] == 5.0f && m.m_fElementsCM[5] == 6.0f && m.m_fElementsCM[6] == 7.0f && m.m_fElementsCM[7] == 8.0f &&
                   m.m_fElementsCM[8] == 9.0f);
    }

    {
      plMat3T m;
      m.SetFromArray(data, plMatrixLayout::RowMajor);

      PLASMA_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && m.m_fElementsCM[1] == 4.0f && m.m_fElementsCM[2] == 7.0f && m.m_fElementsCM[3] == 2.0f &&
                   m.m_fElementsCM[4] == 5.0f && m.m_fElementsCM[5] == 8.0f && m.m_fElementsCM[6] == 3.0f && m.m_fElementsCM[7] == 6.0f &&
                   m.m_fElementsCM[8] == 9.0f);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetElements")
  {
    plMat3T m;
    m.SetElements(1, 2, 3, 4, 5, 6, 7, 8, 9);

    PLASMA_TEST_FLOAT(m.Element(0, 0), 1, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(1, 0), 2, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(2, 0), 3, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(0, 1), 4, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(1, 1), 5, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(2, 1), 6, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(0, 2), 7, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(1, 2), 8, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(2, 2), 9, 0.00001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetAsArray")
  {
    plMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    plMathTestType data[9];

    m.GetAsArray(data, plMatrixLayout::ColumnMajor);
    PLASMA_TEST_FLOAT(data[0], 1, 0.0001f);
    PLASMA_TEST_FLOAT(data[1], 4, 0.0001f);
    PLASMA_TEST_FLOAT(data[2], 7, 0.0001f);
    PLASMA_TEST_FLOAT(data[3], 2, 0.0001f);
    PLASMA_TEST_FLOAT(data[4], 5, 0.0001f);
    PLASMA_TEST_FLOAT(data[5], 8, 0.0001f);
    PLASMA_TEST_FLOAT(data[6], 3, 0.0001f);
    PLASMA_TEST_FLOAT(data[7], 6, 0.0001f);
    PLASMA_TEST_FLOAT(data[8], 9, 0.0001f);

    m.GetAsArray(data, plMatrixLayout::RowMajor);
    PLASMA_TEST_FLOAT(data[0], 1, 0.0001f);
    PLASMA_TEST_FLOAT(data[1], 2, 0.0001f);
    PLASMA_TEST_FLOAT(data[2], 3, 0.0001f);
    PLASMA_TEST_FLOAT(data[3], 4, 0.0001f);
    PLASMA_TEST_FLOAT(data[4], 5, 0.0001f);
    PLASMA_TEST_FLOAT(data[5], 6, 0.0001f);
    PLASMA_TEST_FLOAT(data[6], 7, 0.0001f);
    PLASMA_TEST_FLOAT(data[7], 8, 0.0001f);
    PLASMA_TEST_FLOAT(data[8], 9, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetZero")
  {
    plMat3T m;
    m.SetZero();

    for (plUInt32 i = 0; i < 9; ++i)
      PLASMA_TEST_FLOAT(m.m_fElementsCM[i], 0.0f, 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetIdentity")
  {
    plMat3T m;
    m.SetIdentity();

    PLASMA_TEST_FLOAT(m.Element(0, 0), 1, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 1), 1, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 2), 1, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetScalingMatrix")
  {
    plMat3T m;
    m.SetScalingMatrix(plVec3T(2, 3, 4));

    PLASMA_TEST_FLOAT(m.Element(0, 0), 2, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 1), 3, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 2), 4, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetRotationMatrixX")
  {
    plMat3T m;

    m.SetRotationMatrixX(plAngle::Degree(90));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(1, -3, 2), 0.0001f));

    m.SetRotationMatrixX(plAngle::Degree(180));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(1, -2, -3), 0.0001f));

    m.SetRotationMatrixX(plAngle::Degree(270));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(1, 3, -2), 0.0001f));

    m.SetRotationMatrixX(plAngle::Degree(360));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(1, 2, 3), 0.0001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetRotationMatrixY")
  {
    plMat3T m;

    m.SetRotationMatrixY(plAngle::Degree(90));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(3, 2, -1), 0.0001f));

    m.SetRotationMatrixY(plAngle::Degree(180));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(-1, 2, -3), 0.0001f));

    m.SetRotationMatrixY(plAngle::Degree(270));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(-3, 2, 1), 0.0001f));

    m.SetRotationMatrixY(plAngle::Degree(360));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(1, 2, 3), 0.0001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetRotationMatrixZ")
  {
    plMat3T m;

    m.SetRotationMatrixZ(plAngle::Degree(90));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(-2, 1, 3), 0.0001f));

    m.SetRotationMatrixZ(plAngle::Degree(180));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(-1, -2, 3), 0.0001f));

    m.SetRotationMatrixZ(plAngle::Degree(270));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(2, -1, 3), 0.0001f));

    m.SetRotationMatrixZ(plAngle::Degree(360));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(1, 2, 3), 0.0001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetRotationMatrix")
  {
    plMat3T m;

    m.SetRotationMatrix(plVec3T(1, 0, 0), plAngle::Degree(90));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(1, -3, 2), plMath::LargeEpsilon<plMathTestType>()));

    m.SetRotationMatrix(plVec3T(1, 0, 0), plAngle::Degree(180));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(1, -2, -3), plMath::LargeEpsilon<plMathTestType>()));

    m.SetRotationMatrix(plVec3T(1, 0, 0), plAngle::Degree(270));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(1, 3, -2), plMath::LargeEpsilon<plMathTestType>()));

    m.SetRotationMatrix(plVec3T(0, 1, 0), plAngle::Degree(90));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(3, 2, -1), plMath::LargeEpsilon<plMathTestType>()));

    m.SetRotationMatrix(plVec3T(0, 1, 0), plAngle::Degree(180));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(-1, 2, -3), plMath::LargeEpsilon<plMathTestType>()));

    m.SetRotationMatrix(plVec3T(0, 1, 0), plAngle::Degree(270));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(-3, 2, 1), plMath::LargeEpsilon<plMathTestType>()));

    m.SetRotationMatrix(plVec3T(0, 0, 1), plAngle::Degree(90));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(-2, 1, 3), plMath::LargeEpsilon<plMathTestType>()));

    m.SetRotationMatrix(plVec3T(0, 0, 1), plAngle::Degree(180));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(-1, -2, 3), plMath::LargeEpsilon<plMathTestType>()));

    m.SetRotationMatrix(plVec3T(0, 0, 1), plAngle::Degree(270));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(2, -1, 3), plMath::LargeEpsilon<plMathTestType>()));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IdentityMatrix")
  {
    plMat3T m = plMat3T::IdentityMatrix();

    PLASMA_TEST_FLOAT(m.Element(0, 0), 1, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 1), 1, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 2), 1, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ZeroMatrix")
  {
    plMat3T m = plMat3T::ZeroMatrix();

    PLASMA_TEST_FLOAT(m.Element(0, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 2), 0, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Transpose")
  {
    plMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    m.Transpose();

    PLASMA_TEST_FLOAT(m.Element(0, 0), 1, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(1, 0), 4, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(2, 0), 7, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(0, 1), 2, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(1, 1), 5, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(2, 1), 8, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(0, 2), 3, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(1, 2), 6, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(2, 2), 9, 0.00001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetTranspose")
  {
    plMat3T m0(1, 2, 3, 4, 5, 6, 7, 8, 9);

    plMat3T m = m0.GetTranspose();

    PLASMA_TEST_FLOAT(m.Element(0, 0), 1, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(1, 0), 4, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(2, 0), 7, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(0, 1), 2, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(1, 1), 5, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(2, 1), 8, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(0, 2), 3, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(1, 2), 6, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(2, 2), 9, 0.00001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Invert")
  {
    for (float x = 1.0f; x < 360.0f; x += 10.0f)
    {
      for (float y = 2.0f; y < 360.0f; y += 17.0f)
      {
        for (float z = 3.0f; z < 360.0f; z += 23.0f)
        {
          plMat3T m, inv;
          m.SetRotationMatrix(plVec3T(x, y, z).GetNormalized(), plAngle::Degree(19.0f));
          inv = m;
          PLASMA_TEST_BOOL(inv.Invert() == PLASMA_SUCCESS);

          plVec3T v = m * plVec3T(1, 1, 1);
          plVec3T vinv = inv * v;

          PLASMA_TEST_VEC3(vinv, plVec3T(1, 1, 1), plMath::DefaultEpsilon<plMathTestType>());
        }
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetInverse")
  {
    for (float x = 1.0f; x < 360.0f; x += 9.0f)
    {
      for (float y = 2.0f; y < 360.0f; y += 19.0f)
      {
        for (float z = 3.0f; z < 360.0f; z += 21.0f)
        {
          plMat3T m, inv;
          m.SetRotationMatrix(plVec3T(x, y, z).GetNormalized(), plAngle::Degree(83.0f));
          inv = m.GetInverse();

          plVec3T v = m * plVec3T(1, 1, 1);
          plVec3T vinv = inv * v;

          PLASMA_TEST_VEC3(vinv, plVec3T(1, 1, 1), plMath::DefaultEpsilon<plMathTestType>());
        }
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsZero")
  {
    plMat3T m;

    m.SetIdentity();
    PLASMA_TEST_BOOL(!m.IsZero());

    m.SetZero();
    PLASMA_TEST_BOOL(m.IsZero());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsIdentity")
  {
    plMat3T m;

    m.SetIdentity();
    PLASMA_TEST_BOOL(m.IsIdentity());

    m.SetZero();
    PLASMA_TEST_BOOL(!m.IsIdentity());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsValid")
  {
    if (plMath::SupportsNaN<plMat3T::ComponentType>())
    {
      plMat3T m;

      m.SetZero();
      PLASMA_TEST_BOOL(m.IsValid());

      m.m_fElementsCM[0] = plMath::NaN<plMat3T::ComponentType>();
      PLASMA_TEST_BOOL(!m.IsValid());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetRow")
  {
    plMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    PLASMA_TEST_VEC3(m.GetRow(0), plVec3T(1, 2, 3), 0.0f);
    PLASMA_TEST_VEC3(m.GetRow(1), plVec3T(4, 5, 6), 0.0f);
    PLASMA_TEST_VEC3(m.GetRow(2), plVec3T(7, 8, 9), 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetRow")
  {
    plMat3T m;
    m.SetZero();

    m.SetRow(0, plVec3T(1, 2, 3));
    PLASMA_TEST_VEC3(m.GetRow(0), plVec3T(1, 2, 3), 0.0f);

    m.SetRow(1, plVec3T(4, 5, 6));
    PLASMA_TEST_VEC3(m.GetRow(1), plVec3T(4, 5, 6), 0.0f);

    m.SetRow(2, plVec3T(7, 8, 9));
    PLASMA_TEST_VEC3(m.GetRow(2), plVec3T(7, 8, 9), 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetColumn")
  {
    plMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    PLASMA_TEST_VEC3(m.GetColumn(0), plVec3T(1, 4, 7), 0.0f);
    PLASMA_TEST_VEC3(m.GetColumn(1), plVec3T(2, 5, 8), 0.0f);
    PLASMA_TEST_VEC3(m.GetColumn(2), plVec3T(3, 6, 9), 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetColumn")
  {
    plMat3T m;
    m.SetZero();

    m.SetColumn(0, plVec3T(1, 2, 3));
    PLASMA_TEST_VEC3(m.GetColumn(0), plVec3T(1, 2, 3), 0.0f);

    m.SetColumn(1, plVec3T(4, 5, 6));
    PLASMA_TEST_VEC3(m.GetColumn(1), plVec3T(4, 5, 6), 0.0f);

    m.SetColumn(2, plVec3T(7, 8, 9));
    PLASMA_TEST_VEC3(m.GetColumn(2), plVec3T(7, 8, 9), 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetDiagonal")
  {
    plMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    PLASMA_TEST_VEC3(m.GetDiagonal(), plVec3T(1, 5, 9), 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetDiagonal")
  {
    plMat3T m;
    m.SetZero();

    m.SetDiagonal(plVec3T(1, 2, 3));
    PLASMA_TEST_VEC3(m.GetColumn(0), plVec3T(1, 0, 0), 0.0f);
    PLASMA_TEST_VEC3(m.GetColumn(1), plVec3T(0, 2, 0), 0.0f);
    PLASMA_TEST_VEC3(m.GetColumn(2), plVec3T(0, 0, 3), 0.0f);
  }


  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetScalingFactors")
  {
    plMat3T m(1, 2, 3, 5, 6, 7, 9, 10, 11);

    plVec3T s = m.GetScalingFactors();
    PLASMA_TEST_VEC3(s,
      plVec3T(plMath::Sqrt((plMathTestType)(1 * 1 + 5 * 5 + 9 * 9)), plMath::Sqrt((plMathTestType)(2 * 2 + 6 * 6 + 10 * 10)),
        plMath::Sqrt((plMathTestType)(3 * 3 + 7 * 7 + 11 * 11))),
      0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetScalingFactors")
  {
    plMat3T m(1, 2, 3, 5, 6, 7, 9, 10, 11);

    PLASMA_TEST_BOOL(m.SetScalingFactors(plVec3T(1, 2, 3)) == PLASMA_SUCCESS);

    plVec3T s = m.GetScalingFactors();
    PLASMA_TEST_VEC3(s, plVec3T(1, 2, 3), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "TransformDirection")
  {
    plMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    const plVec3T r = m.TransformDirection(plVec3T(1, 2, 3));

    PLASMA_TEST_VEC3(r, plVec3T(1 * 1 + 2 * 2 + 3 * 3, 1 * 4 + 2 * 5 + 3 * 6, 1 * 7 + 2 * 8 + 3 * 9), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator*=")
  {
    plMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    m *= 2.0f;

    PLASMA_TEST_VEC3(m.GetRow(0), plVec3T(2, 4, 6), 0.0001f);
    PLASMA_TEST_VEC3(m.GetRow(1), plVec3T(8, 10, 12), 0.0001f);
    PLASMA_TEST_VEC3(m.GetRow(2), plVec3T(14, 16, 18), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator/=")
  {
    plMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    m *= 4.0f;
    m /= 2.0f;

    PLASMA_TEST_VEC3(m.GetRow(0), plVec3T(2, 4, 6), 0.0001f);
    PLASMA_TEST_VEC3(m.GetRow(1), plVec3T(8, 10, 12), 0.0001f);
    PLASMA_TEST_VEC3(m.GetRow(2), plVec3T(14, 16, 18), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsIdentical")
  {
    plMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    plMat3T m2 = m;

    PLASMA_TEST_BOOL(m.IsIdentical(m2));

    m2.m_fElementsCM[0] += 0.00001f;
    PLASMA_TEST_BOOL(!m.IsIdentical(m2));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEqual")
  {
    plMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    plMat3T m2 = m;

    PLASMA_TEST_BOOL(m.IsEqual(m2, 0.0001f));

    m2.m_fElementsCM[0] += 0.00001f;
    PLASMA_TEST_BOOL(m.IsEqual(m2, 0.0001f));
    PLASMA_TEST_BOOL(!m.IsEqual(m2, 0.000001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator*(mat, mat)")
  {
    plMat3T m1(1, 2, 3, 4, 5, 6, 7, 8, 9);

    plMat3T m2(-1, -2, -3, -4, -5, -6, -7, -8, -9);

    plMat3T r = m1 * m2;

    PLASMA_TEST_VEC3(r.GetColumn(0), plVec3T(-1 * 1 + -4 * 2 + -7 * 3, -1 * 4 + -4 * 5 + -7 * 6, -1 * 7 + -4 * 8 + -7 * 9), 0.001f);
    PLASMA_TEST_VEC3(r.GetColumn(1), plVec3T(-2 * 1 + -5 * 2 + -8 * 3, -2 * 4 + -5 * 5 + -8 * 6, -2 * 7 + -5 * 8 + -8 * 9), 0.001f);
    PLASMA_TEST_VEC3(r.GetColumn(2), plVec3T(-3 * 1 + -6 * 2 + -9 * 3, -3 * 4 + -6 * 5 + -9 * 6, -3 * 7 + -6 * 8 + -9 * 9), 0.001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator*(mat, vec)")
  {
    plMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    const plVec3T r = m * (plVec3T(1, 2, 3));

    PLASMA_TEST_VEC3(r, plVec3T(1 * 1 + 2 * 2 + 3 * 3, 1 * 4 + 2 * 5 + 3 * 6, 1 * 7 + 2 * 8 + 3 * 9), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator*(mat, float) | operator*(float, mat)")
  {
    plMat3T m0(1, 2, 3, 4, 5, 6, 7, 8, 9);

    plMat3T m = m0 * (plMathTestType)2;
    plMat3T m2 = (plMathTestType)2 * m0;

    PLASMA_TEST_VEC3(m.GetRow(0), plVec3T(2, 4, 6), 0.0001f);
    PLASMA_TEST_VEC3(m.GetRow(1), plVec3T(8, 10, 12), 0.0001f);
    PLASMA_TEST_VEC3(m.GetRow(2), plVec3T(14, 16, 18), 0.0001f);

    PLASMA_TEST_VEC3(m2.GetRow(0), plVec3T(2, 4, 6), 0.0001f);
    PLASMA_TEST_VEC3(m2.GetRow(1), plVec3T(8, 10, 12), 0.0001f);
    PLASMA_TEST_VEC3(m2.GetRow(2), plVec3T(14, 16, 18), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator/(mat, float)")
  {
    plMat3T m0(1, 2, 3, 4, 5, 6, 7, 8, 9);

    m0 *= 4.0f;

    plMat3T m = m0 / (plMathTestType)2;

    PLASMA_TEST_VEC3(m.GetRow(0), plVec3T(2, 4, 6), 0.0001f);
    PLASMA_TEST_VEC3(m.GetRow(1), plVec3T(8, 10, 12), 0.0001f);
    PLASMA_TEST_VEC3(m.GetRow(2), plVec3T(14, 16, 18), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator+(mat, mat) | operator-(mat, mat)")
  {
    plMat3T m0(1, 2, 3, 4, 5, 6, 7, 8, 9);

    plMat3T m1(-1, -2, -3, -4, -5, -6, -7, -8, -9);

    PLASMA_TEST_BOOL((m0 + m1).IsZero());
    PLASMA_TEST_BOOL((m0 - m1).IsEqual(m0 * (plMathTestType)2, 0.0001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator== (mat, mat) | operator!= (mat, mat)")
  {
    plMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    plMat3T m2 = m;

    PLASMA_TEST_BOOL(m == m2);

    m2.m_fElementsCM[0] += 0.00001f;

    PLASMA_TEST_BOOL(m != m2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsNaN")
  {
    if (plMath::SupportsNaN<plMathTestType>())
    {
      plMat3T m;

      m.SetIdentity();
      PLASMA_TEST_BOOL(!m.IsNaN());

      for (plUInt32 i = 0; i < 9; ++i)
      {
        m.SetIdentity();
        m.m_fElementsCM[i] = plMath::NaN<plMathTestType>();

        PLASMA_TEST_BOOL(m.IsNaN());
      }
    }
  }
}
