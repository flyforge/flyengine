#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/CurveFunctions.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Reflection/Reflection.h>

// Default are D3D convention before a renderer is initialized.
plClipSpaceDepthRange::Enum plClipSpaceDepthRange::Default = plClipSpaceDepthRange::ZeroToOne;
plClipSpaceYMode::Enum plClipSpaceYMode::RenderToTextureDefault = plClipSpaceYMode::Regular;

plHandedness::Enum plHandedness::Default = plHandedness::LeftHanded;

bool plMath::IsPowerOf(plInt32 value, plInt32 iBase)
{
  if (value == 1)
    return true;

  while (value > iBase)
  {
    if (value % iBase == 0)
      value /= iBase;
    else
      return false;
  }

  return (value == iBase);
}

plUInt32 plMath::PowerOfTwo_Floor(plUInt32 uiNpot)
{
  if (IsPowerOf2(uiNpot))
    return (uiNpot);

  for (plUInt32 i = 1; i <= (sizeof(uiNpot) * 8); ++i)
  {
    uiNpot >>= 1;

    if (uiNpot == 1)
      return (uiNpot << i);
  }

  return (1);
}

plUInt32 plMath::PowerOfTwo_Ceil(plUInt32 uiNpot)
{
  if (IsPowerOf2(uiNpot))
    return (uiNpot);

  for (plUInt32 i = 1; i <= (sizeof(uiNpot) * 8); ++i)
  {
    uiNpot >>= 1;

    if (uiNpot == 1)
    {
      // note: left shift by 32 bits is undefined behavior and typically just returns the left operand unchanged
      // so for npot values larger than 1^31 we do run into this code path, but instead of returning 0, as one may expect, it will usually return 1
      return uiNpot << (i + 1u);
    }
  }

  return (1u);
}


plUInt32 plMath::GreatestCommonDivisor(plUInt32 a, plUInt32 b)
{
  // https://lemire.me/blog/2013/12/26/fastest-way-to-compute-the-greatest-common-divisor/
  if (a == 0)
  {
    return a;
  }
  if (b == 0)
  {
    return b;
  }

  plUInt32 shift = FirstBitLow(a | b);
  a >>= FirstBitLow(a);
  do
  {
    b >>= FirstBitLow(b);
    if (a > b)
    {
      Swap(a, b);
    }
    b = b - a;
  } while (b != 0);
  return a << shift;
}

plResult plMath::TryMultiply32(plUInt32& out_uiResult, plUInt32 a, plUInt32 b, plUInt32 c, plUInt32 d)
{
  plUInt64 result = static_cast<plUInt64>(a) * static_cast<plUInt64>(b);

  if (result > 0xFFFFFFFFllu)
  {
    return PLASMA_FAILURE;
  }

  result *= static_cast<plUInt64>(c);

  if (result > 0xFFFFFFFFllu)
  {
    return PLASMA_FAILURE;
  }

  result *= static_cast<plUInt64>(d);

  if (result > 0xFFFFFFFFllu)
  {
    return PLASMA_FAILURE;
  }

  out_uiResult = static_cast<plUInt32>(result & 0xFFFFFFFFllu);
  return PLASMA_SUCCESS;
}

plUInt32 plMath::SafeMultiply32(plUInt32 a, plUInt32 b, plUInt32 c, plUInt32 d)
{
  plUInt32 result = 0;
  if (TryMultiply32(result, a, b, c, d).Succeeded())
  {
    return result;
  }

  PLASMA_REPORT_FAILURE("Safe multiplication failed: {0} * {1} * {2} * {3} exceeds UInt32 range.", a, b, c, d);
  std::terminate();
}

plResult plMath::TryMultiply64(plUInt64& out_uiResult, plUInt64 a, plUInt64 b, plUInt64 c, plUInt64 d)
{
  if (a == 0 || b == 0 || c == 0 || d == 0)
  {
    out_uiResult = 0;
    return PLASMA_SUCCESS;
  }

#if PLASMA_ENABLED(PLASMA_PLATFORM_ARCH_X86) && PLASMA_ENABLED(PLASMA_PLATFORM_64BIT) && PLASMA_ENABLED(PLASMA_COMPILER_MSVC)

  plUInt64 uiHighBits = 0;

  const plUInt64 ab = _umul128(a, b, &uiHighBits);
  if (uiHighBits != 0)
  {
    return PLASMA_FAILURE;
  }

  const plUInt64 abc = _umul128(ab, c, &uiHighBits);
  if (uiHighBits != 0)
  {
    return PLASMA_FAILURE;
  }

  const plUInt64 abcd = _umul128(abc, d, &uiHighBits);
  if (uiHighBits != 0)
  {
    return PLASMA_FAILURE;
  }

#else
  const plUInt64 ab = a * b;
  const plUInt64 abc = ab * c;
  const plUInt64 abcd = abc * d;

  if (a > 1 && b > 1 && (ab / a != b))
  {
    return PLASMA_FAILURE;
  }

  if (c > 1 && (abc / c != ab))
  {
    return PLASMA_FAILURE;
  }

  if (d > 1 && (abcd / d != abc))
  {
    return PLASMA_FAILURE;
  }

#endif

  out_uiResult = abcd;
  return PLASMA_SUCCESS;
}

plUInt64 plMath::SafeMultiply64(plUInt64 a, plUInt64 b, plUInt64 c, plUInt64 d)
{
  plUInt64 result = 0;
  if (TryMultiply64(result, a, b, c, d).Succeeded())
  {
    return result;
  }

  PLASMA_REPORT_FAILURE("Safe multiplication failed: {0} * {1} * {2} * {3} exceeds plUInt64 range.", a, b, c, d);
  std::terminate();
}

#if PLASMA_ENABLED(PLASMA_PLATFORM_32BIT)
size_t plMath::SafeConvertToSizeT(plUInt64 uiValue)
{
  size_t result = 0;
  if (TryConvertToSizeT(result, uiValue).Succeeded())
  {
    return result;
  }

  PLASMA_REPORT_FAILURE("Given value ({}) can't be converted to size_t because it is too big.", uiValue);
  std::terminate();
}
#endif

void plAngle::NormalizeRange()
{
  const float fTwoPi = 2.0f * Pi<float>();

  const float fTwoPiTen = 10.0f * Pi<float>();

  if (m_fRadian > fTwoPiTen || m_fRadian < -fTwoPiTen)
  {
    m_fRadian = plMath::Mod(m_fRadian, fTwoPi);
  }

  while (m_fRadian >= fTwoPi)
  {
    m_fRadian -= fTwoPi;
  }

  while (m_fRadian < 0.0f)
  {
    m_fRadian += fTwoPi;
  }
}

float plMath::ReplaceNaN(float fValue, float fFallback)
{
  // ATTENTION: if this is a template, inline or constexpr function, the current MSVC (17.6)
  // seems to generate incorrect code and the IsNaN check doesn't detect NaNs.
  // As an out-of-line function it works.

  if (plMath::IsNaN(fValue))
    return fFallback;

  return fValue;
}

double plMath::ReplaceNaN(double fValue, double fFallback)
{
  // ATTENTION: if this is a template, inline or constexpr function, the current MSVC (17.6)
  // seems to generate incorrect code and the IsNaN check doesn't detect NaNs.
  // As an out-of-line function it works.

  if (plMath::IsNaN(fValue))
    return fFallback;

  return fValue;
}

plVec3 plBasisAxis::GetBasisVector(Enum basisAxis)
{
  switch (basisAxis)
  {
    case plBasisAxis::PositiveX:
      return plVec3(1.0f, 0.0f, 0.0f);

    case plBasisAxis::NegativeX:
      return plVec3(-1.0f, 0.0f, 0.0f);

    case plBasisAxis::PositiveY:
      return plVec3(0.0f, 1.0f, 0.0f);

    case plBasisAxis::NegativeY:
      return plVec3(0.0f, -1.0f, 0.0f);

    case plBasisAxis::PositiveZ:
      return plVec3(0.0f, 0.0f, 1.0f);

    case plBasisAxis::NegativeZ:
      return plVec3(0.0f, 0.0f, -1.0f);

    default:
      PLASMA_REPORT_FAILURE("Invalid basis dir {0}", basisAxis);
      return plVec3::MakeZero();
  }
}

plMat3 plBasisAxis::CalculateTransformationMatrix(Enum forwardDir, Enum rightDir, Enum dir, float fUniformScale /*= 1.0f*/, float fScaleX /*= 1.0f*/, float fScaleY /*= 1.0f*/, float fScaleZ /*= 1.0f*/)
{
  plMat3 mResult;
  mResult.SetRow(0, plBasisAxis::GetBasisVector(forwardDir) * fUniformScale * fScaleX);
  mResult.SetRow(1, plBasisAxis::GetBasisVector(rightDir) * fUniformScale * fScaleY);
  mResult.SetRow(2, plBasisAxis::GetBasisVector(dir) * fUniformScale * fScaleZ);

  return mResult;
}


plQuat plBasisAxis::GetBasisRotation_PosX(Enum axis)
{
  plQuat rotAxis;
  switch (axis)
  {
    case plBasisAxis::PositiveX:
      rotAxis.SetIdentity();
      break;
    case plBasisAxis::PositiveY:
      rotAxis = plQuat::MakeFromAxisAndAngle(plVec3(0, 0, 1), plAngle::MakeFromDegree(90));
      break;
    case plBasisAxis::PositiveZ:
      rotAxis = plQuat::MakeFromAxisAndAngle(plVec3(0, 1, 0), plAngle::MakeFromDegree(-90));
      break;
    case plBasisAxis::NegativeX:
      rotAxis = plQuat::MakeFromAxisAndAngle(plVec3(0, 1, 0), plAngle::MakeFromDegree(180));
      break;
    case plBasisAxis::NegativeY:
      rotAxis = plQuat::MakeFromAxisAndAngle(plVec3(0, 0, 1), plAngle::MakeFromDegree(-90));
      break;
    case plBasisAxis::NegativeZ:
      rotAxis = plQuat::MakeFromAxisAndAngle(plVec3(0, 1, 0), plAngle::MakeFromDegree(90));
      break;
  }

  return rotAxis;
}

plQuat plBasisAxis::GetBasisRotation(Enum identity, Enum axis)
{
  plQuat rotId;
  switch (identity)
  {
    case plBasisAxis::PositiveX:
      rotId.SetIdentity();
      break;
    case plBasisAxis::PositiveY:
      rotId = plQuat::MakeFromAxisAndAngle(plVec3(0, 0, 1), plAngle::MakeFromDegree(-90));
      break;
    case plBasisAxis::PositiveZ:
      rotId = plQuat::MakeFromAxisAndAngle(plVec3(0, 1, 0), plAngle::MakeFromDegree(90));
      break;
    case plBasisAxis::NegativeX:
      rotId = plQuat::MakeFromAxisAndAngle(plVec3(0, 1, 0), plAngle::MakeFromDegree(180));
      break;
    case plBasisAxis::NegativeY:
      rotId = plQuat::MakeFromAxisAndAngle(plVec3(0, 0, 1), plAngle::MakeFromDegree(90));
      break;
    case plBasisAxis::NegativeZ:
      rotId = plQuat::MakeFromAxisAndAngle(plVec3(0, 1, 0), plAngle::MakeFromDegree(90));
      break;
  }

  plQuat rotAxis;
  switch (axis)
  {
    case plBasisAxis::PositiveX:
      rotAxis.SetIdentity();
      break;
    case plBasisAxis::PositiveY:
      rotAxis = plQuat::MakeFromAxisAndAngle(plVec3(0, 0, 1), plAngle::MakeFromDegree(90));
      break;
    case plBasisAxis::PositiveZ:
      rotAxis = plQuat::MakeFromAxisAndAngle(plVec3(0, 1, 0), plAngle::MakeFromDegree(-90));
      break;
    case plBasisAxis::NegativeX:
      rotAxis = plQuat::MakeFromAxisAndAngle(plVec3(0, 1, 0), plAngle::MakeFromDegree(180));
      break;
    case plBasisAxis::NegativeY:
      rotAxis = plQuat::MakeFromAxisAndAngle(plVec3(0, 0, 1), plAngle::MakeFromDegree(-90));
      break;
    case plBasisAxis::NegativeZ:
      rotAxis = plQuat::MakeFromAxisAndAngle(plVec3(0, 1, 0), plAngle::MakeFromDegree(90));
      break;
  }

  return rotAxis * rotId;
}

plBasisAxis::Enum plBasisAxis::GetOrthogonalAxis(Enum axis1, Enum axis2, bool bFlip)
{
  const plVec3 a1 = plBasisAxis::GetBasisVector(axis1);
  const plVec3 a2 = plBasisAxis::GetBasisVector(axis2);

  plVec3 c = a1.CrossRH(a2);

  if (bFlip)
    c = -c;

  if (c.IsEqual(plVec3::MakeAxisX(), 0.01f))
    return plBasisAxis::PositiveX;
  if (c.IsEqual(-plVec3::MakeAxisX(), 0.01f))
    return plBasisAxis::NegativeX;

  if (c.IsEqual(plVec3::MakeAxisY(), 0.01f))
    return plBasisAxis::PositiveY;
  if (c.IsEqual(-plVec3::MakeAxisY(), 0.01f))
    return plBasisAxis::NegativeY;

  if (c.IsEqual(plVec3::MakeAxisZ(), 0.01f))
    return plBasisAxis::PositiveZ;
  if (c.IsEqual(-plVec3::MakeAxisZ(), 0.01f))
    return plBasisAxis::NegativeZ;

  return axis1;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plComparisonOperator, 1)
  PLASMA_ENUM_CONSTANTS(plComparisonOperator::Equal, plComparisonOperator::NotEqual)
  PLASMA_ENUM_CONSTANTS(plComparisonOperator::Less, plComparisonOperator::LessEqual)
  PLASMA_ENUM_CONSTANTS(plComparisonOperator::Greater, plComparisonOperator::GreaterEqual)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plCurveFunction, 1)
 PLASMA_ENUM_CONSTANT(plCurveFunction::Linear),
 PLASMA_ENUM_CONSTANT(plCurveFunction::ConstantZero),
 PLASMA_ENUM_CONSTANT(plCurveFunction::ConstantOne),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseInSine),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseOutSine),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseInOutSine),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseInQuad),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseOutQuad),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseInOutQuad),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseInCubic),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseOutCubic),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseInOutCubic),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseInQuartic),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseOutQuartic),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseInOutQuartic),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseInQuintic),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseOutQuintic),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseInOutQuintic),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseInExpo),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseOutExpo),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseInOutExpo),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseInCirc),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseOutCirc),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseInOutCirc),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseInBack),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseOutBack),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseInOutBack),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseInElastic),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseOutElastic),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseInOutElastic),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseInBounce),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseOutBounce),
 PLASMA_ENUM_CONSTANT(plCurveFunction::EaseInOutBounce),
 PLASMA_ENUM_CONSTANT(plCurveFunction::Conical),
 PLASMA_ENUM_CONSTANT(plCurveFunction::FadeInHoldFadeOut),
 PLASMA_ENUM_CONSTANT(plCurveFunction::FadeInFadeOut),
 PLASMA_ENUM_CONSTANT(plCurveFunction::Bell),
PLASMA_END_STATIC_REFLECTED_ENUM;
// clang-format on

PLASMA_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Math);
