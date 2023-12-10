#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

struct PLASMA_CORE_DLL plMsgCollision : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgCollision, plMessage);

  plGameObjectHandle m_hObjectA;
  plGameObjectHandle m_hObjectB;

  plComponentHandle m_hComponentA;
  plComponentHandle m_hComponentB;

  plVec3 m_vPosition; ///< The collision position in world space.
  plVec3 m_vNormal;   ///< The collision normal on the surface of object B.
  plVec3 m_vImpulse;  ///< The collision impulse applied from object A to object B.
};
