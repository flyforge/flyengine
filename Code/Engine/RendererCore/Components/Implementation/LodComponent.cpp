#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Components/LodComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

float CalculateSphereScreenSpaceCoverage(const plBoundingSphere& sphere, const plCamera& camera)
{
  if (camera.IsPerspective())
  {
    return plGraphicsUtils::CalculateSphereScreenCoverage(sphere, camera.GetCenterPosition(), camera.GetFovY(1.0f));
  }
  else
  {
    return plGraphicsUtils::CalculateSphereScreenCoverage(sphere.m_fRadius, camera.GetDimensionY(1.0f));
  }
}

struct LodCompFlags
{
  enum Enum
  {
    ShowDebugInfo = 0,
    OverlapRanges = 1,
  };
};

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plLodComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("BoundsOffset", m_vBoundsOffset),
    PL_MEMBER_PROPERTY("BoundsRadius", m_fBoundsRadius)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.01f, 100.0f)),
    PL_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    PL_ACCESSOR_PROPERTY("OverlapRanges", GetOverlapRanges, SetOverlapRanges)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_ARRAY_MEMBER_PROPERTY("LodThresholds", m_LodThresholds)->AddAttributes(new plMaxArraySizeAttribute(4), new plClampValueAttribute(0.0f, 1.0f)),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Rendering"),
    new plSphereVisualizerAttribute("BoundsRadius", plColor::MediumVioletRed, nullptr, plVisualizerAnchor::Center, plVec3(1.0f), "BoundsOffset"),
    new plTransformManipulatorAttribute("BoundsOffset"),
  }
  PL_END_ATTRIBUTES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
    PL_MESSAGE_HANDLER(plMsgComponentInternalTrigger, OnMsgComponentInternalTrigger),
  }
  PL_END_MESSAGEHANDLERS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static const plTempHashedString sLod0("LOD0");
static const plTempHashedString sLod1("LOD1");
static const plTempHashedString sLod2("LOD2");
static const plTempHashedString sLod3("LOD3");
static const plTempHashedString sLod4("LOD4");

plLodComponent::plLodComponent()
{
  SetOverlapRanges(true);
}

plLodComponent::~plLodComponent() = default;

void plLodComponent::SetShowDebugInfo(bool bShow)
{
  SetUserFlag(LodCompFlags::ShowDebugInfo, bShow);
}

bool plLodComponent::GetShowDebugInfo() const
{
  return GetUserFlag(LodCompFlags::ShowDebugInfo);
}

void plLodComponent::SetOverlapRanges(bool bShow)
{
  SetUserFlag(LodCompFlags::OverlapRanges, bShow);
}

bool plLodComponent::GetOverlapRanges() const
{
  return GetUserFlag(LodCompFlags::OverlapRanges);
}

void plLodComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_vBoundsOffset;
  s << m_fBoundsRadius;

  s.WriteArray(m_LodThresholds).AssertSuccess();
}

void plLodComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_vBoundsOffset;
  s >> m_fBoundsRadius;

  s.ReadArray(m_LodThresholds).AssertSuccess();
}

plResult plLodComponent::GetLocalBounds(plBoundingBoxSphere& out_bounds, bool& out_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  out_bounds = plBoundingSphere::MakeFromCenterAndRadius(m_vBoundsOffset, m_fBoundsRadius);
  out_bAlwaysVisible = false;
  return PL_SUCCESS;
}

void plLodComponent::OnActivated()
{
  SUPER::OnActivated();

  // start with the highest LOD (lowest detail)
  m_iCurLod = m_LodThresholds.GetCount();

  plMsgComponentInternalTrigger trig;
  trig.m_iPayload = m_iCurLod;
  OnMsgComponentInternalTrigger(trig);
}

void plLodComponent::OnDeactivated()
{
  // when the component gets deactivated, activate all LOD children
  // this is important for editing to not behave weirdly
  // not sure whether this can have unintended side-effects at runtime
  // but there this should only be called for objects that get deleted anyway

  plGameObject* pLod[5];
  pLod[0] = GetOwner()->FindChildByName(sLod0);
  pLod[1] = GetOwner()->FindChildByName(sLod1);
  pLod[2] = GetOwner()->FindChildByName(sLod2);
  pLod[3] = GetOwner()->FindChildByName(sLod3);
  pLod[4] = GetOwner()->FindChildByName(sLod4);

  for (plUInt32 i = 0; i < PL_ARRAY_SIZE(pLod); ++i)
  {
    if (pLod[i])
    {
      pLod[i]->SetActiveFlag(true);
    }
  }

  SUPER::OnDeactivated();
}

void plLodComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  if (msg.m_pView->GetCameraUsageHint() != plCameraUsageHint::EditorView &&
      msg.m_pView->GetCameraUsageHint() != plCameraUsageHint::MainView)
  {
    return;
  }

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != plInvalidRenderDataCategory)
    return;

  const plInt32 iNumLods = (plInt32)m_LodThresholds.GetCount();

  const plVec3 vScale = GetOwner()->GetGlobalScaling();
  const float fScale = plMath::Max(vScale.x, vScale.y, vScale.z);
  const plVec3 vCenter = GetOwner()->GetGlobalTransform() * m_vBoundsOffset;

  const float fCoverage = CalculateSphereScreenSpaceCoverage(plBoundingSphere::MakeFromCenterAndRadius(vCenter, fScale * m_fBoundsRadius), *msg.m_pView->GetCullingCamera());

  // clamp the input value, this is to prevent issues while editing the threshold array
  plInt32 iNewLod = plMath::Clamp<plInt32>(m_iCurLod, 0, iNumLods);

  float fCoverageP = 1;
  float fCoverageN = 0;

  if (iNewLod > 0)
  {
    fCoverageP = m_LodThresholds[iNewLod - 1];
  }

  if (iNewLod < iNumLods)
  {
    fCoverageN = m_LodThresholds[iNewLod];
  }

  if (GetOverlapRanges())
  {
    const float fLodRangeOverlap = 0.40f;

    if (iNewLod + 1 < iNumLods)
    {
      float range = (fCoverageN - m_LodThresholds[iNewLod + 1]);
      fCoverageN -= range * fLodRangeOverlap; // overlap into the next range
    }
    else
    {
      float range = (fCoverageN - 0.0f);
      fCoverageN -= range * fLodRangeOverlap; // overlap into the next range
    }
  }

  if (fCoverage < fCoverageN)
  {
    ++iNewLod;
  }
  else if (fCoverage > fCoverageP)
  {
    --iNewLod;
  }

  iNewLod = plMath::Clamp(iNewLod, 0, iNumLods);

  if (GetShowDebugInfo())
  {
    plStringBuilder sb;
    sb.SetFormat("Coverage: {}\nLOD {}\nRange: {} - {}", plArgF(fCoverage, 3), iNewLod, plArgF(fCoverageP, 3), plArgF(fCoverageN, 3));
    plDebugRenderer::Draw3DText(msg.m_pView->GetHandle(), sb, GetOwner()->GetGlobalPosition(), plColor::White);
  }

  if (iNewLod == m_iCurLod)
    return;

  plMsgComponentInternalTrigger trig;
  trig.m_iPayload = iNewLod;

  PostMessage(trig);
}

void plLodComponent::OnMsgComponentInternalTrigger(plMsgComponentInternalTrigger& msg)
{
  m_iCurLod = msg.m_iPayload;

  // search for direct children named LODn, don't waste performance searching recursively
  plGameObject* pLod[5];
  pLod[0] = GetOwner()->FindChildByName(sLod0, false);
  pLod[1] = GetOwner()->FindChildByName(sLod1, false);
  pLod[2] = GetOwner()->FindChildByName(sLod2, false);
  pLod[3] = GetOwner()->FindChildByName(sLod3, false);
  pLod[4] = GetOwner()->FindChildByName(sLod4, false);

  // activate the selected LOD, deactivate all others
  for (plUInt32 i = 0; i < PL_ARRAY_SIZE(pLod); ++i)
  {
    if (pLod[i])
    {
      pLod[i]->SetActiveFlag(m_iCurLod == i);
    }
  }
}