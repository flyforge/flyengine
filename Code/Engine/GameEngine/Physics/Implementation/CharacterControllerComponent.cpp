#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_IMPLEMENT_MESSAGE_TYPE(plMsgMoveCharacterController);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgMoveCharacterController, 1, plRTTIDefaultAllocator<plMsgMoveCharacterController>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("MoveForwards", m_fMoveForwards),
    PL_MEMBER_PROPERTY("MoveBackwards", m_fMoveBackwards),
    PL_MEMBER_PROPERTY("StrafeLeft", m_fStrafeLeft),
    PL_MEMBER_PROPERTY("StrafeRight", m_fStrafeRight),
    PL_MEMBER_PROPERTY("RotateLeft", m_fRotateLeft),
    PL_MEMBER_PROPERTY("RotateRight", m_fRotateRight),
    PL_MEMBER_PROPERTY("Run", m_bRun),
    PL_MEMBER_PROPERTY("Jump", m_bJump),
    PL_MEMBER_PROPERTY("Crouch", m_bCrouch),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_ABSTRACT_COMPONENT_TYPE(plCharacterControllerComponent, 1)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Physics"),
  }
  PL_END_ATTRIBUTES;
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(RawMove, In, "moveDeltaGlobal"),
    PL_SCRIPT_FUNCTION_PROPERTY(TeleportCharacter, In, "globalFootPosition"),
    PL_SCRIPT_FUNCTION_PROPERTY(IsDestinationUnobstructed, In, "globalFootPosition", In, "characterHeight"),
    PL_SCRIPT_FUNCTION_PROPERTY(IsTouchingGround),
    PL_SCRIPT_FUNCTION_PROPERTY(IsCrouching),
  }
  PL_END_FUNCTIONS;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgMoveCharacterController, MoveCharacter),
  }
  PL_END_MESSAGEHANDLERS;
}
PL_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

plCharacterControllerComponent::plCharacterControllerComponent() = default;

void plCharacterControllerComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  // auto& s = stream.GetStream();
}

void plCharacterControllerComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  // auto& s = stream.GetStream();
}



PL_STATICLINK_FILE(GameEngine, GameEngine_Physics_Implementation_CharacterControllerComponent);
