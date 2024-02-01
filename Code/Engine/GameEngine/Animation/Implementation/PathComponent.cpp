#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/HierarchyChangedMessages.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/PathComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

// TODO PathComponent:
// tangent mode for each node (auto, linear, Bplier)
// linked tangents on/off
// editing tangents (in a plane, needs new manipulator)


// clang-format off
PL_BEGIN_STATIC_REFLECTED_BITFLAGS(plPathComponentFlags, 1)
  PL_BITFLAGS_CONSTANTS(plPathComponentFlags::VisualizePath, plPathComponentFlags::VisualizeUpDir)
PL_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

// clang-format off
PL_IMPLEMENT_MESSAGE_TYPE(plMsgPathChanged);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgPathChanged, 1, plRTTIDefaultAllocator<plMsgPathChanged>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult plPathComponent::ControlPoint::Serialize(plStreamWriter& s) const
{
  s << m_vPosition;
  s << m_vTangentIn;
  s << m_vTangentOut;
  s << m_Roll;

  return PL_SUCCESS;
}

plResult plPathComponent::ControlPoint::Deserialize(plStreamReader& s)
{
  s >> m_vPosition;
  s >> m_vTangentIn;
  s >> m_vTangentOut;
  s >> m_Roll;

  return PL_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plPathComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_BITFLAGS_ACCESSOR_PROPERTY("Flags", plPathComponentFlags, GetPathFlags, SetPathFlags)->AddAttributes(new plDefaultValueAttribute(plPathComponentFlags::VisualizePath)),
    PL_ACCESSOR_PROPERTY("Closed", GetClosed,SetClosed),
    PL_ACCESSOR_PROPERTY("Detail", GetLinearizationError, SetLinearizationError)->AddAttributes(new plDefaultValueAttribute(0.01f), new plClampValueAttribute(0.001f, 1.0f)),
    PL_ARRAY_ACCESSOR_PROPERTY("Nodes", Nodes_GetCount, Nodes_GetNode, Nodes_SetNode, Nodes_Insert, Nodes_Remove),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgPathChanged, OnMsgPathChanged),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Animation/Paths"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plPathComponent::plPathComponent() = default;
plPathComponent::~plPathComponent() = default;

void plPathComponent::SerializeComponent(plWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);

  auto& s = ref_stream.GetStream();
  s << m_PathFlags;
  s << m_bClosed;
  s << m_fLinearizationError;

  if (m_bControlPointsChanged && !m_bDisableControlPointUpdates)
  {
    plDynamicArray<ControlPoint> controlPoints;
    FindControlPoints(controlPoints);
    ref_stream.GetStream().WriteArray(controlPoints).AssertSuccess();
  }
  else
  {
    ref_stream.GetStream().WriteArray(m_ControlPointRepresentation).AssertSuccess();
  }
}

void plPathComponent::DeserializeComponent(plWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);

  auto& s = ref_stream.GetStream();
  s >> m_PathFlags;
  s >> m_bClosed;
  s >> m_fLinearizationError;

  ref_stream.GetStream().ReadArray(m_ControlPointRepresentation).AssertSuccess();

  m_bDisableControlPointUpdates = true;
  m_bControlPointsChanged = false;
  m_bLinearizedRepresentationChanged = true;
}

void plPathComponent::SetClosed(bool bClosed)
{
  if (m_bClosed == bClosed)
    return;

  m_bClosed = bClosed;
  m_bControlPointsChanged = true;
  m_bLinearizedRepresentationChanged = true;
}

void plPathComponent::SetPathFlags(plBitflags<plPathComponentFlags> flags)
{
  if (m_PathFlags == flags)
    return;

  m_PathFlags = flags;

  if (IsActiveAndInitialized())
  {
    if (m_PathFlags.IsNoFlagSet())
      static_cast<plPathComponentManager*>(GetOwningManager())->SetEnableUpdate(this, false);
    else
      static_cast<plPathComponentManager*>(GetOwningManager())->SetEnableUpdate(this, true);
  }
}

void plPathComponent::Nodes_SetNode(plUInt32 i, const plString& node)
{
  m_Nodes[i] = node;
  m_bControlPointsChanged = true;
  m_bLinearizedRepresentationChanged = true;
}

void plPathComponent::Nodes_Insert(plUInt32 uiIndex, const plString& node)
{
  m_Nodes.Insert(node, uiIndex);
  m_bControlPointsChanged = true;
  m_bLinearizedRepresentationChanged = true;
}

void plPathComponent::Nodes_Remove(plUInt32 uiIndex)
{
  m_Nodes.RemoveAtAndCopy(uiIndex);
  m_bControlPointsChanged = true;
  m_bLinearizedRepresentationChanged = true;
}

void plPathComponent::FindControlPoints(plDynamicArray<ControlPoint>& out_ControlPoints) const
{
  auto& points = out_ControlPoints;

  points.Clear();

  if (m_Nodes.GetCount() <= 1)
    return;

  plGameObject* pOwner = const_cast<plGameObject*>(GetOwner());
  const plTransform invTrans = pOwner->GetGlobalTransform().GetInverse();

  plHybridArray<plPathNodeTangentMode::StorageType, 64> tangentsIn;
  plHybridArray<plPathNodeTangentMode::StorageType, 64> tangentsOut;

  for (const plString& sNode : m_Nodes)
  {
    const plGameObject* pNodeObj = pOwner->FindChildByName(plTempHashedString(sNode), false);
    if (pNodeObj == nullptr)
      continue;

    const plPathNodeComponent* pNodeComp;
    if (!pNodeObj->TryGetComponentOfBaseType(pNodeComp))
      continue;

    auto& cp = points.ExpandAndGetRef();
    cp.m_vPosition = invTrans * pNodeObj->GetGlobalPosition();
    cp.m_Roll = pNodeComp->GetRoll();

    tangentsOut.PushBack(pNodeComp->GetTangentMode1().GetValue());
    tangentsIn.PushBack(pNodeComp->GetTangentMode2().GetValue());
  }

  const plUInt32 uiNumPoints = points.GetCount();
  const plUInt32 uiLastIdx = points.GetCount() - 1;

  if (uiNumPoints <= 1)
  {
    points.Clear();
    return;
  }

  plUInt32 uiNumTangentsToUpdate = uiNumPoints;
  plUInt32 uiPrevIdx = uiLastIdx - 1;
  plUInt32 uiCurIdx = uiLastIdx;
  plUInt32 uiNextIdx = 0;

  if (!m_bClosed)
  {
    const plVec3 vStartTangent = (points[1].m_vPosition - points[0].m_vPosition) * 0.3333333333f;
    const plVec3 vEndTangent = (points[uiLastIdx].m_vPosition - points[uiLastIdx - 1].m_vPosition) * 0.3333333333f;

    points[0].m_vTangentIn = vStartTangent;
    points[0].m_vTangentOut = -vStartTangent;

    points[uiLastIdx].m_vTangentIn = vEndTangent;
    points[uiLastIdx].m_vTangentOut = -vEndTangent;

    uiNumTangentsToUpdate = uiNumPoints - 2;
    uiPrevIdx = 0;
    uiCurIdx = 1;
    uiNextIdx = 2;
  }

  for (plUInt32 i = 0; i < uiNumTangentsToUpdate; ++i)
  {
    auto& tCP = points[uiCurIdx];
    const auto& pCP = points[uiPrevIdx];
    const auto& nCP = points[uiNextIdx];

    const float fLength = plMath::Max(0.001f, (nCP.m_vPosition - pCP.m_vPosition).GetLength());
    const float fLerpFactor = plMath::Min(1.0f, (tCP.m_vPosition - pCP.m_vPosition).GetLength() / fLength);

    const plVec3 dirP = (tCP.m_vPosition - pCP.m_vPosition) * 0.3333333333f;
    const plVec3 dirN = (nCP.m_vPosition - tCP.m_vPosition) * 0.3333333333f;

    const plVec3 tangent = plMath::Lerp(dirP, dirN, fLerpFactor);

    switch (tangentsIn[uiCurIdx])
    {
      case plPathNodeTangentMode::Auto:
        tCP.m_vTangentIn = tangent;
        break;
      case plPathNodeTangentMode::Linear:
        tCP.m_vTangentIn = dirN;
        break;
    }

    switch (tangentsOut[uiCurIdx])
    {
      case plPathNodeTangentMode::Auto:
        tCP.m_vTangentOut = -tangent;
        break;
      case plPathNodeTangentMode::Linear:
        tCP.m_vTangentOut = -dirP;
        break;
    }

    uiPrevIdx = uiCurIdx;
    uiCurIdx = uiNextIdx;
    ++uiNextIdx;
  }
}

void plPathComponent::EnsureControlPointRepresentationIsUpToDate()
{
  if (!m_bControlPointsChanged || m_bDisableControlPointUpdates)
    return;

  m_ControlPointRepresentation.Clear();

  if (!IsActive())
    return;

  FindControlPoints(m_ControlPointRepresentation);
  m_bControlPointsChanged = false;
}

void plPathComponent::EnsureLinearizedRepresentationIsUpToDate()
{
  if (!m_bLinearizedRepresentationChanged)
    return;

  m_LinearizedRepresentation.Clear();

  if (!IsActive())
    return;

  EnsureControlPointRepresentationIsUpToDate();

  CreateLinearizedPathRepresentation(m_ControlPointRepresentation);
  m_bLinearizedRepresentationChanged = false;
}

void plPathComponent::OnMsgPathChanged(plMsgPathChanged& ref_msg)
{
  m_bControlPointsChanged = true;
  m_bLinearizedRepresentationChanged = true;
}

void plPathComponent::DrawDebugVisualizations()
{
  if (m_PathFlags.AreNoneSet(plPathComponentFlags::VisualizePath | plPathComponentFlags::VisualizeUpDir))
    return;

  const bool bVisPath = m_PathFlags.IsSet(plPathComponentFlags::VisualizePath);
  const bool bVisUp = m_PathFlags.IsSet(plPathComponentFlags::VisualizeUpDir);

  EnsureLinearizedRepresentationIsUpToDate();

  if (m_LinearizedRepresentation.IsEmpty())
    return;

  plHybridArray<plDebugRenderer::Line, 32> lines;

  plUInt32 uiPrev = 0;
  plUInt32 uiNext = 1;

  for (; uiNext < m_LinearizedRepresentation.GetCount(); ++uiNext)
  {
    const auto& n0 = m_LinearizedRepresentation[uiPrev];
    const auto& n1 = m_LinearizedRepresentation[uiNext];

    if (bVisPath)
    {
      auto& line = lines.ExpandAndGetRef();
      line.m_start = n0.m_vPosition;
      line.m_end = n1.m_vPosition;
      line.m_startColor = plColor::DarkRed;
      line.m_endColor = plColor::DarkRed;
    }

    if (bVisUp)
    {
      auto& line = lines.ExpandAndGetRef();
      line.m_start = n0.m_vPosition;
      line.m_end = n0.m_vPosition + n0.m_vUpDirection * 0.25f;
      line.m_startColor = plColor::Black;
      line.m_endColor = plColor::LightBlue;
    }

    uiPrev = uiNext;
  }

  plDebugRenderer::DrawLines(GetWorld(), lines, plColor::White, GetOwner()->GetGlobalTransform());
}

void plPathComponent::OnActivated()
{
  SUPER::OnActivated();

  if (m_PathFlags.IsAnyFlagSet())
  {
    static_cast<plPathComponentManager*>(GetOwningManager())->SetEnableUpdate(this, true);
  }
}

void plPathComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  if (m_PathFlags.IsAnyFlagSet())
  {
    // assume that if no flag is set, update is already disabled
    static_cast<plPathComponentManager*>(GetOwningManager())->SetEnableUpdate(this, false);
  }
}

void plPathComponent::LinearSampler::SetToStart()
{
  m_fSegmentFraction = 0.0f;
  m_uiSegmentNode = 0;
}

void plPathComponent::SetLinearSamplerTo(LinearSampler& ref_sampler, float fDistance) const
{
  if (fDistance < 0.0f && m_LinearizedRepresentation.GetCount() >= 2)
  {
    ref_sampler.m_uiSegmentNode = m_LinearizedRepresentation.GetCount() - 1;
    ref_sampler.m_fSegmentFraction = 1.0f;
  }
  else
  {
    ref_sampler.m_uiSegmentNode = 0;
    ref_sampler.m_fSegmentFraction = 0.0f;
  }

  AdvanceLinearSamplerBy(ref_sampler, fDistance);
}

bool plPathComponent::AdvanceLinearSamplerBy(LinearSampler& ref_sampler, float& inout_fAddDistance) const
{
  if (inout_fAddDistance == 0.0f || m_LinearizedRepresentation.IsEmpty())
  {
    inout_fAddDistance = 0.0f;
    return false;
  }

  if (m_LinearizedRepresentation.GetCount() == 1)
  {
    ref_sampler.SetToStart();
    inout_fAddDistance = 0.0f;
    return false;
  }

  if (inout_fAddDistance >= 0)
  {
    for (plUInt32 i = ref_sampler.m_uiSegmentNode + 1; i < m_LinearizedRepresentation.GetCount(); ++i)
    {
      const auto& nd0 = m_LinearizedRepresentation[i - 1];
      const auto& nd1 = m_LinearizedRepresentation[i];

      const float fSegmentLength = (nd1.m_vPosition - nd0.m_vPosition).GetLength();
      const float fSegmentDistance = ref_sampler.m_fSegmentFraction * fSegmentLength;
      const float fRemainingSegmentDistance = fSegmentLength - fSegmentDistance;

      if (inout_fAddDistance >= fRemainingSegmentDistance)
      {
        inout_fAddDistance -= fRemainingSegmentDistance;
        ref_sampler.m_uiSegmentNode = i;
        ref_sampler.m_fSegmentFraction = 0.0f;
      }
      else
      {
        ref_sampler.m_fSegmentFraction = (fSegmentDistance + inout_fAddDistance) / fSegmentLength;
        return true;
      }
    }

    ref_sampler.m_uiSegmentNode = m_LinearizedRepresentation.GetCount() - 1;
    ref_sampler.m_fSegmentFraction = 1.0f;
    return false;
  }
  else
  {
    while (true)
    {
      plUInt32 ic = ref_sampler.m_uiSegmentNode;
      plUInt32 in = plMath::Min(ref_sampler.m_uiSegmentNode + 1, m_LinearizedRepresentation.GetCount() - 1);

      const auto& nd0 = m_LinearizedRepresentation[ic];
      const auto& nd1 = m_LinearizedRepresentation[in];

      const float fSegmentLength = (nd1.m_vPosition - nd0.m_vPosition).GetLength();
      const float fSegmentDistance = ref_sampler.m_fSegmentFraction * fSegmentLength;
      const float fRemainingSegmentDistance = -fSegmentDistance;

      if (inout_fAddDistance <= fRemainingSegmentDistance)
      {
        inout_fAddDistance -= fRemainingSegmentDistance;

        if (ref_sampler.m_uiSegmentNode == 0)
        {
          ref_sampler.m_uiSegmentNode = 0;
          ref_sampler.m_fSegmentFraction = 0.0f;
          return false;
        }

        ref_sampler.m_uiSegmentNode--;
        ref_sampler.m_fSegmentFraction = 1.0f;
      }
      else
      {
        ref_sampler.m_fSegmentFraction = (fSegmentDistance + inout_fAddDistance) / fSegmentLength;
        return true;
      }
    }
  }
}

plPathComponent::LinearizedElement plPathComponent::SampleLinearizedRepresentation(const LinearSampler& sampler) const
{
  if (m_LinearizedRepresentation.IsEmpty())
    return {};

  if (sampler.m_uiSegmentNode + 1 >= m_LinearizedRepresentation.GetCount())
  {
    const plUInt32 idx = m_LinearizedRepresentation.GetCount() - 1;

    return m_LinearizedRepresentation[idx];
  }

  const auto& nd0 = m_LinearizedRepresentation[sampler.m_uiSegmentNode];
  const auto& nd1 = m_LinearizedRepresentation[sampler.m_uiSegmentNode + 1];

  LinearizedElement res;
  res.m_vPosition = plMath::Lerp(nd0.m_vPosition, nd1.m_vPosition, sampler.m_fSegmentFraction);
  res.m_vUpDirection = plMath::Lerp(nd0.m_vUpDirection, nd1.m_vUpDirection, sampler.m_fSegmentFraction);

  return res;
}

void plPathComponent::SetLinearizationError(float fError)
{
  if (m_fLinearizationError == fError)
    return;

  m_fLinearizationError = fError;
  m_bLinearizedRepresentationChanged = true;
}

static void ComputeCpDirs(const plDynamicArray<plPathComponent::ControlPoint>& points, bool bClosed, const plCoordinateSystem& cs, plDynamicArray<plVec3>& inout_cpFwd, plDynamicArray<plVec3>& inout_cpUp)
{
  const plUInt32 uiNumCPs = points.GetCount();
  inout_cpFwd.SetCount(uiNumCPs);
  inout_cpUp.SetCount(uiNumCPs);

  for (plUInt32 uiCurPt = 0; uiCurPt < uiNumCPs; ++uiCurPt)
  {
    plUInt32 uiPrevPt;
    plUInt32 uiNextPt = uiCurPt + 1;

    if (bClosed)
    {
      if (uiCurPt == 0)
        uiPrevPt = uiNumCPs - 1;
      else
        uiPrevPt = uiCurPt - 1;

      uiNextPt %= uiNumCPs;
    }
    else
    {
      if (uiCurPt == 0)
        uiPrevPt = 0;
      else
        uiPrevPt = uiCurPt - 1;

      uiNextPt = plMath::Min(uiCurPt + 1, uiNumCPs - 1);
    }

    const auto& cpP = points[uiPrevPt];
    const auto& cpC = points[uiCurPt];
    const auto& cpN = points[uiNextPt];

    const plVec3 posPrev = plMath::EvaluateBplierCurve(0.98f, cpP.m_vPosition, cpP.m_vPosition + cpP.m_vTangentIn, cpC.m_vPosition + cpC.m_vTangentOut, cpC.m_vPosition);
    const plVec3 posNext = plMath::EvaluateBplierCurve(0.02f, cpC.m_vPosition, cpC.m_vPosition + cpC.m_vTangentIn, cpN.m_vPosition + cpN.m_vTangentOut, cpN.m_vPosition);

    plVec3 dirP = (posPrev - cpC.m_vPosition);
    plVec3 dirN = (posNext - cpC.m_vPosition);
    dirP.NormalizeIfNotZero(plVec3::MakeZero()).IgnoreResult();
    dirN.NormalizeIfNotZero(plVec3::MakeZero()).IgnoreResult();

    plVec3 dirAvg = dirP - dirN;
    dirAvg.NormalizeIfNotZero(cs.m_vForwardDir).IgnoreResult();

    plVec3 dirUp = cs.m_vUpDir;
    dirUp.MakeOrthogonalTo(dirAvg);

    dirUp.NormalizeIfNotZero(cs.m_vUpDir).IgnoreResult();

    inout_cpFwd[uiCurPt] = dirAvg;
    inout_cpUp[uiCurPt] = dirUp;
  }
}

static double ComputePathLength(plArrayPtr<plPathComponent::LinearizedElement> points)
{
  double fLength = 0;
  for (plUInt32 i = 1; i < points.GetCount(); ++i)
  {
    fLength += (points[i - 1].m_vPosition - points[i].m_vPosition).GetLength();
  }

  return fLength;
}

static plVec3 ComputeTangentAt(float fT, const plPathComponent::ControlPoint& cp0, const plPathComponent::ControlPoint& cp1)
{
  const plVec3 posPrev = plMath::EvaluateBplierCurve(plMath::Max(0.0f, fT - 0.02f), cp0.m_vPosition, cp0.m_vPosition + cp0.m_vTangentIn, cp1.m_vPosition + cp1.m_vTangentOut, cp1.m_vPosition);
  const plVec3 posNext = plMath::EvaluateBplierCurve(plMath::Min(1.0f, fT + 0.02f), cp0.m_vPosition, cp0.m_vPosition + cp0.m_vTangentIn, cp1.m_vPosition + cp1.m_vTangentOut, cp1.m_vPosition);

  return (posNext - posPrev).GetNormalized();
}

static void InsertHalfPoint(plDynamicArray<plPathComponent::LinearizedElement>& ref_result, plDynamicArray<plVec3>& ref_tangents, const plPathComponent::ControlPoint& cp0, const plPathComponent::ControlPoint& cp1, float fLowerT, float fUpperT, const plVec3& vLowerPos, const plVec3& vUpperPos, float fDistSqr, plInt32 iMinSteps, plInt32 iMaxSteps)
{
  const float fHalfT = plMath::Lerp(fLowerT, fUpperT, 0.5f);

  const plVec3 vHalfPos = plMath::EvaluateBplierCurve(fHalfT, cp0.m_vPosition, cp0.m_vPosition + cp0.m_vTangentIn, cp1.m_vPosition + cp1.m_vTangentOut, cp1.m_vPosition);

  if (iMinSteps <= 0)
  {
    const plVec3 vInterpPos = plMath::Lerp(vLowerPos, vUpperPos, 0.5f);

    if ((vHalfPos - vInterpPos).GetLengthSquared() < fDistSqr)
    {
      return;
    }
  }

  if (iMaxSteps > 0)
  {
    InsertHalfPoint(ref_result, ref_tangents, cp0, cp1, fLowerT, fHalfT, vLowerPos, vHalfPos, fDistSqr, iMinSteps - 1, iMaxSteps - 1);
  }

  ref_result.ExpandAndGetRef().m_vPosition = vHalfPos;
  ref_tangents.ExpandAndGetRef() = ComputeTangentAt(fHalfT, cp0, cp1);

  if (iMaxSteps > 0)
  {
    InsertHalfPoint(ref_result, ref_tangents, cp0, cp1, fHalfT, fUpperT, vHalfPos, vUpperPos, fDistSqr, iMinSteps - 1, iMaxSteps - 1);
  }
}

static void GeneratePathSegment(plUInt32 uiCp0, plUInt32 uiCp1, plArrayPtr<const plPathComponent::ControlPoint> points, plArrayPtr<plVec3> cpUp, plArrayPtr<plVec3> cpFwd, plDynamicArray<plPathComponent::LinearizedElement>& ref_result, plDynamicArray<plVec3>& ref_tangents, float fDistSqr)
{
  ref_tangents.Clear();

  const auto& cp0 = points[uiCp0];
  const auto& cp1 = points[uiCp1];

  plInt32 iRollDiv = 0;
  float fToRoll = plMath::Abs((cp1.m_Roll - cp0.m_Roll).GetDegree());
  while (fToRoll > 45.0f)
  {
    fToRoll *= 0.5f;
    iRollDiv++;
  }

  ref_result.ExpandAndGetRef().m_vPosition = cp0.m_vPosition;
  ref_tangents.ExpandAndGetRef() = -cpFwd[uiCp0];

  InsertHalfPoint(ref_result, ref_tangents, cp0, cp1, 0.0f, 1.0f, cp0.m_vPosition, cp1.m_vPosition, fDistSqr, plMath::Max(1, iRollDiv), 7);

  ref_result.ExpandAndGetRef().m_vPosition = cp1.m_vPosition;
  ref_tangents.ExpandAndGetRef() = -cpFwd[uiCp1];
}

static void ComputeSegmentUpVector(plArrayPtr<plPathComponent::LinearizedElement> segmentElements, plUInt32 uiCp0, plUInt32 uiCp1, const plArrayPtr<const plPathComponent::ControlPoint> points, const plArrayPtr<const plVec3> cpUp, const plArrayPtr<const plVec3> tangents, const plVec3& vWorldUp)
{
  const auto& cp0 = points[uiCp0];
  const auto& cp1 = points[uiCp1];

  const plVec3 cp0up = cpUp[uiCp0];
  const plVec3 cp1up = cpUp[uiCp1];

  const double fSegmentLength = ComputePathLength(segmentElements);

  if (fSegmentLength <= 0.00001f)
  {
    for (plUInt32 t = 0; t < segmentElements.GetCount(); ++t)
    {
      segmentElements[t].m_vUpDirection = cp1up;
    }

    return;
  }

  const double fInvSegmentLength = 1.0 / fSegmentLength;

  double fCurDist = 0.0;
  plVec3 vPrevPos = segmentElements[0].m_vPosition;


  for (plUInt32 t = 0; t < segmentElements.GetCount(); ++t)
  {
    fCurDist += (segmentElements[t].m_vPosition - vPrevPos).GetLength();
    vPrevPos = segmentElements[t].m_vPosition;

    const float fLerpFactor = (float)(fCurDist * fInvSegmentLength);

    const plAngle roll = plMath::Lerp(cp0.m_Roll, cp1.m_Roll, fLerpFactor);

    plQuat qRoll = plQuat::MakeFromAxisAndAngle(tangents[t], roll);

    plVec3 vLocalUp = plMath::Lerp(cp0up, cp1up, fLerpFactor);
    vLocalUp.NormalizeIfNotZero(vWorldUp).IgnoreResult();

    segmentElements[t].m_vUpDirection = qRoll * vLocalUp;
  }
}

void plPathComponent::CreateLinearizedPathRepresentation(const plDynamicArray<ControlPoint>& points)
{
  m_LinearizedRepresentation.Clear();

  const plUInt32 uiNumCPs = points.GetCount();

  if (uiNumCPs <= 1)
    return;

  plHybridArray<plVec3, 64> cpUp;
  plHybridArray<plVec3, 64> cpFwd;

  plCoordinateSystem cs;
  GetWorld()->GetCoordinateSystem(GetOwner()->GetGlobalPosition(), cs);

  ComputeCpDirs(points, m_bClosed, cs, cpFwd, cpUp);

  const plUInt32 uiNumCPsToUse = m_bClosed ? uiNumCPs + 1 : uiNumCPs;

  plHybridArray<plVec3, 64> tangents;

  for (plUInt32 uiCurPt = 1; uiCurPt < uiNumCPsToUse; ++uiCurPt)
  {
    const plUInt32 uiCp0 = uiCurPt - 1;
    const plUInt32 uiCp1 = uiCurPt % uiNumCPs;

    const plUInt32 uiFirstNewNode = m_LinearizedRepresentation.GetCount();
    GeneratePathSegment(uiCp0, uiCp1, points, cpUp, cpFwd, m_LinearizedRepresentation, tangents, plMath::Square(m_fLinearizationError));

    plArrayPtr<plPathComponent::LinearizedElement> segmentElements = m_LinearizedRepresentation.GetArrayPtr().GetSubArray(uiFirstNewNode);

    ComputeSegmentUpVector(segmentElements, uiCp0, uiCp1, points, cpUp, tangents, cs.m_vUpDir);
  }

  m_fLinearizedLength = (float)ComputePathLength(m_LinearizedRepresentation);
}



//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plPathNodeTangentMode, 1)
  PL_ENUM_CONSTANTS(plPathNodeTangentMode::Auto, plPathNodeTangentMode::Linear)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_COMPONENT_TYPE(plPathNodeComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Roll", GetRoll, SetRoll),
    PL_ENUM_ACCESSOR_PROPERTY("Tangent1", plPathNodeTangentMode, GetTangentMode1, SetTangentMode1),
    PL_ENUM_ACCESSOR_PROPERTY("Tangent2", plPathNodeTangentMode, GetTangentMode2, SetTangentMode2),
  }
  PL_END_PROPERTIES;

  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgTransformChanged, OnMsgTransformChanged),
    PL_MESSAGE_HANDLER(plMsgParentChanged, OnMsgParentChanged),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Animation/Paths"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plPathNodeComponent::plPathNodeComponent() = default;
plPathNodeComponent::~plPathNodeComponent() = default;

void plPathNodeComponent::SetRoll(plAngle roll)
{
  if (m_Roll != roll)
  {
    m_Roll = roll;
    PathChanged();
  }
}

void plPathNodeComponent::SetTangentMode1(plEnum<plPathNodeTangentMode> mode)
{
  if (m_TangentMode1 != mode)
  {
    m_TangentMode1 = mode;
    PathChanged();
  }
}

void plPathNodeComponent::SetTangentMode2(plEnum<plPathNodeTangentMode> mode)
{
  if (m_TangentMode2 != mode)
  {
    m_TangentMode2 = mode;
    PathChanged();
  }
}

void plPathNodeComponent::OnMsgTransformChanged(plMsgTransformChanged& msg)
{
  PathChanged();
}

void plPathNodeComponent::OnMsgParentChanged(plMsgParentChanged& msg)
{
  if (msg.m_Type == plMsgParentChanged::Type::ParentUnlinked)
  {
    plGameObject* pOldParent = nullptr;
    if (GetWorld()->TryGetObject(msg.m_hParent, pOldParent))
    {
      plMsgPathChanged msg2;
      pOldParent->SendEventMessage(msg2, this);
    }
  }
  else
  {
    PathChanged();
  }
}

void plPathNodeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->EnableStaticTransformChangesNotifications();
  GetOwner()->EnableParentChangesNotifications();

  PathChanged();
}

void plPathNodeComponent::OnDeactivated()
{
  plMsgPathChanged msg2;
  GetOwner()->SendEventMessage(msg2, this);

  SUPER::OnDeactivated();
}

void plPathNodeComponent::PathChanged()
{
  if (!IsActiveAndInitialized())
    return;

  plMsgPathChanged msg2;
  GetOwner()->SendEventMessage(msg2, this);
}

//////////////////////////////////////////////////////////////////////////

plPathComponentManager::plPathComponentManager(plWorld* pWorld)
  : plComponentManager(pWorld)
{
}

void plPathComponentManager::SetEnableUpdate(plPathComponent* pThis, bool bEnable)
{
  if (bEnable)
  {
    if (!m_NeedUpdate.Contains(pThis))
      m_NeedUpdate.PushBack(pThis);
  }
  else
  {
    m_NeedUpdate.RemoveAndSwap(pThis);
  }
}

void plPathComponentManager::Initialize()
{
  auto desc = plWorldModule::UpdateFunctionDesc(plWorldModule::UpdateFunction(&plPathComponentManager::Update, this), "plPathComponentManager::Update");
  desc.m_bOnlyUpdateWhenSimulating = false;
  desc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PostTransform;

  this->RegisterUpdateFunction(desc);
}

void plPathComponentManager::Update(const plWorldModule::UpdateContext& context)
{
  for (plPathComponent* pComponent : m_NeedUpdate)
  {
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->DrawDebugVisualizations();
    }
  }
}


PL_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_PathComponent);

