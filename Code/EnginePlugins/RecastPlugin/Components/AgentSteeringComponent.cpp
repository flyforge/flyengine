#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RecastPlugin/Components/AgentSteeringComponent.h>

// clang-format off
PL_BEGIN_ABSTRACT_COMPONENT_TYPE(plAgentSteeringComponent, 1)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("AI/Experimental"),
    new plInDevelopmentAttribute(plInDevelopmentAttribute::Phase::Alpha),
  }
  PL_END_ATTRIBUTES;
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(SetTargetPosition, In, "position"),
    PL_SCRIPT_FUNCTION_PROPERTY(GetTargetPosition),
    PL_SCRIPT_FUNCTION_PROPERTY(ClearTargetPosition),
    //PL_SCRIPT_FUNCTION_PROPERTY(GetPathToTargetState),
  }
  PL_END_FUNCTIONS;
}
PL_END_ABSTRACT_COMPONENT_TYPE
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



PL_STATICLINK_FILE(GameEngine, GameEngine_AI_Implementation_AgentSteeringComponent);
