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
PLASMA_BEGIN_ABSTRACT_COMPONENT_TYPE(plProcVolumeComponent, 1)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Value", GetValue, SetValue)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PLASMA_ACCESSOR_PROPERTY("SortOrder", GetSortOrder, SetSortOrder)->AddAttributes(new plClampValueAttribute(-64.0f, 64.0f)),
    PLASMA_ENUM_ACCESSOR_PROPERTY("BlendMode", plProcGenBlendMode, GetBlendMode, SetBlendMode)->AddAttributes(new plDefaultValueAttribute(plProcGenBlendMode::Set)),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgTransformChanged, OnTransformChanged)
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plColorAttribute(plColorScheme::Construction),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
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

void plProcVolumeComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  plStreamWriter& s = stream.GetStream();

  s << m_fValue;
  s << m_fSortOrder;
  s << m_BlendMode;
}

void plProcVolumeComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = stream.GetStream();

  s >> m_fValue;
  s >> m_fSortOrder;
  s >> m_BlendMode;
}

void plProcVolumeComponent::OnTransformChanged(plMsgTransformChanged& msg)
{
  plBoundingBoxSphere combined = GetOwner()->GetLocalBounds();
  combined.Transform(msg.m_OldGlobalTransform.GetAsMat4());

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
PLASMA_BEGIN_COMPONENT_TYPE(plProcVolumeSphereComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new plDefaultValueAttribute(5.0f), new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_ACCESSOR_PROPERTY("FadeOutStart", GetFadeOutStart, SetFadeOutStart)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, 1.0f)),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
    PLASMA_MESSAGE_HANDLER(plMsgExtractVolumes, OnExtractVolumes)
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Procedural Generation"),
    new plSphereManipulatorAttribute("Radius"),
    new plSphereVisualizerAttribute("Radius", plColor::LimeGreen),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
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

void plProcVolumeSphereComponent::SetFadeOutStart(float fFadeOutStart)
{
  if (m_fFadeOutStart != fFadeOutStart)
  {
    m_fFadeOutStart = fFadeOutStart;

    InvalidateArea();
  }
}

void plProcVolumeSphereComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  plStreamWriter& s = stream.GetStream();

  s << m_fRadius;
  s << m_fFadeOutStart;
}

void plProcVolumeSphereComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = stream.GetStream();

  s >> m_fRadius;
  s >> m_fFadeOutStart;
}

void plProcVolumeSphereComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(plBoundingSphere(plVec3::ZeroVector(), m_fRadius), s_ProcVolumeCategory);
}

void plProcVolumeSphereComponent::OnExtractVolumes(plMsgExtractVolumes& msg) const
{
  msg.m_pCollection->AddSphere(GetOwner()->GetGlobalTransformSimd(), m_fRadius, m_BlendMode, m_fSortOrder, m_fValue, m_fFadeOutStart);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plProcVolumeBoxComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new plDefaultValueAttribute(plVec3(10.0f)), new plClampValueAttribute(plVec3(0), plVariant())),
    PLASMA_ACCESSOR_PROPERTY("FadeOutStart", GetFadeOutStart, SetFadeOutStart)->AddAttributes(new plDefaultValueAttribute(plVec3(0.5f)), new plClampValueAttribute(plVec3(0.0f), plVec3(1.0f))),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
    PLASMA_MESSAGE_HANDLER(plMsgExtractVolumes, OnExtractVolumes)
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Procedural Generation"),
    new plBoxManipulatorAttribute("Extents", 1.0f, true),
    new plBoxVisualizerAttribute("Extents", 1.0f, plColor::LimeGreen),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plProcVolumeBoxComponent::plProcVolumeBoxComponent() = default;
plProcVolumeBoxComponent::~plProcVolumeBoxComponent() = default;

void plProcVolumeBoxComponent::SetExtents(const plVec3& extents)
{
  if (m_vExtents != extents)
  {
    m_vExtents = extents;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }

    InvalidateArea();
  }
}

void plProcVolumeBoxComponent::SetFadeOutStart(const plVec3& fadeOutStart)
{
  if (m_vFadeOutStart != fadeOutStart)
  {
    m_vFadeOutStart = fadeOutStart;

    InvalidateArea();
  }
}

void plProcVolumeBoxComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  plStreamWriter& s = stream.GetStream();

  s << m_vExtents;
  s << m_vFadeOutStart;
}

void plProcVolumeBoxComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = stream.GetStream();

  s >> m_vExtents;
  s >> m_vFadeOutStart;
}

void plProcVolumeBoxComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(plBoundingBox(-m_vExtents * 0.5f, m_vExtents * 0.5f), s_ProcVolumeCategory);
}

void plProcVolumeBoxComponent::OnExtractVolumes(plMsgExtractVolumes& msg) const
{
  msg.m_pCollection->AddBox(GetOwner()->GetGlobalTransformSimd(), m_vExtents, m_BlendMode, m_fSortOrder, m_fValue, m_vFadeOutStart);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plProcVolumeImageComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Image", GetImageFile, SetImageFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Data_2D")),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgExtractVolumes, OnExtractVolumes)
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plProcVolumeImageComponent::plProcVolumeImageComponent() = default;
plProcVolumeImageComponent::~plProcVolumeImageComponent() = default;

void plProcVolumeImageComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  plStreamWriter& s = stream.GetStream();

  s << m_hImage;
}

void plProcVolumeImageComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = stream.GetStream();

  s >> m_hImage;
}

void plProcVolumeImageComponent::OnExtractVolumes(plMsgExtractVolumes& msg) const
{
  msg.m_pCollection->AddImage(GetOwner()->GetGlobalTransformSimd(), m_vExtents, m_BlendMode, m_fSortOrder, m_fValue, m_vFadeOutStart, m_hImage);
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
