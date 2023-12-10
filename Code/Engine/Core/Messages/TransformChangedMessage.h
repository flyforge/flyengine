#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

struct PLASMA_CORE_DLL plMsgTransformChanged : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgTransformChanged, plMessage);

  plTransform m_OldGlobalTransform;
  plTransform m_NewGlobalTransform;
};
