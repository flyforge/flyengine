#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <ProcGenPlugin/Components/ProcVolumeComponent.h>
#include <ProcGenPlugin/Components/VolumeCollection.h>

namespace
{
  plSpatialData::Category s_ProcVolumeCategory = plSpatialData::RegisterCategory("ProcVolume", plSpatialData::Flags::None);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_ABSTRACT_COMPONENT_TYPE(plProcVolumeComponent, 1)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Value", GetValue, SetValue)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PL_ACCESSOR_PROPERTY("SortOrder", GetSortOrder, SetSortOrder)->AddAttributes(new plClampValueAttribute(-64.0f, 64.0f)),
    PL_ENUM_ACCESSOR_PROPERTY("BlendMode", plProcGenBlendMode, GetBlendMode, SetBlendMode)->AddAttributes(new plDefaultValueAttribute(plProcGenBlendMode::Set)),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgTransformChanged, OnTransformChanged)
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Construction/Procedural Generation"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plEvent<const plProcGenInternal::InvalidatedArea&> plProcVolumeComponent::s_AreaInvalidatedEvent;

plProcVolumeComponent::plProcVolumeComponent() = default;
plProcVolumeComponent::~plProcVolumeComponent() = default;

void plProcVolumeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->EnableStaticTransformChangesNotifications();

  GetOwner()->UpdateLocalBounds();

  if (GetUniqueID() != plInvalidIndex)
  {
    // Only necessary in Editor
    InvalidateArea();
  }
}

void plProcVolumeComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  if (GetUniqueID() != plInvalidIndex)
  {
    // Only necessary in Editor
    plBoundingBoxSphere globalBounds = GetOwner()->GetGlobalBounds();
    if (globalBounds.IsValid())
    {
      InvalidateArea(globalBounds.GetBox());
    }
  }

  // Don't disable notifications as other components attached to the owner game object might need them too.
  // GetOwner()->DisableStaticTransformChangesNotifications();

  GetOwner()->UpdateLocalBounds();
}

void plProcVolumeComponent::SetValue(float fValue)
{
  if (m_fValue != fValue)
  {
    m_fValue = fValue;

    InvalidateArea();
  }
}

void plProcVolumeComponent::SetSortOrder(float fOrder)
{
  fOrder = plMath::Clamp(fOrder, -64.0f, 64.0f);

  if (m_fSortOrder != fOrder)
  {
    m_fSortOrder = fOrder;

    InvalidateArea();
  }
}

void plProcVolumeComponent::SetBlendMode(plEnum<plProcGenBlendMode> blendMode)
{
  if (m_BlendMode != blendMode)
  {
    m_BlendMode = blendMode;

    InvalidateArea();
  }
}

void plProcVolumeComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_fValue;
  s << m_fSortOrder;
  s << m_BlendMode;
}

void plProcVolumeComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_fValue;
  s >> m_fSortOrder;
  s >> m_BlendMode;
}

void plProcVolumeComponent::OnTransformChanged(plMsgTransformChanged& ref_msg)
{
  plBoundingBoxSphere combined = GetOwner()->GetLocalBounds();
  combined.Transform(ref_msg.m_OldGlobalTransform.GetAsMat4());

  combined.ExpandToInclude(GetOwner()->GetGlobalBounds());

  InvalidateArea(combined.GetBox());
}

void plProcVolumeComponent::InvalidateArea()
{
  if (!IsActiveAndInitialized())
    return;

  plBoundingBoxSphere globalBounds = GetOwner()->GetGlobalBounds();
  if (globalBounds.IsValid())
  {
    InvalidateArea(globalBounds.GetBox());
  }
}

void plProcVolumeComponent::InvalidateArea(const plBoundingBox& box)
{
  plProcGenInternal::InvalidatedArea area;
  area.m_Box = box;
  area.m_pWorld = GetWorld();

  s_AreaInvalidatedEvent.Broadcast(area);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plProcVolumeSphereComponent, 2, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new plDefaultValueAttribute(5.0f), new plClampValueAttribute(0.0f, plVariant())),
    PL_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, 1.0f)),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
    PL_MESSAGE_HANDLER(plMsgExtractVolumes, OnExtractVolumes)
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plSphereManipulatorAttribute("Radius"),
    new plSphereVisualizerAttribute("Radius", plColor::LimeGreen),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plProcVolumeSphereComponent::plProcVolumeSphereComponent() = default;
plProcVolumeSphereComponent::~plProcVolumeSphereComponent() = default;

void plProcVolumeSphereComponent::SetRadius(float fRadius)
{
  if (m_fRadius != fRadius)
  {
    m_fRadius = fRadius;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }

    InvalidateArea();
  }
}

void plProcVolumeSphereComponent::SetFalloff(float fFalloff)
{
  if (m_fFalloff != fFalloff)
  {
    m_fFalloff = fFalloff;

    InvalidateArea();
  }
}

void plProcVolumeSphereComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fFalloff;
}

void plProcVolumeSphereComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fFalloff;

  if (uiVersion < 2)
  {
    m_fFalloff = 1.0f - m_fFalloff;
  }
}

void plProcVolumeSphereComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(plBoundingSphere::MakeFromCenterAndRadius(plVec3::MakeZero(), m_fRadius), s_ProcVolumeCategory);
}

void plProcVolumeSphereComponent::OnExtractVolumes(plMsgExtractVolumes& ref_msg) const
{
  ref_msg.m_pCollection->AddSphere(GetOwner()->GetGlobalTransformSimd(), m_fRadius, m_BlendMode, m_fSortOrder, m_fValue, m_fFalloff);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plProcVolumeBoxComponent, 2, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new plDefaultValueAttribute(plVec3(10.0f)), new plClampValueAttribute(plVec3(0), plVariant())),
    PL_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new plDefaultValueAttribute(plVec3(0.5f)), new plClampValueAttribute(plVec3(0.0f), plVec3(1.0f))),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
    PL_MESSAGE_HANDLER(plMsgExtractVolumes, OnExtractVolumes)
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plBoxManipulatorAttribute("Extents", 1.0f, true),
    new plBoxVisualizerAttribute("Extents", 1.0f, plColor::LimeGreen),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plProcVolumeBoxComponent::plProcVolumeBoxComponent() = default;
plProcVolumeBoxComponent::~plProcVolumeBoxComponent() = default;

void plProcVolumeBoxComponent::SetExtents(const plVec3& vExtents)
{
  if (m_vExtents != vExtents)
  {
    m_vExtents = vExtents;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }

    InvalidateArea();
  }
}

void plProcVolumeBoxComponent::SetFalloff(const plVec3& vFalloff)
{
  if (m_vFalloff != vFalloff)
  {
    m_vFalloff = vFalloff;

    InvalidateArea();
  }
}

void plProcVolumeBoxComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
  s << m_vFalloff;
}

void plProcVolumeBoxComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;
  s >> m_vFalloff;

  if (uiVersion < 2)
  {
    m_vFalloff = plVec3(1.0f) - m_vFalloff;
  }
}

void plProcVolumeBoxComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(plBoundingBoxSphere::MakeFromBox(plBoundingBox::MakeFromMinMax(-m_vExtents * 0.5f, m_vExtents * 0.5f)), s_ProcVolumeCategory);
}

void plProcVolumeBoxComponent::OnExtractVolumes(plMsgExtractVolumes& ref_msg) const
{
  ref_msg.m_pCollection->AddBox(GetOwner()->GetGlobalTransformSimd(), m_vExtents, m_BlendMode, m_fSortOrder, m_fValue, m_vFalloff);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plProcVolumeImageComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Image", GetImageFile, SetImageFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Data_2D")),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgExtractVolumes, OnExtractVolumes)
  }
  PL_END_MESSAGEHANDLERS;
}
PL_END_COMPONENT_TYPE
// clang-format on

plProcVolumeImageComponent::plProcVolumeImageComponent() = default;
plProcVolumeImageComponent::~plProcVolumeImageComponent() = default;

void plProcVolumeImageComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_hImage;
}

void plProcVolumeImageComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_hImage;
}

void plProcVolumeImageComponent::OnExtractVolumes(plMsgExtractVolumes& ref_msg) const
{
  ref_msg.m_pCollection->AddImage(GetOwner()->GetGlobalTransformSimd(), m_vExtents, m_BlendMode, m_fSortOrder, m_fValue, m_vFalloff, m_hImage);
}

void plProcVolumeImageComponent::SetImageFile(const char* szFile)
{
  plImageDataResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plImageDataResource>(szFile);
  }

  SetImage(hResource);
}

const char* plProcVolumeImageComponent::GetImageFile() const
{
  if (!m_hImage.IsValid())
    return "";

  return m_hImage.GetResourceID();
}

void plProcVolumeImageComponent::SetImage(const plImageDataResourceHandle& hResource)
{
  m_hImage = hResource;
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class plProcVolumeSphereComponent_1_2 : public plGraphPatch
{
public:
  plProcVolumeSphereComponent_1_2()
    : plGraphPatch("plProcVolumeSphereComponent", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    auto* pFadeOutStart = pNode->FindProperty("FadeOutStart");
    if (pFadeOutStart && pFadeOutStart->m_Value.IsA<float>())
    {
      float fFalloff = 1.0f - pFadeOutStart->m_Value.Get<float>();
      pNode->AddProperty("Falloff", fFalloff);
    }
  }
};

plProcVolumeSphereComponent_1_2 g_plProcVolumeSphereComponent_1_2;

class plProcVolumeBoxComponent_1_2 : public plGraphPatch
{
public:
  plProcVolumeBoxComponent_1_2()
    : plGraphPatch("plProcVolumeBoxComponent", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    auto* pFadeOutStart = pNode->FindProperty("FadeOutStart");
    if (pFadeOutStart && pFadeOutStart->m_Value.IsA<plVec3>())
    {
      plVec3 vFalloff = plVec3(1.0f) - pFadeOutStart->m_Value.Get<plVec3>();
      pNode->AddProperty("Falloff", vFalloff);
    }
  }
};

plProcVolumeBoxComponent_1_2 g_plProcVolumeBoxComponent_1_2;
