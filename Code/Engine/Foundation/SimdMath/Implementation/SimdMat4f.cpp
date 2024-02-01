#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Mat4.h>
#include <Foundation/SimdMath/SimdMat4f.h>

///\todo optimize

plResult plSimdMat4f::Invert(const plSimdFloat& fEpsilon)
{
  plMat4 tmp;
  GetAsArray(tmp.m_fElementsCM, plMatrixLayout::ColumnMajor);

  if (tmp.Invert(fEpsilon).Failed())
    return PL_FAILURE;

  *this = plSimdMat4f::MakeFromColumnMajorArray(tmp.m_fElementsCM);

  return PL_SUCCESS;
}


