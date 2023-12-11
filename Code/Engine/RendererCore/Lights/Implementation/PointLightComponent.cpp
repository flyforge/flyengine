#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plPointLightRenderData, 1, plRTTIDefaultAllocator<plPointLightRenderData>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_COMPONENT_TYPE(plPointLightComponent, 2, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(0.0f), new plSuffixAttribute(" m"), new plMinValueTextAttribute("Auto")),
    //PLASMA_ACCESSOR_PROPERTY("ProjectedTexture", GetProjectedTextureFile, SetProjectedTextureFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Texture_Cube")),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plSphereManipulatorAttribute("Range"),
    new plPointLightVisualizerAttribute("Range", "Intensity", "LightColor"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plPointLightComponent::plPointLightComponent()
{
  m_fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);
}

plPointLightComponent::~plPointLightComponent() = default;

plResult plPointLightComponent::GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  m_fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);

  ref_bounds = plBoundingSphere::MakeFromCenterAndRadius(plVec3::MakeZero(), m_fEffectiveRange);
  return PLASMA_SUCCESS;
}

void plPointLightComponent::SetRange(float fRange)
{
  m_fRange = fRange;

  TriggerLocalBoundsUpdate();
}

float plPointLightComponent::GetRange() const
{
  return m_fRange;
}

float plPointLightComponent::GetEffectiveRange() const
{
  return m_fEffectiveRange;
}

void plPointLightComponent::SetProjectedTexture(const plTextureCubeResourceHandle& hProjectedTexture)
{
  m_hProjectedTexture = hProjectedTexture;

  InvalidateCachedRenderData();
}

const plTextureCubeResourceHandle& plPointLightComponent::GetProjectedTexture() const
{
  return m_hProjectedTexture;
}

void plPointLightComponent::SetProjectedTextureFile(const char* szFile)
{
  plTextureCubeResourceHandle hProjectedTexture;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hProjectedTexture = plResourceManager::LoadResource<plTextureCubeResource>(szFile);
  }

  SetProjectedTexture(hProjectedTexture);
}

const char* plPointLightComponent::GetProjectedTextureFile() const
{
  if (!m_hProjectedTexture.IsValid())
    return "";

  return m_hProjectedTexture.GetResourceID();
}

void plPointLightComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  // Don't extract light render data for selection or in shadow views.
  if (msg.m_OverrideCategory != plInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == plCameraUsageHint::Shadow)
    return;

  if (m_fIntensity <= 0.0f || m_fEffectiveRange <= 0.0f)
    return;

  plTransform t = GetOwner()->GetGlobalTransform();

  float fScreenSpaceSize = CalculateScreenSpaceSize(plBoundingSphere::MakeFromCenterAndRadius(t.m_vPosition, m_fEffectiveRange * 0.5f), *msg.m_pView->GetCullingCamera());

  auto pRenderData = plCreateRenderDataForThisFrame<plPointLightRenderData>(GetOwner());

  pRenderData->m_GlobalTransform = t;
  pRenderData->m_LightColor = m_LightColor;
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fRange = m_fEffectiveRange;
  pRenderData->m_hProjectedTexture = m_hProjectedTexture;
  pRenderData->m_uiShadowDataOffset = m_bCastShadows ? plShadowPool::AddPointLight(this, fScreenSpaceSize, msg.m_pView) : plInvalidIndex;

  pRenderData->FillBatchIdAndSortingKey(fScreenSpaceSize);

  plRenderData::Caching::Enum caching = m_bCastShadows ? plRenderData::Caching::Never : plRenderData::Caching::IfStatic;
  msg.AddRenderData(pRenderData, plDefaultRenderDataCategories::Light, caching);
}

void plPointLightComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_fRange;
  s << m_hProjectedTexture;
}

void plPointLightComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_fRange;
  s >> m_hProjectedTexture;
}

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plPointLightVisualizerAttribute, 1, plRTTIDefaultAllocator<plPointLightVisualizerAttribute>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plPointLightVisualizerAttribute::plPointLightVisualizerAttribute()
  : plVisualizerAttribute(nullptr)
{
}

plPointLightVisualizerAttribute::plPointLightVisualizerAttribute(
  const char* szRangeProperty, const char* szIntensityProperty, const char* szColorProperty)
  : plVisualizerAttribute(szRangeProperty, szIntensityProperty, szColorProperty)
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class plPointLightComponentPatch_1_2 : public plGraphPatch
{
public:
  plPointLightComponentPatch_1_2()
    : plGraphPatch("plPointLightComponent", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    ref_context.PatchBaseClass("plLightComponent", 2, true);
  }
};

plPointLightComponentPatch_1_2 g_plPointLightComponentPatch_1_2;

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_PointLightComponent);