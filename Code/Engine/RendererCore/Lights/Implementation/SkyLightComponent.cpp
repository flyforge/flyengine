#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/SkyLightComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureCubeResource.h>

namespace
{
  static plVariantArray GetDefaultTags()
  {
    plVariantArray value(plStaticsAllocatorWrapper::GetAllocator());
    value.PushBack("SkyLight");
    return value;
  }
} // namespace

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plSkyLightComponent, 3, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ENUM_ACCESSOR_PROPERTY("ReflectionProbeMode", plReflectionProbeMode, GetReflectionProbeMode, SetReflectionProbeMode)->AddAttributes(new plDefaultValueAttribute(plReflectionProbeMode::Dynamic), new plGroupAttribute("Capture Description")),
    PL_ACCESSOR_PROPERTY("CubeMap", GetCubeMapFile, SetCubeMapFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Texture_Cube")),
    PL_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(1.0f)),
    PL_ACCESSOR_PROPERTY("Saturation", GetSaturation, SetSaturation)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(1.0f)),
    PL_SET_ACCESSOR_PROPERTY("IncludeTags", GetIncludeTags, InsertIncludeTag, RemoveIncludeTag)->AddAttributes(new plTagSetWidgetAttribute("Default"), new plDefaultValueAttribute(GetDefaultTags())),
    PL_SET_ACCESSOR_PROPERTY("ExcludeTags", GetExcludeTags, InsertExcludeTag, RemoveExcludeTag)->AddAttributes(new plTagSetWidgetAttribute("Default")),
    PL_ACCESSOR_PROPERTY("NearPlane", GetNearPlane, SetNearPlane)->AddAttributes(new plDefaultValueAttribute(0.0f), new plClampValueAttribute(0.0f, {}), new plMinValueTextAttribute("Auto")),
    PL_ACCESSOR_PROPERTY("FarPlane", GetFarPlane, SetFarPlane)->AddAttributes(new plDefaultValueAttribute(100.0f), new plClampValueAttribute(0.01f, 10000.0f)),
    PL_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    PL_ACCESSOR_PROPERTY("ShowMipMaps", GetShowMipMaps, SetShowMipMaps),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
    PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
    PL_MESSAGE_HANDLER(plMsgTransformChanged, OnTransformChanged),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Lighting"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plSkyLightComponent::plSkyLightComponent()
{
  m_Desc.m_uniqueID = plUuid::MakeUuid();
}

plSkyLightComponent::~plSkyLightComponent() = default;

void plSkyLightComponent::OnActivated()
{
  GetOwner()->EnableStaticTransformChangesNotifications();
  m_Id = plReflectionPool::RegisterSkyLight(GetWorld(), m_Desc, this);

  GetOwner()->UpdateLocalBounds();
}

void plSkyLightComponent::OnDeactivated()
{
  plReflectionPool::DeregisterSkyLight(GetWorld(), m_Id);
  m_Id.Invalidate();

  GetOwner()->UpdateLocalBounds();
}

void plSkyLightComponent::SetReflectionProbeMode(plEnum<plReflectionProbeMode> mode)
{
  m_Desc.m_Mode = mode;
  m_bStatesDirty = true;
}

plEnum<plReflectionProbeMode> plSkyLightComponent::GetReflectionProbeMode() const
{
  return m_Desc.m_Mode;
}

void plSkyLightComponent::SetIntensity(float fIntensity)
{
  m_Desc.m_fIntensity = fIntensity;
  m_bStatesDirty = true;
}

float plSkyLightComponent::GetIntensity() const
{
  return m_Desc.m_fIntensity;
}

void plSkyLightComponent::SetSaturation(float fSaturation)
{
  m_Desc.m_fSaturation = fSaturation;
  m_bStatesDirty = true;
}

float plSkyLightComponent::GetSaturation() const
{
  return m_Desc.m_fSaturation;
}

const plTagSet& plSkyLightComponent::GetIncludeTags() const
{
  return m_Desc.m_IncludeTags;
}

void plSkyLightComponent::InsertIncludeTag(const char* szTag)
{
  m_Desc.m_IncludeTags.SetByName(szTag);
  m_bStatesDirty = true;
}

void plSkyLightComponent::RemoveIncludeTag(const char* szTag)
{
  m_Desc.m_IncludeTags.RemoveByName(szTag);
  m_bStatesDirty = true;
}

const plTagSet& plSkyLightComponent::GetExcludeTags() const
{
  return m_Desc.m_ExcludeTags;
}

void plSkyLightComponent::InsertExcludeTag(const char* szTag)
{
  m_Desc.m_ExcludeTags.SetByName(szTag);
  m_bStatesDirty = true;
}

void plSkyLightComponent::RemoveExcludeTag(const char* szTag)
{
  m_Desc.m_ExcludeTags.RemoveByName(szTag);
  m_bStatesDirty = true;
}

void plSkyLightComponent::SetShowDebugInfo(bool bShowDebugInfo)
{
  m_Desc.m_bShowDebugInfo = bShowDebugInfo;
  m_bStatesDirty = true;
}

bool plSkyLightComponent::GetShowDebugInfo() const
{
  return m_Desc.m_bShowDebugInfo;
}

void plSkyLightComponent::SetShowMipMaps(bool bShowMipMaps)
{
  m_Desc.m_bShowMipMaps = bShowMipMaps;
  m_bStatesDirty = true;
}

bool plSkyLightComponent::GetShowMipMaps() const
{
  return m_Desc.m_bShowMipMaps;
}


void plSkyLightComponent::SetCubeMapFile(const char* szFile)
{
  plTextureCubeResourceHandle hCubeMap;
  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hCubeMap = plResourceManager::LoadResource<plTextureCubeResource>(szFile);
  }
  m_hCubeMap = hCubeMap;
  m_bStatesDirty = true;
}

const char* plSkyLightComponent::GetCubeMapFile() const
{
  return m_hCubeMap.IsValid() ? m_hCubeMap.GetResourceID().GetData() : "";
}

void plSkyLightComponent::SetNearPlane(float fNearPlane)
{
  m_Desc.m_fNearPlane = fNearPlane;
  m_bStatesDirty = true;
}

void plSkyLightComponent::SetFarPlane(float fFarPlane)
{
  m_Desc.m_fFarPlane = fFarPlane;
  m_bStatesDirty = true;
}

void plSkyLightComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? plDefaultSpatialDataCategories::RenderDynamic : plDefaultSpatialDataCategories::RenderStatic);
}

void plSkyLightComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  // Don't trigger reflection rendering in shadow or other reflection views.
  if (msg.m_pView->GetCameraUsageHint() == plCameraUsageHint::Shadow || msg.m_pView->GetCameraUsageHint() == plCameraUsageHint::Reflection)
    return;

  if (m_bStatesDirty)
  {
    m_bStatesDirty = false;
    plReflectionPool::UpdateSkyLight(GetWorld(), m_Id, m_Desc, this);
  }

  plReflectionPool::ExtractReflectionProbe(this, msg, nullptr, GetWorld(), m_Id, plMath::MaxValue<float>());
}

void plSkyLightComponent::OnTransformChanged(plMsgTransformChanged& msg)
{
  m_bStatesDirty = true;
}

void plSkyLightComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  m_Desc.m_IncludeTags.Save(s);
  m_Desc.m_ExcludeTags.Save(s);
  s << m_Desc.m_Mode;
  s << m_Desc.m_bShowDebugInfo;
  s << m_Desc.m_fIntensity;
  s << m_Desc.m_fSaturation;
  s << m_hCubeMap;
  s << m_Desc.m_fNearPlane;
  s << m_Desc.m_fFarPlane;
}

void plSkyLightComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  m_Desc.m_IncludeTags.Load(s, plTagRegistry::GetGlobalRegistry());
  m_Desc.m_ExcludeTags.Load(s, plTagRegistry::GetGlobalRegistry());
  s >> m_Desc.m_Mode;
  s >> m_Desc.m_bShowDebugInfo;
  s >> m_Desc.m_fIntensity;
  s >> m_Desc.m_fSaturation;
  if (uiVersion >= 2)
  {
    s >> m_hCubeMap;
  }
  if (uiVersion >= 3)
  {
    s >> m_Desc.m_fNearPlane;
    s >> m_Desc.m_fFarPlane;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class plSkyLightComponentPatch_2_3 : public plGraphPatch
{
public:
  plSkyLightComponentPatch_2_3()
    : plGraphPatch("plSkyLightComponent", 3)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    // Inline ReflectionData sub-object into the sky light itself.
    if (const plAbstractObjectNode::Property* pProp0 = pNode->FindProperty("ReflectionData"))
    {
      if (pProp0->m_Value.IsA<plUuid>())
      {
        if (plAbstractObjectNode* pSubNode = pGraph->GetNode(pProp0->m_Value.Get<plUuid>()))
        {
          for (auto pProp : pSubNode->GetProperties())
          {
            pNode->AddProperty(pProp.m_sPropertyName, pProp.m_Value);
          }
        }
      }
    }
  }
};

plSkyLightComponentPatch_2_3 g_plSkyLightComponentPatch_2_3;

PL_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SkyLightComponent);
