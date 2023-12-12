#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/XR/StageSpaceComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plStageSpaceComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_ACCESSOR_PROPERTY("StageSpace", plXRStageSpace, GetStageSpace, SetStageSpace)->AddAttributes(new plDefaultValueAttribute((plInt32)plXRStageSpace::Enum::Standing)),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("XR"),
    new plColorAttribute(plColorScheme::XR),
    new plInDevelopmentAttribute(plInDevelopmentAttribute::Phase::Beta),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plStageSpaceComponent::plStageSpaceComponent() = default;
plStageSpaceComponent::~plStageSpaceComponent() = default;

plEnum<plXRStageSpace> plStageSpaceComponent::GetStageSpace() const
{
  return m_Space;
}

void plStageSpaceComponent::SetStageSpace(plEnum<plXRStageSpace> space)
{
  m_Space = space;
}

void plStageSpaceComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  plStreamWriter& s = stream.GetStream();

  s << m_Space;
}

void plStageSpaceComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = stream.GetStream();

  s >> m_Space;
}

void plStageSpaceComponent::OnActivated() {}

void plStageSpaceComponent::OnDeactivated() {}

PLASMA_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_StageSpaceComponent);
