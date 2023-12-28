#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Utils/BlackboardTemplateResource.h>
#include <GameEngine/Volumes/VolumeComponent.h>

plSpatialData::Category s_VolumeCategory = plSpatialData::RegisterCategory("GenericVolume", plSpatialData::Flags::None);

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_ABSTRACT_COMPONENT_TYPE(plVolumeComponent, 1)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Template", GetTemplateFile, SetTemplateFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_BlackboardTemplate")),
    PLASMA_ACCESSOR_PROPERTY("SortOrder", GetSortOrder, SetSortOrder)->AddAttributes(new plClampValueAttribute(-64.0f, 64.0f)),
  }
  PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Gameplay"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

plVolumeComponent::plVolumeComponent() = default;
plVolumeComponent::~plVolumeComponent() = default;

void plVolumeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->UpdateLocalBounds();
}

void plVolumeComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  GetOwner()->UpdateLocalBounds();
}

void plVolumeComponent::SetTemplateFile(const char* szFile)
{
  plBlackboardTemplateResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plBlackboardTemplateResource>(szFile);
  }

  SetTemplate(hResource);
}

const char* plVolumeComponent::GetTemplateFile() const
{
  if (!m_hTemplateResource.IsValid())
    return "";

  return m_hTemplateResource.GetResourceID();
}

void plVolumeComponent::SetTemplate(const plBlackboardTemplateResourceHandle& hResource)
{
  m_hTemplateResource = hResource;
}

void plVolumeComponent::SetSortOrder(float fOrder)
{
  fOrder = plMath::Clamp(fOrder, -64.0f, 64.0f);
  m_fSortOrder = fOrder;
}

void plVolumeComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_hTemplateResource;
  s << m_fSortOrder;
}

void plVolumeComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_hTemplateResource;
  s >> m_fSortOrder;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plVolumeSphereComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new plDefaultValueAttribute(5.0f), new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, 1.0f)),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plSphereManipulatorAttribute("Radius"),
    new plSphereVisualizerAttribute("Radius", plColorScheme::LightUI(plColorScheme::Cyan)),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plVolumeSphereComponent::plVolumeSphereComponent() = default;
plVolumeSphereComponent::~plVolumeSphereComponent() = default;

void plVolumeSphereComponent::SetRadius(float fRadius)
{
  if (m_fRadius != fRadius)
  {
    m_fRadius = fRadius;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }
  }
}

void plVolumeSphereComponent::SetFalloff(float fFalloff)
{
  m_fFalloff = fFalloff;
}

void plVolumeSphereComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fFalloff;
}

void plVolumeSphereComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fFalloff;
}

void plVolumeSphereComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(plBoundingSphere(plVec3::ZeroVector(), m_fRadius), s_VolumeCategory);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plVolumeBoxComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new plDefaultValueAttribute(plVec3(10.0f)), new plClampValueAttribute(plVec3(0), plVariant())),
    PLASMA_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new plDefaultValueAttribute(plVec3(0.5f)), new plClampValueAttribute(plVec3(0.0f), plVec3(1.0f))),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plBoxManipulatorAttribute("Extents", 1.0f, true),
    new plBoxVisualizerAttribute("Extents", 1.0f, plColor::LimeGreen),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plVolumeBoxComponent::plVolumeBoxComponent() = default;
plVolumeBoxComponent::~plVolumeBoxComponent() = default;

void plVolumeBoxComponent::SetExtents(const plVec3& vExtents)
{
  if (m_vExtents != vExtents)
  {
    m_vExtents = vExtents;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }
  }
}

void plVolumeBoxComponent::SetFalloff(const plVec3& vFalloff)
{
  m_vFalloff = vFalloff;
}

void plVolumeBoxComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
  s << m_vFalloff;
}

void plVolumeBoxComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;
  s >> m_vFalloff;
}

void plVolumeBoxComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(plBoundingBox(-m_vExtents * 0.5f, m_vExtents * 0.5f), s_VolumeCategory);
}
