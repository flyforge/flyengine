#pragma once

#include <Core/Messages/EventMessage.h>
#include <GameEngine/GameEngineDLL.h>

struct PL_GAMEENGINE_DLL plMsgDamage : public plEventMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgDamage, plEventMessage);

  double m_fDamage = 0;
  plString m_sHitObjectName; ///< The actual game object that was hit (may be a child of the object to which the message is sent)

  plVec3 m_vGlobalPosition;  ///< The global position at which the damage was applied. Set to zero, if unused.
  plVec3 m_vImpactDirection; ///< The direction into which the damage was applied (e.g. direction of a projectile). May be zero.
};
