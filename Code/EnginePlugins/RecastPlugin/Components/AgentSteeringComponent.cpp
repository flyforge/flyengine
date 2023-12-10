#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RecastPlugin/Components/AgentSteeringComponent.h>

// clang-format off
PLASMA_BEGIN_ABSTRACT_COMPONENT_TYPE(plAgentSteeringComponent, 1)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("AI/Experimental"),
    new plInDevelopmentAttribute(plInDevelopmentAttribute::Phase::Alpha),
  }
  PLASMA_END_ATTRIBUTES;
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(SetTargetPosition, In, "position"),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(GetTargetPosition),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(ClearTargetPosition),
    //PLASMA_SCRIPT_FUNCTION_PROPERTY(GetPathToTargetState),
  }
  PLASMA_END_FUNCTIONS;
}
PLASMA_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

plAgentSteeringComponent::plAgentSteeringComponent() = default;
plAgentSteeringComponent::~plAgentSteeringComponent() = default;

void plAgentSteeringComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
}

void plAgentSteeringComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();
}



PLASMA_STATICLINK_FILE(GameEngine, GameEngine_AI_Implementation_AgentSteeringComponent);
