#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Tracks/Curve1D.h>

plCurve1D::ControlPoint::ControlPoint()
{
  m_Position.SetZero();
  m_LeftTangent.SetZero();
  m_RightTangent.SetZero();
  m_uiOriginalIndex = 0;
}

plCurve1D::plCurve1D()
{
  Clear();
}

void plCurve1D::Clear()
{
  m_fMinX = 0;
  m_fMaxX = 0;
  m_fMinY = 0;
  m_fMaxY = 0;

  m_ControlPoints.Clear();
}

bool plCurve1D::IsEmpty() const
{
  return m_ControlPoints.IsEmpty();
}

plCurve1D::ControlPoint& plCurve1D::AddControlPoint(double x)
{
  auto& cp = m_ControlPoints.ExpandAndGetRef();
  cp.m_uiOriginalIndex = static_cast<plUInt16>(m_ControlPoints.GetCount() - 1);
  cp.m_Position.x = x;
  cp.m_Position.y = 0;
  cp.m_LeftTangent.x = -0.1f;
  cp.m_LeftTangent.y = 0.0f;
  cp.m_RightTangent.x = +0.1f;
  cp.m_RightTangent.y = 0.0f;

  return cp;
}

void plCurve1D::QueryExtents(double& ref_fMinx, double& ref_fMaxx) const
{
  ref_fMinx = m_fMinX;
  ref_fMaxx = m_fMaxX;
}

void plCurve1D::QueryExtremeValues(double& ref_fMinVal, double& ref_fMaxVal) const
{
  ref_fMinVal = m_fMinY;
  ref_fMaxVal = m_fMaxY;
}

plUInt32 plCurve1D::GetNumControlPoints() const
{
  return m_ControlPoints.GetCount();
}

void plCurve1D::SortControlPoints()
{
  m_ControlPoints.Sort();

  RecomputeExtents();
}

plInt32 plCurve1D::FindApproxControlPoint(double x) const
{
  plUInt32 uiLowIdx = 0;
  plUInt32 uiHighIdx = m_LinearApproximation.GetCount();

  // do a binary search to reduce the search space
  while (uiHighIdx - uiLowIdx > 8)
  {
    const plUInt32 uiMidIdx = uiLowIdx + ((uiHighIdx - uiLowIdx) >> 1); // lerp

    // doesn't matter whether to use > or >=
    if (m_LinearApproximation[uiMidIdx].x > x)
      uiHighIdx = uiMidIdx;
    else
      uiLowIdx = uiMidIdx;
  }

  // now do a linear search to find the final item
  for (plUInt32 idx = uiLowIdx; idx < uiHighIdx; ++idx)
  {
    if (m_LinearApproximation[idx].x >= x)
    {
      // when m_LinearApproximation[0].x >= x, we want to return -1
      return ((plInt32)idx) - 1;
    }
  }

  // return last index
  return (plInt32)uiHighIdx - 1;
}

double plCurve1D::Evaluate(double x) const
{
  PL_ASSERT_DEBUG(!m_LinearApproximation.IsEmpty(), "Cannot evaluate curve without precomputing curve approximation data first. Call CreateLinearApproximation() on curve before calling Evaluate().");

  if (m_LinearApproximation.GetCount() >= 2)
  {
    const plUInt32 numCPs = m_LinearApproximation.GetCount();
    const plInt32 iControlPoint = FindApproxControlPoint(x);

    if (iControlPoint == -1)
    {
      // clamp to left value
      return m_LinearApproximation[0].y;
    }
    else if (iControlPoint == numCPs - 1)
    {
      // clamp to right value
      return m_LinearApproximation[numCPs - 1].y;
    }
    else
    {
      const double v1 = m_LinearApproximation[iControlPoint].y;
      const double v2 = m_LinearApproximation[iControlPoint + 1].y;

      // interpolate
      double lerpX = x - m_LinearApproximation[iControlPoint].x;
      const double len = (m_LinearApproximation[iControlPoint + 1].x - m_LinearApproximation[iControlPoint].x);

      if (len <= 0)
        lerpX = 0;
      else
        lerpX /= len; // TODO remove division ?

      return plMath::Lerp(v1, v2, lerpX);
    }
  }
  else if (m_LinearApproximation.GetCount() == 1)
  {
    return m_LinearApproximation[0].y;
  }

  return 0;
}

double plCurve1D::ConvertNormalizedPos(double fPos) const
{
  double fMin, fMax;
  QueryExtents(fMin, fMax);

  return plMath::Lerp(fMin, fMax, fPos);
}


double plCurve1D::NormalizeValue(double value) const
{
  double fMin, fMax;
  QueryExtremeValues(fMin, fMax);

  if (fMin >= fMax)
    return 0;

  return (value - fMin) / (fMax - fMin);
}

plUInt64 plCurve1D::GetHeapMemoryUsage() const
{
  return m_ControlPoints.GetHeapMemoryUsage();
}

void plCurve1D::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = 4;

  inout_stream << uiVersion;

  const plUInt32 numCp = m_ControlPoints.GetCount();

  inout_stream << numCp;

  for (const auto& cp : m_ControlPoints)
  {
    inout_stream << cp.m_Position;
    inout_stream << cp.m_LeftTangent;
    inout_stream << cp.m_RightTangent;
    inout_stream << cp.m_TangentModeRight;
    inout_stream << cp.m_TangentModeLeft;
  }
}

void plCurve1D::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;

  inout_stream >> uiVersion;
  PL_ASSERT_DEV(uiVersion <= 4, "Incorrect version '{0}' for plCurve1D", uiVersion);

  plUInt32 numCp = 0;

  inout_stream >> numCp;

  m_ControlPoints.SetCountUninitialized(numCp);

  if (uiVersion <= 2)
  {
    for (auto& cp : m_ControlPoints)
    {
      plVec2 pos;
      inout_stream >> pos;
      cp.m_Position.Set(pos.x, pos.y);

      if (uiVersion >= 2)
      {
        inout_stream >> cp.m_LeftTangent;
        inout_stream >> cp.m_RightTangent;
      }
    }
  }
  else
  {
    for (auto& cp : m_ControlPoints)
    {
      inout_stream >> cp.m_Position;
      inout_stream >> cp.m_LeftTangent;
      inout_stream >> cp.m_RightTangent;

      if (uiVersion >= 4)
      {
        inout_stream >> cp.m_TangentModeRight;
        inout_stream >> cp.m_TangentModeLeft;
      }
    }
  }
}

void plCurve1D::CreateLinearApproximation(double fMaxError /*= 0.01f*/, plUInt8 uiMaxSubDivs /*= 8*/)
{
  m_LinearApproximation.Clear();

  /// \todo Since we do this, we actually don't need the linear approximation anymore and could just evaluate the full curve
  ApplyTangentModes();

  ClampTangents();

  if (m_ControlPoints.IsEmpty())
  {
    m_LinearApproximation.PushBack(plVec2d::MakeZero());
    return;
  }

  for (plUInt32 i = 1; i < m_ControlPoints.GetCount(); ++i)
  {
    PL_ASSERT_DEBUG(m_ControlPoints[i - 1].m_Position.x <= m_ControlPoints[i].m_Position.x, "Curve control points are not sorted. Call SortControlPoints() before CreateLinearApproximation().");

    double fMinY, fMaxY;
    ApproximateMinMaxValues(m_ControlPoints[i - 1], m_ControlPoints[i], fMinY, fMaxY);

    const double rangeY = plMath::Max(0.1, fMaxY - fMinY);
    const double fMaxErrorY = fMaxError * rangeY;
    const double fMaxErrorX = (m_ControlPoints[i].m_Position.x - m_ControlPoints[i - 1].m_Position.x) * fMaxError;


    m_LinearApproximation.PushBack(m_ControlPoints[i - 1].m_Position);

    ApproximateCurve(m_ControlPoints[i - 1].m_Position,
      m_ControlPoints[i - 1].m_Position + plVec2d(m_ControlPoints[i - 1].m_RightTangent.x, m_ControlPoints[i - 1].m_RightTangent.y),
      m_ControlPoints[i].m_Position + plVec2d(m_ControlPoints[i].m_LeftTangent.x, m_ControlPoints[i].m_LeftTangent.y), m_ControlPoints[i].m_Position,
      fMaxErrorX, fMaxErrorY, uiMaxSubDivs);
  }

  m_LinearApproximation.PushBack(m_ControlPoints.PeekBack().m_Position);

  RecomputeLinearApproxExtremes();
}

void plCurve1D::RecomputeExtents()
{
  m_fMinX = plMath::MaxValue<float>();
  m_fMaxX = -plMath::MaxValue<float>();

  for (const auto& cp : m_ControlPoints)
  {
    m_fMinX = plMath::Min(m_fMinX, cp.m_Position.x);
    m_fMaxX = plMath::Max(m_fMaxX, cp.m_Position.x);

    // ignore X values that could go outside the control point range due to Bplier curve interpolation
    // we just assume the curve is always restricted along X by the CPs

    // m_fMinX = plMath::Min(m_fMinX, cp.m_Position.x + cp.m_LeftTangent.x);
    // m_fMaxX = plMath::Max(m_fMaxX, cp.m_Position.x + cp.m_LeftTangent.x);

    // m_fMinX = plMath::Min(m_fMinX, cp.m_Position.x + cp.m_RightTangent.x);
    // m_fMaxX = plMath::Max(m_fMaxX, cp.m_Position.x + cp.m_RightTangent.x);
  }
}


void plCurve1D::RecomputeLinearApproxExtremes()
{
  m_fMinY = plMath::MaxValue<float>();
  m_fMaxY = -plMath::MaxValue<float>();

  for (const auto& cp : m_LinearApproximation)
  {
    m_fMinY = plMath::Min(m_fMinY, cp.y);
    m_fMaxY = plMath::Max(m_fMaxY, cp.y);
  }
}

void plCurve1D::ApproximateMinMaxValues(const ControlPoint& lhs, const ControlPoint& rhs, double& fMinY, double& fMaxY)
{
  fMinY = plMath::Min(lhs.m_Position.y, rhs.m_Position.y);
  fMaxY = plMath::Max(lhs.m_Position.y, rhs.m_Position.y);

  fMinY = plMath::Min(fMinY, lhs.m_Position.y + lhs.m_RightTangent.y);
  fMaxY = plMath::Max(fMaxY, lhs.m_Position.y + lhs.m_RightTangent.y);

  fMinY = plMath::Min(fMinY, rhs.m_Position.y + rhs.m_LeftTangent.y);
  fMaxY = plMath::Max(fMaxY, rhs.m_Position.y + rhs.m_LeftTangent.y);
}

void plCurve1D::ApproximateCurve(
  const plVec2d& p0, const plVec2d& p1, const plVec2d& p2, const plVec2d& p3, double fMaxErrorX, double fMaxErrorY, plInt32 iSubDivLeft)
{
  const plVec2d cubicCenter = plMath::EvaluateBplierCurve(0.5, p0, p1, p2, p3);

  ApproximateCurvePiece(p0, p1, p2, p3, 0.0f, p0, 0.5, cubicCenter, fMaxErrorX, fMaxErrorY, iSubDivLeft);

  // always insert the center point
  // with an S curve the cubicCenter and the linearCenter can be identical even though the rest of the curve is absolutely not linear
  m_LinearApproximation.PushBack(cubicCenter);

  ApproximateCurvePiece(p0, p1, p2, p3, 0.5, cubicCenter, 1.0, p3, fMaxErrorX, fMaxErrorY, iSubDivLeft);
}

void plCurve1D::ApproximateCurvePiece(const plVec2d& p0, const plVec2d& p1, const plVec2d& p2, const plVec2d& p3, double tLeft, const plVec2d& pLeft,
  double tRight, const plVec2d& pRight, double fMaxErrorX, double fMaxErrorY, plInt32 iSubDivLeft)
{
  // this is a safe guard
  if (iSubDivLeft <= 0)
    return;

  const double tCenter = plMath::Lerp(tLeft, tRight, 0.5);

  const plVec2d cubicCenter = plMath::EvaluateBplierCurve(tCenter, p0, p1, p2, p3);
  const plVec2d linearCenter = plMath::Lerp(pLeft, pRight, 0.5);

  // check whether the linear interpolation between pLeft and pRight would already result in a good enough approximation
  // if not, subdivide the curve further

  const double fThisErrorX = plMath::Abs(cubicCenter.x - linearCenter.x);
  const double fThisErrorY = plMath::Abs(cubicCenter.y - linearCenter.y);

  if (fThisErrorX < fMaxErrorX && fThisErrorY < fMaxErrorY)
    return;

  ApproximateCurvePiece(p0, p1, p2, p3, tLeft, pLeft, tCenter, cubicCenter, fMaxErrorX, fMaxErrorY, iSubDivLeft - 1);

  m_LinearApproximation.PushBack(cubicCenter);

  ApproximateCurvePiece(p0, p1, p2, p3, tCenter, cubicCenter, tRight, pRight, fMaxErrorX, fMaxErrorY, iSubDivLeft - 1);
}

void plCurve1D::ClampTangents()
{
  if (m_ControlPoints.GetCount() < 2)
    return;

  for (plUInt32 i = 1; i < m_ControlPoints.GetCount() - 1; ++i)
  {
    auto& tCP = m_ControlPoints[i];
    const auto& pCP = m_ControlPoints[i - 1];
    const auto& nCP = m_ControlPoints[i + 1];

    plVec2d lpt = tCP.m_Position + plVec2d(tCP.m_LeftTangent.x, tCP.m_LeftTangent.y);
    plVec2d rpt = tCP.m_Position + plVec2d(tCP.m_RightTangent.x, tCP.m_RightTangent.y);

    lpt.x = plMath::Clamp(lpt.x, pCP.m_Position.x, tCP.m_Position.x);
    rpt.x = plMath::Clamp(rpt.x, tCP.m_Position.x, nCP.m_Position.x);

    const plVec2d tangentL = lpt - tCP.m_Position;
    const plVec2d tangentR = rpt - tCP.m_Position;

    tCP.m_LeftTangent.Set((float)tangentL.x, (float)tangentL.y);
    tCP.m_RightTangent.Set((float)tangentR.x, (float)tangentR.y);
  }

  // first CP
  {
    auto& tCP = m_ControlPoints[0];
    const auto& nCP = m_ControlPoints[1];

    plVec2d rpt = tCP.m_Position + plVec2d(tCP.m_RightTangent.x, tCP.m_RightTangent.y);
    rpt.x = plMath::Clamp(rpt.x, tCP.m_Position.x, nCP.m_Position.x);

    const plVec2d tangentR = rpt - tCP.m_Position;
    tCP.m_RightTangent.Set((float)tangentR.x, (float)tangentR.y);
  }

  // last CP
  {
    auto& tCP = m_ControlPoints[m_ControlPoints.GetCount() - 1];
    const auto& pCP = m_ControlPoints[m_ControlPoints.GetCount() - 2];

    plVec2d lpt = tCP.m_Position + plVec2d(tCP.m_LeftTangent.x, tCP.m_LeftTangent.y);
    lpt.x = plMath::Clamp(lpt.x, pCP.m_Position.x, tCP.m_Position.x);

    const plVec2d tangentL = lpt - tCP.m_Position;
    tCP.m_LeftTangent.Set((float)tangentL.x, (float)tangentL.y);
  }
}

void plCurve1D::ApplyTangentModes()
{
  if (m_ControlPoints.GetCount() < 2)
    return;

  for (plUInt32 i = 1; i < m_ControlPoints.GetCount() - 1; ++i)
  {
    const auto& cp = m_ControlPoints[i];

    PL_ASSERT_DEBUG(cp.m_Position.x >= m_ControlPoints[i - 1].m_Position.x, "Curve control points are not sorted. Call SortControlPoints() before CreateLinearApproximation().");
    PL_ASSERT_DEBUG(m_ControlPoints[i + 1].m_Position.x >= cp.m_Position.x, "Curve control points are not sorted. Call SortControlPoints() before CreateLinearApproximation().");

    if (cp.m_TangentModeLeft == plCurveTangentMode::FixedLength)
      MakeFixedLengthTangentLeft(i);
    else if (cp.m_TangentModeLeft == plCurveTangentMode::Linear)
      MakeLinearTangentLeft(i);
    else if (cp.m_TangentModeLeft == plCurveTangentMode::Auto)
      MakeAutoTangentLeft(i);

    if (cp.m_TangentModeRight == plCurveTangentMode::FixedLength)
      MakeFixedLengthTangentRight(i);
    else if (cp.m_TangentModeRight == plCurveTangentMode::Linear)
      MakeLinearTangentRight(i);
    else if (cp.m_TangentModeRight == plCurveTangentMode::Auto)
      MakeAutoTangentRight(i);
  }

  // first CP
  {
    const plUInt32 i = 0;
    const auto& cp = m_ControlPoints[i];

    if (cp.m_TangentModeRight == plCurveTangentMode::FixedLength)
      MakeFixedLengthTangentRight(i);
    else if (cp.m_TangentModeRight == plCurveTangentMode::Linear)
      MakeLinearTangentRight(i);
    else if (cp.m_TangentModeRight == plCurveTangentMode::Auto)
      MakeLinearTangentRight(i); // note: first point will always be linear in auto mode
  }

  // last CP
  {
    const plUInt32 i = m_ControlPoints.GetCount() - 1;
    const auto& cp = m_ControlPoints[i];

    if (cp.m_TangentModeLeft == plCurveTangentMode::FixedLength)
      MakeFixedLengthTangentLeft(i);
    else if (cp.m_TangentModeLeft == plCurveTangentMode::Linear)
      MakeLinearTangentLeft(i);
    else if (cp.m_TangentModeLeft == plCurveTangentMode::Auto)
      MakeLinearTangentLeft(i); // note: last point will always be linear in auto mode
  }
}

void plCurve1D::MakeFixedLengthTangentLeft(plUInt32 uiCpIdx)
{
  auto& tCP = m_ControlPoints[uiCpIdx];
  const auto& pCP = m_ControlPoints[uiCpIdx - 1];

  const double lengthL = (pCP.m_Position.x - tCP.m_Position.x) * 0.3333333333;

  if (lengthL >= -0.0000001)
  {
    tCP.m_LeftTangent.SetZero();
  }
  else
  {
    const double tLen = plMath::Min((double)tCP.m_LeftTangent.x, -0.001);

    const double fNormL = lengthL / tLen;
    tCP.m_LeftTangent.x = (float)lengthL;
    tCP.m_LeftTangent.y *= (float)fNormL;
  }
}

void plCurve1D::MakeFixedLengthTangentRight(plUInt32 uiCpIdx)
{
  auto& tCP = m_ControlPoints[uiCpIdx];
  const auto& nCP = m_ControlPoints[uiCpIdx + 1];

  const double lengthR = (nCP.m_Position.x - tCP.m_Position.x) * 0.3333333333;

  if (lengthR <= 0.0000001)
  {
    tCP.m_RightTangent.SetZero();
  }
  else
  {
    const double tLen = plMath::Max((double)tCP.m_RightTangent.x, 0.001);

    const double fNormR = lengthR / tLen;
    tCP.m_RightTangent.x = (float)lengthR;
    tCP.m_RightTangent.y *= (float)fNormR;
  }
}

void plCurve1D::MakeLinearTangentLeft(plUInt32 uiCpIdx)
{
  auto& tCP = m_ControlPoints[uiCpIdx];
  const auto& pCP = m_ControlPoints[uiCpIdx - 1];

  const plVec2d tangent = (pCP.m_Position - tCP.m_Position) * 0.3333333333;
  tCP.m_LeftTangent.Set((float)tangent.x, (float)tangent.y);
}

void plCurve1D::MakeLinearTangentRight(plUInt32 uiCpIdx)
{
  auto& tCP = m_ControlPoints[uiCpIdx];
  const auto& nCP = m_ControlPoints[uiCpIdx + 1];

  const plVec2d tangent = (nCP.m_Position - tCP.m_Position) * 0.3333333333;
  tCP.m_RightTangent.Set((float)tangent.x, (float)tangent.y);
}

void plCurve1D::MakeAutoTangentLeft(plUInt32 uiCpIdx)
{
  auto& tCP = m_ControlPoints[uiCpIdx];
  const auto& pCP = m_ControlPoints[uiCpIdx - 1];
  const auto& nCP = m_ControlPoints[uiCpIdx + 1];

  const double len = (nCP.m_Position.x - pCP.m_Position.x);
  if (len <= 0)
    return;

  const double fLerpFactor = (tCP.m_Position.x - pCP.m_Position.x) / len;

  const plVec2d dirP = (tCP.m_Position - pCP.m_Position) * 0.3333333333;
  const plVec2d dirN = (nCP.m_Position - tCP.m_Position) * 0.3333333333;

  const plVec2d tangent = plMath::Lerp(dirP, dirN, fLerpFactor);

  tCP.m_LeftTangent.Set(-(float)tangent.x, -(float)tangent.y);
}

void plCurve1D::MakeAutoTangentRight(plUInt32 uiCpIdx)
{
  auto& tCP = m_ControlPoints[uiCpIdx];
  const auto& pCP = m_ControlPoints[uiCpIdx - 1];
  const auto& nCP = m_ControlPoints[uiCpIdx + 1];

  const double len = (nCP.m_Position.x - pCP.m_Position.x);
  if (len <= 0)
    return;

  const double fLerpFactor = (tCP.m_Position.x - pCP.m_Position.x) / len;

  const plVec2d dirP = (tCP.m_Position - pCP.m_Position) * 0.3333333333;
  const plVec2d dirN = (nCP.m_Position - tCP.m_Position) * 0.3333333333;

  const plVec2d tangent = plMath::Lerp(dirP, dirN, fLerpFactor);

  tCP.m_RightTangent.Set((float)tangent.x, (float)tangent.y);
}


