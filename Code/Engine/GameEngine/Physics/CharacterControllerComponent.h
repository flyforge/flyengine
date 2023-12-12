#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

//////////////////////////////////////////////////////////////////////////

struct PLASMA_GAMEENGINE_DLL plMsgMoveCharacterController : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgMoveCharacterController, plMessage);

  double m_fMoveForwards = 0;
  double m_fMoveBackwards = 0;
  double m_fStrafeLeft = 0;
  double m_fStrafeRight = 0;
  double m_fRotateLeft = 0;
  double m_fRotateRight = 0;
  bool m_bRun = false;
  bool m_bJump = false;
  bool m_bCrouch = false;
  plQuat m_qVelocityRotation = plQuat::IdentityQuaternion();
};

/// \brief Base class for implementations of a character controller.
///
/// A character controller implements the raw physical locomotion aspect of moving a character (player or NPC)
/// through the world. This is typically implemented through a physics engine, when natural collision behavior
/// is desired. In more constricted settings, it could use a much simpler implementation.
class PLASMA_GAMEENGINE_DLL plCharacterControllerComponent : public plComponent
{
  PLASMA_DECLARE_ABSTRACT_COMPONENT_TYPE(plCharacterControllerComponent, plComponent);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // plCharacterControllerComponent

public:
  plCharacterControllerComponent();

  /// \brief Attempts to move the character into the given direction.
  ///
  /// Implements the specific constraints, such as colliding with geometry and sliding along walls.
  /// Should NOT add further functionality, such as gravity. This function applies the result immediately
  /// and moves the owner object to the final location.
  virtual void RawMove(const plVec3& vMoveDeltaGlobal) = 0; // [ scriptable ]

  /// \brief Instructs the CC to move in certain directions. An implementation can queue the request for later processing.
  ///
  /// It can also add further functionality, such as adding gravity, stair stepping, etc.
  virtual void MoveCharacter(plMsgMoveCharacterController& msg) = 0; // [ msg handler ]

  /// \brief Teleports the CC to the desired global position. Ignores obstacles on the path.
  ///
  /// Careful, the CC may get stuck in the new location, if it is inside static geometry.
  /// If it teleports into dynamic geometry, the result may also be undesirable.
  virtual void TeleportCharacter(const plVec3& vGlobalFootPos) = 0; // [ scriptable ]

  /// \brief Checks whether the CC can be teleported to the target position without getting stuck.
  ///
  /// This can be used to check before using TeleportCharacter(). It can also be used to check whether a character
  /// can stand up from a crouching position, by passing a non-zero character height.
  ///
  /// If a character height of 0 is passed in, the current height is used.
  virtual bool IsDestinationUnobstructed(const plVec3& vGlobalFootPos, float fCharacterHeight) = 0; // [ scriptable ]

  /// \brief Checks whether the CC is currently touching the ground.
  virtual bool IsTouchingGround() = 0; // [ scriptable ]

  /// \brief Checks whether the CC is currently in the crouch state.
  virtual bool IsCrouching() = 0; // [ scriptable ]
};
