#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RecastPlugin/Components/NpcComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_ABSTRACT_COMPONENT_TYPE(plNpcComponent, 1)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("AI/Experimental"),
    new plInDevelopmentAttribute(plInDevelopmentAttribute::Phase::Alpha),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

plNpcComponent::plNpcComponent() = default;
plNpcComponent::~plNpcComponent() = default;

void plNpcComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
}

void plNpcComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();
}



PLASMA_STATICLINK_FILE(GameEngine, GameEngine_AI_Implementation_NpcComponent);
