#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Implementation/AllClasses_inl.h>
#include <Foundation/Math/Mat4.h>

PLASMA_CREATE_SIMPLE_TEST(Math, Mat4)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Default Constructor")
  {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    if (plMath::SupportsNaN<plMat3T::ComponentType>())
    {
      // In debug the default constructor initializes everything with NaN.
      plMat4T m;
      PLASMA_TEST_BOOL(plMath::IsNaN(m.m_fElementsCM[0]) && plMath::IsNaN(m.m_fElementsCM[1]) && plMath::IsNaN(m.m_fElementsCM[2]) &&
                   plMath::IsNaN(m.m_fElementsCM[3]) && plMath::IsNaN(m.m_fElementsCM[4]) && plMath::IsNaN(m.m_fElementsCM[5]) &&
                   plMath::IsNaN(m.m_fElementsCM[6]) && plMath::IsNaN(m.m_fElementsCM[7]) && plMath::IsNaN(m.m_fElementsCM[8]) &&
                   plMath::IsNaN(m.m_fElementsCM[9]) && plMath::IsNaN(m.m_fElementsCM[10]) && plMath::IsNaN(m.m_fElementsCM[11]) &&
                   plMath::IsNaN(m.m_fElementsCM[12]) && plMath::IsNaN(m.m_fElementsCM[13]) && plMath::IsNaN(m.m_fElementsCM[14]) &&
                   plMath::IsNaN(m.m_fElementsCM[15]));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    plMat4T::ComponentType testBlock[16] = {(plMat4T::ComponentType)1, (plMat4T::ComponentType)2, (plMat4T::ComponentType)3,
      (plMat4T::ComponentType)4, (plMat4T::ComponentType)5, (plMat4T::ComponentType)6, (plMat4T::ComponentType)7, (plMat4T::ComponentType)8,
      (plMat4T::ComponentType)9, (plMat4T::ComponentType)10, (plMat4T::ComponentType)11, (plMat4T::ComponentType)12, (plMat4T::ComponentType)13,
      (plMat4T::ComponentType)14, (plMat4T::ComponentType)15, (plMat4T::ComponentType)16};
    plMat4T* m = ::new ((void*)&testBlock[0]) plMat4T;

    PLASMA_TEST_BOOL(m->m_fElementsCM[0] == 1.0f && m->m_fElementsCM[1] == 2.0f && m->m_fElementsCM[2] == 3.0f && m->m_fElementsCM[3] == 4.0f &&
                 m->m_fElementsCM[4] == 5.0f && m->m_fElementsCM[5] == 6.0f && m->m_fElementsCM[6] == 7.0f && m->m_fElementsCM[7] == 8.0f &&
                 m->m_fElementsCM[8] == 9.0f && m->m_fElementsCM[9] == 10.0f && m->m_fElementsCM[10] == 11.0f && m->m_fElementsCM[11] == 12.0f &&
                 m->m_fElementsCM[12] == 13.0f && m->m_fElementsCM[13] == 14.0f && m->m_fElementsCM[14] == 15.0f && m->m_fElementsCM[15] == 16.0f);
#endif
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor (Array Data)")
  {
    const plMathTestType data[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    {
      plMat4T m(data, plMatrixLayout::ColumnMajor);

      PLASMA_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && m.m_fElementsCM[1] == 2.0f && m.m_fElementsCM[2] == 3.0f && m.m_fElementsCM[3] == 4.0f &&
                   m.m_fElementsCM[4] == 5.0f && m.m_fElementsCM[5] == 6.0f && m.m_fElementsCM[6] == 7.0f && m.m_fElementsCM[7] == 8.0f &&
                   m.m_fElementsCM[8] == 9.0f && m.m_fElementsCM[9] == 10.0f && m.m_fElementsCM[10] == 11.0f && m.m_fElementsCM[11] == 12.0f &&
                   m.m_fElementsCM[12] == 13.0f && m.m_fElementsCM[13] == 14.0f && m.m_fElementsCM[14] == 15.0f && m.m_fElementsCM[15] == 16.0f);
    }

    {
      plMat4T m(data, plMatrixLayout::RowMajor);

      PLASMA_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && m.m_fElementsCM[1] == 5.0f && m.m_fElementsCM[2] == 9.0f && m.m_fElementsCM[3] == 13.0f &&
                   m.m_fElementsCM[4] == 2.0f && m.m_fElementsCM[5] == 6.0f && m.m_fElementsCM[6] == 10.0f && m.m_fElementsCM[7] == 14.0f &&
                   m.m_fElementsCM[8] == 3.0f && m.m_fElementsCM[9] == 7.0f && m.m_fElementsCM[10] == 11.0f && m.m_fElementsCM[11] == 15.0f &&
                   m.m_fElementsCM[12] == 4.0f && m.m_fElementsCM[13] == 8.0f && m.m_fElementsCM[14] == 12.0f && m.m_fElementsCM[15] == 16.0f);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor (Elements)")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    PLASMA_TEST_FLOAT(m.Element(0, 0), 1, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(1, 0), 2, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(2, 0), 3, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(3, 0), 4, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(0, 1), 5, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(1, 1), 6, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(2, 1), 7, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(3, 1), 8, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(0, 2), 9, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(1, 2), 10, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(2, 2), 11, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(3, 2), 12, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(0, 3), 13, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(1, 3), 14, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(2, 3), 15, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(3, 3), 16, 0.00001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor (composite)")
  {
    plMat3T mr(1, 2, 3, 4, 5, 6, 7, 8, 9);
    plVec3T vt(10, 11, 12);

    plMat4T m(mr, vt);

    PLASMA_TEST_FLOAT(m.Element(0, 0), 1, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 0), 2, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 0), 3, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 0), 10, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 1), 4, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 1), 5, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 1), 6, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 1), 11, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 2), 7, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 2), 8, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 2), 9, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 2), 12, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 3), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 3), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 3), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetFromArray")
  {
    const plMathTestType data[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    {
      plMat4T m;
      m.SetFromArray(data, plMatrixLayout::ColumnMajor);

      PLASMA_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && m.m_fElementsCM[1] == 2.0f && m.m_fElementsCM[2] == 3.0f && m.m_fElementsCM[3] == 4.0f &&
                   m.m_fElementsCM[4] == 5.0f && m.m_fElementsCM[5] == 6.0f && m.m_fElementsCM[6] == 7.0f && m.m_fElementsCM[7] == 8.0f &&
                   m.m_fElementsCM[8] == 9.0f && m.m_fElementsCM[9] == 10.0f && m.m_fElementsCM[10] == 11.0f && m.m_fElementsCM[11] == 12.0f &&
                   m.m_fElementsCM[12] == 13.0f && m.m_fElementsCM[13] == 14.0f && m.m_fElementsCM[14] == 15.0f && m.m_fElementsCM[15] == 16.0f);
    }

    {
      plMat4T m;
      m.SetFromArray(data, plMatrixLayout::RowMajor);

      PLASMA_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && m.m_fElementsCM[1] == 5.0f && m.m_fElementsCM[2] == 9.0f && m.m_fElementsCM[3] == 13.0f &&
                   m.m_fElementsCM[4] == 2.0f && m.m_fElementsCM[5] == 6.0f && m.m_fElementsCM[6] == 10.0f && m.m_fElementsCM[7] == 14.0f &&
                   m.m_fElementsCM[8] == 3.0f && m.m_fElementsCM[9] == 7.0f && m.m_fElementsCM[10] == 11.0f && m.m_fElementsCM[11] == 15.0f &&
                   m.m_fElementsCM[12] == 4.0f && m.m_fElementsCM[13] == 8.0f && m.m_fElementsCM[14] == 12.0f && m.m_fElementsCM[15] == 16.0f);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetElements")
  {
    plMat4T m;
    m.SetElements(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    PLASMA_TEST_FLOAT(m.Element(0, 0), 1, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(1, 0), 2, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(2, 0), 3, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(3, 0), 4, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(0, 1), 5, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(1, 1), 6, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(2, 1), 7, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(3, 1), 8, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(0, 2), 9, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(1, 2), 10, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(2, 2), 11, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(3, 2), 12, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(0, 3), 13, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(1, 3), 14, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(2, 3), 15, 0.00001f);
    PLASMA_TEST_FLOAT(m.Element(3, 3), 16, 0.00001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetTransformationMatrix")
  {
    plMat3T mr(1, 2, 3, 4, 5, 6, 7, 8, 9);
    plVec3T vt(10, 11, 12);

    plMat4T m;
    m.SetTransformationMatrix(mr, vt);

    PLASMA_TEST_FLOAT(m.Element(0, 0), 1, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 0), 2, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 0), 3, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 0), 10, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 1), 4, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 1), 5, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 1), 6, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 1), 11, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 2), 7, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 2), 8, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 2), 9, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 2), 12, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 3), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 3), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 3), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetAsArray")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    plMathTestType data[16];

    m.GetAsArray(data, plMatrixLayout::ColumnMajor);
    PLASMA_TEST_FLOAT(data[0], 1, 0.0001f);
    PLASMA_TEST_FLOAT(data[1], 5, 0.0001f);
    PLASMA_TEST_FLOAT(data[2], 9, 0.0001f);
    PLASMA_TEST_FLOAT(data[3], 13, 0.0001f);
    PLASMA_TEST_FLOAT(data[4], 2, 0.0001f);
    PLASMA_TEST_FLOAT(data[5], 6, 0.0001f);
    PLASMA_TEST_FLOAT(data[6], 10, 0.0001f);
    PLASMA_TEST_FLOAT(data[7], 14, 0.0001f);
    PLASMA_TEST_FLOAT(data[8], 3, 0.0001f);
    PLASMA_TEST_FLOAT(data[9], 7, 0.0001f);
    PLASMA_TEST_FLOAT(data[10], 11, 0.0001f);
    PLASMA_TEST_FLOAT(data[11], 15, 0.0001f);
    PLASMA_TEST_FLOAT(data[12], 4, 0.0001f);
    PLASMA_TEST_FLOAT(data[13], 8, 0.0001f);
    PLASMA_TEST_FLOAT(data[14], 12, 0.0001f);
    PLASMA_TEST_FLOAT(data[15], 16, 0.0001f);

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
    PLASMA_TEST_FLOAT(data[9], 10, 0.0001f);
    PLASMA_TEST_FLOAT(data[10], 11, 0.0001f);
    PLASMA_TEST_FLOAT(data[11], 12, 0.0001f);
    PLASMA_TEST_FLOAT(data[12], 13, 0.0001f);
    PLASMA_TEST_FLOAT(data[13], 14, 0.0001f);
    PLASMA_TEST_FLOAT(data[14], 15, 0.0001f);
    PLASMA_TEST_FLOAT(data[15], 16, 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetZero")
  {
    plMat4T m;
    m.SetZero();

    for (plUInt32 i = 0; i < 16; ++i)
      PLASMA_TEST_FLOAT(m.m_fElementsCM[i], 0.0f, 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetIdentity")
  {
    plMat4T m;
    m.SetIdentity();

    PLASMA_TEST_FLOAT(m.Element(0, 0), 1, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 1), 1, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 2), 1, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 3), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 3), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 3), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetTranslationMatrix")
  {
    plMat4T m;
    m.SetTranslationMatrix(plVec3T(2, 3, 4));

    PLASMA_TEST_FLOAT(m.Element(0, 0), 1, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 0), 2, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 1), 1, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 1), 3, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 2), 1, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 2), 4, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 3), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 3), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 3), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetScalingMatrix")
  {
    plMat4T m;
    m.SetScalingMatrix(plVec3T(2, 3, 4));

    PLASMA_TEST_FLOAT(m.Element(0, 0), 2, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 1), 3, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 2), 4, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 3), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 3), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 3), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetRotationMatrixX")
  {
    plMat4T m;

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
    plMat4T m;

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
    plMat4T m;

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
    plMat4T m;

    m.SetRotationMatrix(plVec3T(1, 0, 0), plAngle::Degree(90));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(1, -3, 2), plMath::DefaultEpsilon<plMat3T::ComponentType>()));

    m.SetRotationMatrix(plVec3T(1, 0, 0), plAngle::Degree(180));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(1, -2, -3), plMath::DefaultEpsilon<plMat3T::ComponentType>()));

    m.SetRotationMatrix(plVec3T(1, 0, 0), plAngle::Degree(270));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(1, 3, -2), plMath::DefaultEpsilon<plMat3T::ComponentType>()));

    m.SetRotationMatrix(plVec3T(0, 1, 0), plAngle::Degree(90));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(3, 2, -1), plMath::DefaultEpsilon<plMat3T::ComponentType>()));

    m.SetRotationMatrix(plVec3T(0, 1, 0), plAngle::Degree(180));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(-1, 2, -3), plMath::DefaultEpsilon<plMat3T::ComponentType>()));

    m.SetRotationMatrix(plVec3T(0, 1, 0), plAngle::Degree(270));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(-3, 2, 1), plMath::DefaultEpsilon<plMat3T::ComponentType>()));

    m.SetRotationMatrix(plVec3T(0, 0, 1), plAngle::Degree(90));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(-2, 1, 3), plMath::DefaultEpsilon<plMat3T::ComponentType>()));

    m.SetRotationMatrix(plVec3T(0, 0, 1), plAngle::Degree(180));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(-1, -2, 3), plMath::DefaultEpsilon<plMat3T::ComponentType>()));

    m.SetRotationMatrix(plVec3T(0, 0, 1), plAngle::Degree(270));
    PLASMA_TEST_BOOL((m * plVec3T(1, 2, 3)).IsEqual(plVec3T(2, -1, 3), plMath::DefaultEpsilon<plMat3T::ComponentType>()));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IdentityMatrix")
  {
    plMat4T m = plMat4T::IdentityMatrix();

    PLASMA_TEST_FLOAT(m.Element(0, 0), 1, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 1), 1, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 2), 1, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 3), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 3), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 3), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ZeroMatrix")
  {
    plMat4T m = plMat4T::ZeroMatrix();

    PLASMA_TEST_FLOAT(m.Element(0, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 0), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 1), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 2), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 3), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 3), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 3), 0, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 3), 0, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Transpose")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m.Transpose();

    PLASMA_TEST_FLOAT(m.Element(0, 0), 1, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 0), 5, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 0), 9, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 0), 13, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 1), 2, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 1), 6, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 1), 10, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 1), 14, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 2), 3, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 2), 7, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 2), 11, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 2), 15, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 3), 4, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 3), 8, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 3), 12, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 3), 16, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetTranspose")
  {
    plMat4T m0(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    plMat4T m = m0.GetTranspose();

    PLASMA_TEST_FLOAT(m.Element(0, 0), 1, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 0), 5, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 0), 9, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 0), 13, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 1), 2, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 1), 6, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 1), 10, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 1), 14, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 2), 3, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 2), 7, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 2), 11, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 2), 15, 0);
    PLASMA_TEST_FLOAT(m.Element(0, 3), 4, 0);
    PLASMA_TEST_FLOAT(m.Element(1, 3), 8, 0);
    PLASMA_TEST_FLOAT(m.Element(2, 3), 12, 0);
    PLASMA_TEST_FLOAT(m.Element(3, 3), 16, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Invert")
  {
    for (float x = 1.0f; x < 360.0f; x += 10.0f)
    {
      for (float y = 2.0f; y < 360.0f; y += 17.0f)
      {
        for (float z = 3.0f; z < 360.0f; z += 23.0f)
        {
          plMat4T m, inv;
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
          plMat4T m, inv;
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
    plMat4T m;

    m.SetIdentity();
    PLASMA_TEST_BOOL(!m.IsZero());

    m.SetZero();
    PLASMA_TEST_BOOL(m.IsZero());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsIdentity")
  {
    plMat4T m;

    m.SetIdentity();
    PLASMA_TEST_BOOL(m.IsIdentity());

    m.SetZero();
    PLASMA_TEST_BOOL(!m.IsIdentity());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsValid")
  {
    if (plMath::SupportsNaN<plMat3T::ComponentType>())
    {
      plMat4T m;

      m.SetZero();
      PLASMA_TEST_BOOL(m.IsValid());

      m.m_fElementsCM[0] = plMath::NaN<plMat4T::ComponentType>();
      PLASMA_TEST_BOOL(!m.IsValid());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetRow")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    PLASMA_TEST_VEC4(m.GetRow(0), plVec4T(1, 2, 3, 4), 0.0f);
    PLASMA_TEST_VEC4(m.GetRow(1), plVec4T(5, 6, 7, 8), 0.0f);
    PLASMA_TEST_VEC4(m.GetRow(2), plVec4T(9, 10, 11, 12), 0.0f);
    PLASMA_TEST_VEC4(m.GetRow(3), plVec4T(13, 14, 15, 16), 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetRow")
  {
    plMat4T m;
    m.SetZero();

    m.SetRow(0, plVec4T(1, 2, 3, 4));
    PLASMA_TEST_VEC4(m.GetRow(0), plVec4T(1, 2, 3, 4), 0.0f);

    m.SetRow(1, plVec4T(5, 6, 7, 8));
    PLASMA_TEST_VEC4(m.GetRow(1), plVec4T(5, 6, 7, 8), 0.0f);

    m.SetRow(2, plVec4T(9, 10, 11, 12));
    PLASMA_TEST_VEC4(m.GetRow(2), plVec4T(9, 10, 11, 12), 0.0f);

    m.SetRow(3, plVec4T(13, 14, 15, 16));
    PLASMA_TEST_VEC4(m.GetRow(3), plVec4T(13, 14, 15, 16), 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetColumn")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    PLASMA_TEST_VEC4(m.GetColumn(0), plVec4T(1, 5, 9, 13), 0.0f);
    PLASMA_TEST_VEC4(m.GetColumn(1), plVec4T(2, 6, 10, 14), 0.0f);
    PLASMA_TEST_VEC4(m.GetColumn(2), plVec4T(3, 7, 11, 15), 0.0f);
    PLASMA_TEST_VEC4(m.GetColumn(3), plVec4T(4, 8, 12, 16), 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetColumn")
  {
    plMat4T m;
    m.SetZero();

    m.SetColumn(0, plVec4T(1, 2, 3, 4));
    PLASMA_TEST_VEC4(m.GetColumn(0), plVec4T(1, 2, 3, 4), 0.0f);

    m.SetColumn(1, plVec4T(5, 6, 7, 8));
    PLASMA_TEST_VEC4(m.GetColumn(1), plVec4T(5, 6, 7, 8), 0.0f);

    m.SetColumn(2, plVec4T(9, 10, 11, 12));
    PLASMA_TEST_VEC4(m.GetColumn(2), plVec4T(9, 10, 11, 12), 0.0f);

    m.SetColumn(3, plVec4T(13, 14, 15, 16));
    PLASMA_TEST_VEC4(m.GetColumn(3), plVec4T(13, 14, 15, 16), 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetDiagonal")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    PLASMA_TEST_VEC4(m.GetDiagonal(), plVec4T(1, 6, 11, 16), 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetDiagonal")
  {
    plMat4T m;
    m.SetZero();

    m.SetDiagonal(plVec4T(1, 2, 3, 4));
    PLASMA_TEST_VEC4(m.GetColumn(0), plVec4T(1, 0, 0, 0), 0.0f);
    PLASMA_TEST_VEC4(m.GetColumn(1), plVec4T(0, 2, 0, 0), 0.0f);
    PLASMA_TEST_VEC4(m.GetColumn(2), plVec4T(0, 0, 3, 0), 0.0f);
    PLASMA_TEST_VEC4(m.GetColumn(3), plVec4T(0, 0, 0, 4), 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetTranslationVector")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    PLASMA_TEST_VEC3(m.GetTranslationVector(), plVec3T(4, 8, 12), 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetTranslationVector")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m.SetTranslationVector(plVec3T(17, 18, 19));
    PLASMA_TEST_VEC4(m.GetRow(0), plVec4T(1, 2, 3, 17), 0.0f);
    PLASMA_TEST_VEC4(m.GetRow(1), plVec4T(5, 6, 7, 18), 0.0f);
    PLASMA_TEST_VEC4(m.GetRow(2), plVec4T(9, 10, 11, 19), 0.0f);
    PLASMA_TEST_VEC4(m.GetRow(3), plVec4T(13, 14, 15, 16), 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetRotationalPart")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    plMat3T r(17, 18, 19, 20, 21, 22, 23, 24, 25);

    m.SetRotationalPart(r);
    PLASMA_TEST_VEC4(m.GetRow(0), plVec4T(17, 18, 19, 4), 0.0f);
    PLASMA_TEST_VEC4(m.GetRow(1), plVec4T(20, 21, 22, 8), 0.0f);
    PLASMA_TEST_VEC4(m.GetRow(2), plVec4T(23, 24, 25, 12), 0.0f);
    PLASMA_TEST_VEC4(m.GetRow(3), plVec4T(13, 14, 15, 16), 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetRotationalPart")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    plMat3T r = m.GetRotationalPart();
    PLASMA_TEST_VEC3(r.GetRow(0), plVec3T(1, 2, 3), 0.0f);
    PLASMA_TEST_VEC3(r.GetRow(1), plVec3T(5, 6, 7), 0.0f);
    PLASMA_TEST_VEC3(r.GetRow(2), plVec3T(9, 10, 11), 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetScalingFactors")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    plVec3T s = m.GetScalingFactors();
    PLASMA_TEST_VEC3(s,
      plVec3T(plMath::Sqrt((plMathTestType)(1 * 1 + 5 * 5 + 9 * 9)), plMath::Sqrt((plMathTestType)(2 * 2 + 6 * 6 + 10 * 10)),
        plMath::Sqrt((plMathTestType)(3 * 3 + 7 * 7 + 11 * 11))),
      0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetScalingFactors")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    PLASMA_TEST_BOOL(m.SetScalingFactors(plVec3T(1, 2, 3)) == PLASMA_SUCCESS);

    plVec3T s = m.GetScalingFactors();
    PLASMA_TEST_VEC3(s, plVec3T(1, 2, 3), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "TransformDirection")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const plVec3T r = m.TransformDirection(plVec3T(1, 2, 3));

    PLASMA_TEST_VEC3(r, plVec3T(1 * 1 + 2 * 2 + 3 * 3, 1 * 5 + 2 * 6 + 3 * 7, 1 * 9 + 2 * 10 + 3 * 11), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "TransformDirection(array)")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    plVec3T data[3] = {plVec3T(1, 2, 3), plVec3T(4, 5, 6), plVec3T(7, 8, 9)};

    m.TransformDirection(data, 2);

    PLASMA_TEST_VEC3(data[0], plVec3T(1 * 1 + 2 * 2 + 3 * 3, 1 * 5 + 2 * 6 + 3 * 7, 1 * 9 + 2 * 10 + 3 * 11), 0.0001f);
    PLASMA_TEST_VEC3(data[1], plVec3T(4 * 1 + 5 * 2 + 6 * 3, 4 * 5 + 5 * 6 + 6 * 7, 4 * 9 + 5 * 10 + 6 * 11), 0.0001f);
    PLASMA_TEST_VEC3(data[2], plVec3T(7, 8, 9), 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "TransformPosition")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const plVec3T r = m.TransformPosition(plVec3T(1, 2, 3));

    PLASMA_TEST_VEC3(r, plVec3T(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "TransformPosition(array)")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    plVec3T data[3] = {plVec3T(1, 2, 3), plVec3T(4, 5, 6), plVec3T(7, 8, 9)};

    m.TransformPosition(data, 2);

    PLASMA_TEST_VEC3(data[0], plVec3T(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), 0.0001f);
    PLASMA_TEST_VEC3(data[1], plVec3T(4 * 1 + 5 * 2 + 6 * 3 + 4, 4 * 5 + 5 * 6 + 6 * 7 + 8, 4 * 9 + 5 * 10 + 6 * 11 + 12), 0.0001f);
    PLASMA_TEST_VEC3(data[2], plVec3T(7, 8, 9), 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Transform")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const plVec4T r = m.Transform(plVec4T(1, 2, 3, 4));

    PLASMA_TEST_VEC4(r,
      plVec4T(1 * 1 + 2 * 2 + 3 * 3 + 4 * 4, 1 * 5 + 2 * 6 + 3 * 7 + 8 * 4, 1 * 9 + 2 * 10 + 3 * 11 + 12 * 4, 1 * 13 + 2 * 14 + 3 * 15 + 4 * 16),
      0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Transform(array)")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    plVec4T data[3] = {plVec4T(1, 2, 3, 4), plVec4T(5, 6, 7, 8), plVec4T(9, 10, 11, 12)};

    m.Transform(data, 2);

    PLASMA_TEST_VEC4(data[0],
      plVec4T(1 * 1 + 2 * 2 + 3 * 3 + 4 * 4, 1 * 5 + 2 * 6 + 3 * 7 + 8 * 4, 1 * 9 + 2 * 10 + 3 * 11 + 12 * 4, 1 * 13 + 2 * 14 + 3 * 15 + 4 * 16),
      0.0001f);
    PLASMA_TEST_VEC4(data[1],
      plVec4T(5 * 1 + 6 * 2 + 7 * 3 + 8 * 4, 5 * 5 + 6 * 6 + 7 * 7 + 8 * 8, 5 * 9 + 6 * 10 + 7 * 11 + 12 * 8, 5 * 13 + 6 * 14 + 7 * 15 + 8 * 16),
      0.0001f);
    PLASMA_TEST_VEC4(data[2], plVec4T(9, 10, 11, 12), 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator*=")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m *= 2.0f;

    PLASMA_TEST_VEC4(m.GetRow(0), plVec4T(2, 4, 6, 8), 0.0001f);
    PLASMA_TEST_VEC4(m.GetRow(1), plVec4T(10, 12, 14, 16), 0.0001f);
    PLASMA_TEST_VEC4(m.GetRow(2), plVec4T(18, 20, 22, 24), 0.0001f);
    PLASMA_TEST_VEC4(m.GetRow(3), plVec4T(26, 28, 30, 32), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator/=")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m *= 4.0f;
    m /= 2.0f;

    PLASMA_TEST_VEC4(m.GetRow(0), plVec4T(2, 4, 6, 8), 0.0001f);
    PLASMA_TEST_VEC4(m.GetRow(1), plVec4T(10, 12, 14, 16), 0.0001f);
    PLASMA_TEST_VEC4(m.GetRow(2), plVec4T(18, 20, 22, 24), 0.0001f);
    PLASMA_TEST_VEC4(m.GetRow(3), plVec4T(26, 28, 30, 32), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsIdentical")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    plMat4T m2 = m;

    PLASMA_TEST_BOOL(m.IsIdentical(m2));

    m2.m_fElementsCM[0] += 0.00001f;
    PLASMA_TEST_BOOL(!m.IsIdentical(m2));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEqual")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    plMat4T m2 = m;

    PLASMA_TEST_BOOL(m.IsEqual(m2, 0.0001f));

    m2.m_fElementsCM[0] += 0.00001f;
    PLASMA_TEST_BOOL(m.IsEqual(m2, 0.0001f));
    PLASMA_TEST_BOOL(!m.IsEqual(m2, 0.000001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator*(mat, mat)")
  {
    plMat4T m1(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    plMat4T m2(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16);

    plMat4T r = m1 * m2;

    PLASMA_TEST_VEC4(r.GetColumn(0),
      plVec4T(-1 * 1 + -5 * 2 + -9 * 3 + -13 * 4, -1 * 5 + -5 * 6 + -9 * 7 + -13 * 8, -1 * 9 + -5 * 10 + -9 * 11 + -13 * 12,
        -1 * 13 + -5 * 14 + -9 * 15 + -13 * 16),
      0.001f);
    PLASMA_TEST_VEC4(r.GetColumn(1),
      plVec4T(-2 * 1 + -6 * 2 + -10 * 3 + -14 * 4, -2 * 5 + -6 * 6 + -10 * 7 + -14 * 8, -2 * 9 + -6 * 10 + -10 * 11 + -14 * 12,
        -2 * 13 + -6 * 14 + -10 * 15 + -14 * 16),
      0.001f);
    PLASMA_TEST_VEC4(r.GetColumn(2),
      plVec4T(-3 * 1 + -7 * 2 + -11 * 3 + -15 * 4, -3 * 5 + -7 * 6 + -11 * 7 + -15 * 8, -3 * 9 + -7 * 10 + -11 * 11 + -15 * 12,
        -3 * 13 + -7 * 14 + -11 * 15 + -15 * 16),
      0.001f);
    PLASMA_TEST_VEC4(r.GetColumn(3),
      plVec4T(-4 * 1 + -8 * 2 + -12 * 3 + -16 * 4, -4 * 5 + -8 * 6 + -12 * 7 + -16 * 8, -4 * 9 + -8 * 10 + -12 * 11 + -16 * 12,
        -4 * 13 + -8 * 14 + -12 * 15 + -16 * 16),
      0.001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator*(mat, vec3)")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const plVec3T r = m * plVec3T(1, 2, 3);

    PLASMA_TEST_VEC3(r, plVec3T(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator*(mat, vec4)")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const plVec4T r = m * plVec4T(1, 2, 3, 4);

    PLASMA_TEST_VEC4(r,
      plVec4T(1 * 1 + 2 * 2 + 3 * 3 + 4 * 4, 1 * 5 + 2 * 6 + 3 * 7 + 4 * 8, 1 * 9 + 2 * 10 + 3 * 11 + 4 * 12, 1 * 13 + 2 * 14 + 3 * 15 + 4 * 16),
      0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator*(mat, float) | operator*(float, mat)")
  {
    plMat4T m0(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    plMat4T m = m0 * (plMathTestType)2;
    plMat4T m2 = (plMathTestType)2 * m0;

    PLASMA_TEST_VEC4(m.GetRow(0), plVec4T(2, 4, 6, 8), 0.0001f);
    PLASMA_TEST_VEC4(m.GetRow(1), plVec4T(10, 12, 14, 16), 0.0001f);
    PLASMA_TEST_VEC4(m.GetRow(2), plVec4T(18, 20, 22, 24), 0.0001f);
    PLASMA_TEST_VEC4(m.GetRow(3), plVec4T(26, 28, 30, 32), 0.0001f);

    PLASMA_TEST_VEC4(m2.GetRow(0), plVec4T(2, 4, 6, 8), 0.0001f);
    PLASMA_TEST_VEC4(m2.GetRow(1), plVec4T(10, 12, 14, 16), 0.0001f);
    PLASMA_TEST_VEC4(m2.GetRow(2), plVec4T(18, 20, 22, 24), 0.0001f);
    PLASMA_TEST_VEC4(m2.GetRow(3), plVec4T(26, 28, 30, 32), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator/(mat, float)")
  {
    plMat4T m0(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m0 *= (plMathTestType)4;

    plMat4T m = m0 / (plMathTestType)2;

    PLASMA_TEST_VEC4(m.GetRow(0), plVec4T(2, 4, 6, 8), 0.0001f);
    PLASMA_TEST_VEC4(m.GetRow(1), plVec4T(10, 12, 14, 16), 0.0001f);
    PLASMA_TEST_VEC4(m.GetRow(2), plVec4T(18, 20, 22, 24), 0.0001f);
    PLASMA_TEST_VEC4(m.GetRow(3), plVec4T(26, 28, 30, 32), 0.0001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator+(mat, mat) | operator-(mat, mat)")
  {
    plMat4T m0(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    plMat4T m1(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16);

    PLASMA_TEST_BOOL((m0 + m1).IsZero());
    PLASMA_TEST_BOOL((m0 - m1).IsEqual(m0 * (plMathTestType)2, 0.0001f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator== (mat, mat) | operator!= (mat, mat)")
  {
    plMat4T m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    plMat4T m2 = m;

    PLASMA_TEST_BOOL(m == m2);

    m2.m_fElementsCM[0] += 0.00001f;

    PLASMA_TEST_BOOL(m != m2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsNaN")
  {
    if (plMath::SupportsNaN<plMathTestType>())
    {
      plMat4T m;

      m.SetIdentity();
      PLASMA_TEST_BOOL(!m.IsNaN());

      for (plUInt32 i = 0; i < 16; ++i)
      {
        m.SetIdentity();
        m.m_fElementsCM[i] = plMath::NaN<plMathTestType>();

        PLASMA_TEST_BOOL(m.IsNaN());
      }
    }
  }
}
