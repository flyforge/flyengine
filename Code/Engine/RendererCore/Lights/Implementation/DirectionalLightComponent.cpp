#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDirectionalLightRenderData, 1, plRTTIDefaultAllocator<plDirectionalLightRenderData>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_COMPONENT_TYPE(plDirectionalLightComponent, 3, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("NumCascades", GetNumCascades, SetNumCascades)->AddAttributes(new plClampValueAttribute(1, 4), new plDefaultValueAttribute(2)),
    PL_ACCESSOR_PROPERTY("MinShadowRange", GetMinShadowRange, SetMinShadowRange)->AddAttributes(new plClampValueAttribute(0.1f, plVariant()), new plDefaultValueAttribute(30.0f), new plSuffixAttribute(" m")),
    PL_ACCESSOR_PROPERTY("FadeOutStart", GetFadeOutStart, SetFadeOutStart)->AddAttributes(new plClampValueAttribute(0.6f, 1.0f), new plDefaultValueAttribute(0.8f)),
    PL_ACCESSOR_PROPERTY("SplitModeWeight", GetSplitModeWeight, SetSplitModeWeight)->AddAttributes(new plClampValueAttribute(0.0f, 1.0f), new plDefaultValueAttribute(0.7f)),
    PL_ACCESSOR_PROPERTY("NearPlaneOffset", GetNearPlaneOffset, SetNearPlaneOffset)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(100.0f), new plSuffixAttribute(" m")),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plDirectionVisualizerAttribute(plBasisAxis::PositiveX, 1.0f, plColor::White, "LightColor"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plDirectionalLightComponent::plDirectionalLightComponent() = default;
plDirectionalLightComponent::~plDirectionalLightComponent() = default;

plResult plDirectionalLightComponent::GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return PL_SUCCESS;
}

void plDirectionalLightComponent::SetNumCascades(plUInt32 uiNumCascades)
{
  m_uiNumCascades = plMath::Clamp(uiNumCascades, 1u, 4u);

  InvalidateCachedRenderData();
}

plUInt32 plDirectionalLightComponent::GetNumCascades() const
{
  return m_uiNumCascades;
}

void plDirectionalLightComponent::SetMinShadowRange(float fMinShadowRange)
{
  m_fMinShadowRange = plMath::Max(fMinShadowRange, 0.0f);

  InvalidateCachedRenderData();
}

float plDirectionalLightComponent::GetMinShadowRange() const
{
  return m_fMinShadowRange;
}

void plDirectionalLightComponent::SetFadeOutStart(float fFadeOutStart)
{
  m_fFadeOutStart = plMath::Clamp(fFadeOutStart, 0.0f, 1.0f);

  InvalidateCachedRenderData();
}

float plDirectionalLightComponent::GetFadeOutStart() const
{
  return m_fFadeOutStart;
}

void plDirectionalLightComponent::SetSplitModeWeight(float fSplitModeWeight)
{
  m_fSplitModeWeight = plMath::Clamp(fSplitModeWeight, 0.0f, 1.0f);

  InvalidateCachedRenderData();
}

float plDirectionalLightComponent::GetSplitModeWeight() const
{
  return m_fSplitModeWeight;
}

void plDirectionalLightComponent::SetNearPlaneOffset(float fNearPlaneOffset)
{
  m_fNearPlaneOffset = plMath::Max(fNearPlaneOffset, 0.0f);

  InvalidateCachedRenderData();
}

float plDirectionalLightComponent::GetNearPlaneOffset() const
{
  return m_fNearPlaneOffset;
}

void plDirectionalLightComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  // Don't extract light render data for selection or in shadow views.
  if (msg.m_OverrideCategory != plInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == plCameraUsageHint::Shadow)
    return;

  if (m_fIntensity <= 0.0f)
    return;

  auto pRenderData = plCreateRenderDataForThisFrame<plDirectionalLightRenderData>(GetOwner());

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_LightColor = GetLightColor();
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fSpecularMultiplier = m_fSpecularMultiplier;
  pRenderData->m_uiShadowDataOffset = m_bCastShadows ? plShadowPool::AddDirectionalLight(this, msg.m_pView) : plInvalidIndex;

  pRenderData->FillBatchIdAndSortingKey(1.0f);

  plRenderData::Caching::Enum caching = m_bCastShadows ? plRenderData::Caching::Never : plRenderData::Caching::IfStatic;
  msg.AddRenderData(pRenderData, plDefaultRenderDataCategories::Light, caching);
}

void plDirectionalLightComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  plStreamWriter& s = inout_stream.GetStream();

  s << m_uiNumCascades;
  s << m_fMinShadowRange;
  s << m_fFadeOutStart;
  s << m_fSplitModeWeight;
  s << m_fNearPlaneOffset;
}

void plDirectionalLightComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  if (uiVersion >= 3)
  {
    s >> m_uiNumCascades;
    s >> m_fMinShadowRange;
    s >> m_fFadeOutStart;
    s >> m_fSplitModeWeight;
    s >> m_fNearPlaneOffset;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class plDirectionalLightComponentPatch_1_2 : public plGraphPatch
{
public:
  plDirectionalLightComponentPatch_1_2()
    : plGraphPatch("plDirectionalLightComponent", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    ref_context.PatchBaseClass("plLightComponent", 2, true);
  }
};

plDirectionalLightComponentPatch_1_2 g_plDirectionalLightComponentPatch_1_2;



PL_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_DirectionalLightComponent);
