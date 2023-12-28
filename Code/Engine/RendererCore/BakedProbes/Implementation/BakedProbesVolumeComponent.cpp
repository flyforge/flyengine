#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/BakedProbes/BakedProbesVolumeComponent.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plBakedProbesVolumeComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new plDefaultValueAttribute(plVec3(10.0f)), new plClampValueAttribute(plVec3(0), plVariant())),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plInDevelopmentAttribute(plInDevelopmentAttribute::Phase::Beta),
    new plCategoryAttribute("Lighting/Baking"),
    new plBoxManipulatorAttribute("Extents", 1.0f, true),
    new plBoxVisualizerAttribute("Extents", 1.0f, plColor::OrangeRed),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plBakedProbesVolumeComponent::plBakedProbesVolumeComponent() = default;
plBakedProbesVolumeComponent::~plBakedProbesVolumeComponent() = default;

void plBakedProbesVolumeComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();
}

void plBakedProbesVolumeComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();
}

void plBakedProbesVolumeComponent::SetExtents(const plVec3& vExtents)
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

void plBakedProbesVolumeComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
}

void plBakedProbesVolumeComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;
}

void plBakedProbesVolumeComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(plBoundingBox(-m_vExtents * 0.5f, m_vExtents * 0.5f), plInvalidSpatialDataCategory);
}


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_BakedProbesVolumeComponent);
