#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgMoveCharacterController);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgMoveCharacterController, 1, plRTTIDefaultAllocator<plMsgMoveCharacterController>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("MoveForwards", m_fMoveForwards),
    PLASMA_MEMBER_PROPERTY("MoveBackwards", m_fMoveBackwards),
    PLASMA_MEMBER_PROPERTY("StrafeLeft", m_fStrafeLeft),
    PLASMA_MEMBER_PROPERTY("StrafeRight", m_fStrafeRight),
    PLASMA_MEMBER_PROPERTY("RotateLeft", m_fRotateLeft),
    PLASMA_MEMBER_PROPERTY("RotateRight", m_fRotateRight),
    PLASMA_MEMBER_PROPERTY("Run", m_bRun),
    PLASMA_MEMBER_PROPERTY("Jump", m_bJump),
    PLASMA_MEMBER_PROPERTY("Crouch", m_bCrouch),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_ABSTRACT_COMPONENT_TYPE(plCharacterControllerComponent, 1)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Gameplay"),
  }
  PLASMA_END_ATTRIBUTES;
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(RawMove, In, "moveDeltaGlobal"),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(TeleportCharacter, In, "globalFootPosition"),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(IsDestinationUnobstructed, In, "globalFootPosition", In, "characterHeight"),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(IsTouchingGround),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(IsCrouching),
  }
  PLASMA_END_FUNCTIONS;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgMoveCharacterController, MoveCharacter),
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

plCharacterControllerComponent::plCharacterControllerComponent() {}

void plCharacterControllerComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  // auto& s = stream.GetStream();
}

void plCharacterControllerComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  // auto& s = stream.GetStream();
}



PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Physics_Implementation_CharacterControllerComponent);
