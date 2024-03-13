#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Pipeline/View.h>

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
plCVarBool cvar_RenderingLightingVisScreenSpaceSize("Rendering.Lighting.VisScreenSpaceSize", false, plCVarFlags::Default, "Enables debug visualization of light screen space size calculation");
#endif

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSpotLightRenderData, 1, plRTTIDefaultAllocator<plSpotLightRenderData>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_COMPONENT_TYPE(plSpotLightComponent, 2, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(0.0f), new plSuffixAttribute(" m"), new plMinValueTextAttribute("Auto")),
    PL_ACCESSOR_PROPERTY("InnerSpotAngle", GetInnerSpotAngle, SetInnerSpotAngle)->AddAttributes(new plClampValueAttribute(plAngle::MakeFromDegree(0.0f), plAngle::MakeFromDegree(179.0f)), new plDefaultValueAttribute(plAngle::MakeFromDegree(15.0f))),
    PL_ACCESSOR_PROPERTY("OuterSpotAngle", GetOuterSpotAngle, SetOuterSpotAngle)->AddAttributes(new plClampValueAttribute(plAngle::MakeFromDegree(0.0f), plAngle::MakeFromDegree(179.0f)), new plDefaultValueAttribute(plAngle::MakeFromDegree(30.0f))),
    //PL_ACCESSOR_PROPERTY("ProjectedTexture", GetProjectedTextureFile, SetProjectedTextureFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plSpotLightVisualizerAttribute("OuterSpotAngle", "Range", "Intensity", "LightColor"),
    new plConeLengthManipulatorAttribute("Range"),
    new plConeAngleManipulatorAttribute("OuterSpotAngle", 1.5f),
    new plConeAngleManipulatorAttribute("InnerSpotAngle", 1.5f),
    new plCapsuleVisualizerAttribute("Length", "Width"),
    new plCapsuleVisualizerAttribute("Length",  "Width", plColor::Grey),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plSpotLightComponent::plSpotLightComponent()
{
  m_fEffectiveRange = m_fRange;//CalculateEffectiveRange(m_fRange, m_fIntensity);
}

plSpotLightComponent::~plSpotLightComponent() = default;

plResult plSpotLightComponent::GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  m_fEffectiveRange = m_fRange;//CalculateEffectiveRange(m_fRange, m_fIntensity);

  ref_bounds = CalculateBoundingSphere(plTransform::MakeIdentity(), m_fEffectiveRange);
  return PL_SUCCESS;
}

void plSpotLightComponent::SetRange(float fRange)
{
  m_fRange = fRange;

  TriggerLocalBoundsUpdate();
}

float plSpotLightComponent::GetRange() const
{
  return m_fRange;
}

float plSpotLightComponent::GetEffectiveRange() const
{
  return m_fEffectiveRange;
}

void plSpotLightComponent::SetInnerSpotAngle(plAngle spotAngle)
{
  m_InnerSpotAngle = plMath::Clamp(spotAngle, plAngle::MakeFromDegree(0.0f), m_OuterSpotAngle);

  InvalidateCachedRenderData();
}

plAngle plSpotLightComponent::GetInnerSpotAngle() const
{
  return m_InnerSpotAngle;
}

void plSpotLightComponent::SetOuterSpotAngle(plAngle spotAngle)
{
  m_OuterSpotAngle = plMath::Clamp(spotAngle, m_InnerSpotAngle, plAngle::MakeFromDegree(179.0f));

  TriggerLocalBoundsUpdate();
}

plAngle plSpotLightComponent::GetOuterSpotAngle() const
{
  return m_OuterSpotAngle;
}

// void plSpotLightComponent::SetProjectedTexture(const plTexture2DResourceHandle& hProjectedTexture)
//{
//   m_hProjectedTexture = hProjectedTexture;
//
//   InvalidateCachedRenderData();
// }
//
// const plTexture2DResourceHandle& plSpotLightComponent::GetProjectedTexture() const
//{
//   return m_hProjectedTexture;
// }
//
// void plSpotLightComponent::SetProjectedTextureFile(const char* szFile)
//{
//   plTexture2DResourceHandle hProjectedTexture;
//
//   if (!plStringUtils::IsNullOrEmpty(szFile))
//   {
//     hProjectedTexture = plResourceManager::LoadResource<plTexture2DResource>(szFile);
//   }
//
//   SetProjectedTexture(hProjectedTexture);
// }
//
// const char* plSpotLightComponent::GetProjectedTextureFile() const
//{
//   if (!m_hProjectedTexture.IsValid())
//     return "";
//
//   return m_hProjectedTexture.GetResourceID();
// }

void plSpotLightComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  // Don't extract light render data for selection or in shadow views.
  if (msg.m_OverrideCategory != plInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == plCameraUsageHint::Shadow)
    return;

  if (m_fIntensity <= 0.0f || m_fEffectiveRange <= 0.0f || m_OuterSpotAngle.GetRadian() <= 0.0f)
    return;

  plTransform t = GetOwner()->GetGlobalTransform();
  plBoundingSphere bs = CalculateBoundingSphere(t, m_fEffectiveRange * 0.5f);

  float fScreenSpaceSize = CalculateScreenSpaceSize(bs, *msg.m_pView->GetCullingCamera());

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  if (cvar_RenderingLightingVisScreenSpaceSize)
  {
    plStringBuilder sb;
    sb.SetFormat("{0}", fScreenSpaceSize);
    plDebugRenderer::Draw3DText(msg.m_pView->GetHandle(), sb, t.m_vPosition, plColor::Olive);
    plDebugRenderer::DrawLineSphere(msg.m_pView->GetHandle(), bs, plColor::Olive);
  }
#endif

  auto pRenderData = plCreateRenderDataForThisFrame<plSpotLightRenderData>(GetOwner());

  pRenderData->m_GlobalTransform = t;
  pRenderData->m_LightColor = GetLightColor();
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fSpecularMultiplier = m_fSpecularMultiplier;
  pRenderData->m_fWidth = m_fWidth;
  pRenderData->m_fLength = m_fLength;
  pRenderData->m_fRange = m_fEffectiveRange;
  pRenderData->m_InnerSpotAngle = m_InnerSpotAngle;
  pRenderData->m_OuterSpotAngle = m_OuterSpotAngle;
  // pRenderData->m_hProjectedTexture = m_hProjectedTexture;
  pRenderData->m_uiShadowDataOffset = m_bCastShadows ? plShadowPool::AddSpotLight(this, fScreenSpaceSize, msg.m_pView) : plInvalidIndex;

  pRenderData->FillBatchIdAndSortingKey(fScreenSpaceSize);

  plRenderData::Caching::Enum caching = m_bCastShadows ? plRenderData::Caching::Never : plRenderData::Caching::IfStatic;
  msg.AddRenderData(pRenderData, plDefaultRenderDataCategories::Light, caching);
}

void plSpotLightComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_fRange;
  s << m_InnerSpotAngle;
  s << m_OuterSpotAngle;
  s << ""; // GetProjectedTextureFile();
}

void plSpotLightComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  plTexture2DResourceHandle m_hProjectedTexture;

  s >> m_fRange;
  s >> m_InnerSpotAngle;
  s >> m_OuterSpotAngle;

  plStringBuilder temp;
  s >> temp;
  // SetProjectedTextureFile(temp);
}

plBoundingSphere plSpotLightComponent::CalculateBoundingSphere(const plTransform& t, float fRange) const
{
  plBoundingSphere res;
  plAngle halfAngle = m_OuterSpotAngle / 2.0f;
  plVec3 position = t.m_vPosition;
  plVec3 forwardDir = t.m_qRotation * plVec3(1.0f, 0.0f, 0.0f);

  if (halfAngle > plAngle::MakeFromDegree(45.0f))
  {
    res.m_vCenter = position + plMath::Cos(halfAngle) * fRange * forwardDir;
    res.m_fRadius = plMath::Sin(halfAngle) * fRange;
  }
  else
  {
    res.m_fRadius = fRange / (2.0f * plMath::Cos(halfAngle));
    res.m_vCenter = position + forwardDir * res.m_fRadius;
  }

  return res;
}

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSpotLightVisualizerAttribute, 1, plRTTIDefaultAllocator<plSpotLightVisualizerAttribute>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plSpotLightVisualizerAttribute::plSpotLightVisualizerAttribute()
  : plVisualizerAttribute(nullptr)
{
}

plSpotLightVisualizerAttribute::plSpotLightVisualizerAttribute(
  const char* szAngleProperty, const char* szRangeProperty, const char* szIntensityProperty, const char* szColorProperty)
  : plVisualizerAttribute(szAngleProperty, szRangeProperty, szIntensityProperty, szColorProperty)
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class plSpotLightComponentPatch_1_2 : public plGraphPatch
{
public:
  plSpotLightComponentPatch_1_2()
    : plGraphPatch("plSpotLightComponent", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    ref_context.PatchBaseClass("plLightComponent", 2, true);

    pNode->RenameProperty("Inner Spot Angle", "InnerSpotAngle");
    pNode->RenameProperty("Outer Spot Angle", "OuterSpotAngle");
  }
};

plSpotLightComponentPatch_1_2 g_plSpotLightComponentPatch_1_2;


PL_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SpotLightComponent);
